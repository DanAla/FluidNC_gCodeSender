"""
core/state_manager.py
Handles all persistent state:
Window positions
SVG/GCode files
Macros
Settings (feeds, speeds, etc.)
Last operation state (for power-cut recovery)
Uses json or sqlite for saving. Auto-saves every 5 seconds.
____________

state_manager.py
Central place to load / save the complete application state.
Uses JSON so the file can also be read by the future C++ version
(nlohmann::json is 100 % compatible).

All GUI panes, window geometry, last used folders, macros, etc.
are stored in  config/settings.json
An additional  config/recovery.json  is written every 5 s
to allow resuming after power loss.
"""

import json
import os
import time
import threading
from typing import Any, Dict

CONFIG_DIR = "config"
SETTINGS_FILE = os.path.join(CONFIG_DIR, "settings.json")
RECOVERY_FILE = os.path.join(CONFIG_DIR, "recovery.json")

# Ensure folder exists
os.makedirs(CONFIG_DIR, exist_ok=True)


class StateManager:
    """Thread-safe, autosaving state container."""

    _instance: "StateManager" = None

    def __init__(self) -> None:
        if StateManager._instance is not None:
            raise RuntimeError("Use StateManager.get()")
        self._lock = threading.RLock()
        self._data: Dict[str, Any] = {}
        self._load()
        # Autosave thread
        self._stop_evt = threading.Event()
        self._thread = threading.Thread(target=self._autosave, daemon=True)
        self._thread.start()

    # ---------- Singleton ----------
    @staticmethod
    def get() -> "StateManager":
        if StateManager._instance is None:
            StateManager._instance = StateManager()
        return StateManager._instance

    # ---------- Public API ----------
    def get_value(self, key: str, default=None):
        with self._lock:
            return self._data.get(key, default)

    def set_value(self, key: str, value: Any) -> None:
        with self._lock:
            self._data[key] = value

    def save(self) -> None:
        """Manual save to settings.json."""
        with self._lock:
            with open(SETTINGS_FILE, "w", encoding="utf-8") as f:
                json.dump(self._data, f, indent=2)

    def save_recovery(self) -> None:
        """Fast dump for power-cut recovery."""
        with self._lock:
            with open(RECOVERY_FILE, "w", encoding="utf-8") as f:
                json.dump(self._data, f)

    # ---------- Private ----------
    def _load(self) -> None:
        if os.path.isfile(SETTINGS_FILE):
            try:
                with open(SETTINGS_FILE, encoding="utf-8") as f:
                    self._data = json.load(f)
            except Exception:
                self._data = {}
        else:
            self._data = {}

    def _autosave(self) -> None:
        """Runs in daemon thread, saves recovery file every 5 s."""
        while not self._stop_evt.wait(5.0):
            self.save_recovery()

    def shutdown(self) -> None:
        self._stop_evt.set()
        self.save()
