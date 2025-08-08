'''
comm/fluidnc_client.py
FluidNC Telnet Client
Async telnet client (threaded)
Reconnect on failure
Sends GCode line-by-line with OK response
Streams DRO updates
Handles $G, $J=, $H, etc.
'''


"""
core/fluidnc_client.py
Asynchronous, thread-safe Telnet client for FluidNC.
Provides:
  * real-time DRO (machine and work coordinates)
  * streaming G-Code line-by-line with ack
  * manual commands
  * connection state callbacks (no wx in this module)
"""

import socket
import threading
import time
import queue
from typing import Callable, Optional, List

_EOL = b"\r\n"


class FluidNCClient:
    """
    Usage:

        client = FluidNCClient(host, port, on_dro)
        client.on_connect_cb    = lambda: ...  # GUI must wrap with wx.CallAfter
        client.on_disconnect_cb = lambda: ...
        client.start()
        client.send_gcode_line("G0 X10")
        ...
        client.stop()
    """

    def __init__(
        self,
        host: str,
        port: int,
        dro_callback: Optional[Callable[[List[float], List[float]], None]] = None,
    ):
        self.host = host
        self.port = port
        self.dro_callback = dro_callback or (lambda *_: None)

        # Callbacks injected by GUI
        self.on_connect_cb: Optional[Callable[[], None]] = None
        self.on_disconnect_cb: Optional[Callable[[], None]] = None

        # Threading
        self._sock: Optional[socket.socket] = None
        self._rx_thread: Optional[threading.Thread] = None
        self._tx_thread: Optional[threading.Thread] = None
        self._running = False
        self._tx_queue: "queue.Queue[str]" = queue.Queue()

        # State
        self.auto_reconnect = False
        self.connected = False
        self._mpos = [0.0, 0.0, 0.0]
        self._wpos = [0.0, 0.0, 0.0]

    # ---------- Public API ----------
    def start(self) -> None:
        """Connect and spawn threads."""
        if self._running:
            return
        self._running = True
        self._rx_thread = threading.Thread(target=self._rx_loop, daemon=True)
        self._tx_thread = threading.Thread(target=self._tx_loop, daemon=True)
        self._rx_thread.start()
        self._tx_thread.start()

    def stop(self) -> None:
        self._running = False
        if self._sock:
            try:
                self._sock.shutdown(socket.SHUT_RDWR)
            except OSError:
                pass
            self._sock.close()
            self._sock = None
        if self._rx_thread and self._rx_thread.is_alive():
            self._rx_thread.join(timeout=1.0)
        if self._tx_thread and self._tx_thread.is_alive():
            self._tx_thread.join(timeout=1.0)

    def send_gcode_line(self, line: str) -> None:
        """Thread-safe enqueue."""
        self._tx_queue.put(line.strip())

    # ---------- Internal ----------
    def _connect(self) -> None:
        while self._running and self.auto_reconnect:
            try:
                self._sock = socket.create_connection((self.host, self.port), timeout=5)
                self.connected = True
                if self.on_connect_cb:
                    self.on_connect_cb()
                break
            except OSError as e:
                self.connected = False
                if self.on_disconnect_cb:
                    self.on_disconnect_cb()
                if not self.auto_reconnect:            # ← NEW early exit
                    break
                print("[FluidNC] connection failed, retry in 2 s:", e)
                time.sleep(2)

    def _rx_loop(self) -> None:
        buf = bytearray()
        while self._running:
            if not self.connected:
                self._connect()
                continue
            try:
                data = self._sock.recv(4096)
                if not data:
                    raise ConnectionResetError
                buf.extend(data)
                while b"\n" in buf:
                    line, buf = buf.split(b"\n", 1)
                    self._handle_line(line.decode(errors="ignore").strip())
            except (OSError, ConnectionResetError):
                self.connected = False
                if self.on_disconnect_cb:
                    self.on_disconnect_cb()
                time.sleep(1)

    def _tx_loop(self) -> None:
        while self._running:
            try:
                line = self._tx_queue.get(timeout=0.2)
            except queue.Empty:
                continue
            if not self.connected:
                self._tx_queue.put(line)  # re-queue
                time.sleep(0.5)
                continue
            try:
                self._sock.sendall((line + "\r\n").encode())
            except OSError:
                self.connected = False
                self._tx_queue.put(line)  # re-queue

    def _handle_line(self, line: str) -> None:
        """Parse <...> status messages for DRO."""
        if line.startswith("<") and line.endswith(">"):
            parts = line[1:-1].split("|")
            for part in parts:
                if part.startswith("MPos:"):
                    self._mpos = list(map(float, part[5:].split(",")))
                elif part.startswith("WPos:"):
                    self._wpos = list(map(float, part[5:].split(",")))
            self.dro_callback(self._mpos, self._wpos)
