"""
gui\telnet_setup_panel.py
Embedded connection pane shown when host/port are missing or when the connection
cannot be established.  Allows editing host, port, retries,
retry interval and offers a manual “Test Connection” button.
Embedded pane:
- Initially full-size, workspace-covering.
- After successful connect collapses to a slim bar at the top.
"""

import socket
import wx
from core.state_manager import StateManager


class TelnetSetupPanel(wx.Panel):
    def __init__(self, parent, on_success):
        super().__init__(parent)
        self.on_success = on_success  # callback when connection ok
        self.state = StateManager.get()

        # ---------- Full-setup widgets ----------
        self.full_panel = wx.Panel(self)
        host_s = wx.BoxSizer(wx.HORIZONTAL)
        host_s.Add(wx.StaticText(self.full_panel, label="Host:"), 0, wx.CENTER | wx.ALL, 5)
        self.host_tc = wx.TextCtrl(self.full_panel, value=str(self.state.get_value("telnet/host", "")))
        host_s.Add(self.host_tc, 1, wx.ALL, 5)

        port_s = wx.BoxSizer(wx.HORIZONTAL)
        port_s.Add(wx.StaticText(self.full_panel, label="Port:"), 0, wx.CENTER | wx.ALL, 5)
        self.port_tc = wx.TextCtrl(self.full_panel, value=str(self.state.get_value("telnet/port", 23)))
        port_s.Add(self.port_tc, 1, wx.ALL, 5)

        retry_s = wx.BoxSizer(wx.HORIZONTAL)
        retry_s.Add(wx.StaticText(self.full_panel, label="Retries:"), 0, wx.CENTER | wx.ALL, 5)
        self.retry_tc = wx.TextCtrl(
            self.full_panel, value=str(self.state.get_value("telnet/connect_retries", 3))
        )
        retry_s.Add(self.retry_tc, 1, wx.ALL, 5)

        interval_s = wx.BoxSizer(wx.HORIZONTAL)
        interval_s.Add(
            wx.StaticText(self.full_panel, label="Retry interval (s):"), 0, wx.CENTER | wx.ALL, 5
        )
        self.interval_tc = wx.TextCtrl(
            self.full_panel, value=str(self.state.get_value("telnet/retry_interval_s", 2))
        )
        interval_s.Add(self.interval_tc, 1, wx.ALL, 5)

        self.test_btn = wx.Button(self.full_panel, label="Test Connection")
        self.status_st = wx.StaticText(self.full_panel, label="Not connected")

        f_sizer = wx.BoxSizer(wx.VERTICAL)
        f_sizer.Add(host_s, 0, wx.EXPAND)
        f_sizer.Add(port_s, 0, wx.EXPAND)
        f_sizer.Add(retry_s, 0, wx.EXPAND)
        f_sizer.Add(interval_s, 0, wx.EXPAND)
        f_sizer.Add(self.test_btn, 0, wx.ALL | wx.CENTER, 10)
        f_sizer.Add(self.status_st, 0, wx.ALL | wx.CENTER, 10)
        self.full_panel.SetSizer(f_sizer)

        # ---------- Compact bar widgets ----------
        self.compact_panel = wx.Panel(self)
        self.status_compact = wx.StaticText(self.compact_panel, label="Connected")
        self.status_compact.SetForegroundColour(wx.Colour(0, 128, 0))  # dark green
        c_sizer = wx.BoxSizer(wx.HORIZONTAL)
        c_sizer.Add(self.status_compact, 1, wx.ALIGN_CENTER_VERTICAL | wx.ALL, 5)
        self.compact_panel.SetSizer(c_sizer)

        # ---------- Layout manager ----------
        self.sizer = wx.BoxSizer(wx.VERTICAL)
        self.sizer.Add(self.full_panel, 1, wx.EXPAND)
        self.sizer.Add(self.compact_panel, 0, wx.EXPAND)
        self.SetSizer(self.sizer)

        self.show_full(True)  # start big

        # Events
        self.Bind(wx.EVT_BUTTON, self.on_test, self.test_btn)

    def show_full(self, full: bool):
        """Toggle between full setup and compact bar."""
        self.full_panel.Show(full)
        self.compact_panel.Show(not full)
        self.Layout()
        if self.GetParent():
            self.GetParent().Layout()

    def on_test(self, _evt):
        host = self.host_tc.GetValue().strip()
        try:
            port = int(self.port_tc.GetValue())
            retries = int(self.retry_tc.GetValue())
            interval = int(self.interval_tc.GetValue())
        except ValueError:
            self.status_st.SetLabel("Port/Retries/Interval must be integers")
            self.status_st.SetForegroundColour(wx.RED)
            return

        self.status_st.SetLabel("Testing…")
        self.status_st.SetForegroundColour(wx.BLACK)
        self.Update()

        # Save settings
        self.state.set_value("telnet/host", host)
        self.state.set_value("telnet/port", port)
        self.state.set_value("telnet/connect_retries", retries)
        self.state.set_value("telnet/retry_interval_s", interval)
        self.state.save()

        try:
            sock = socket.create_connection((host, port), timeout=3)
            sock.close()
            self.status_st.SetLabel("Connection OK")
            self.status_st.SetForegroundColour(wx.Colour(0, 128, 0))  # dark green
            self.show_full(False)  # collapse to bar
            self.on_success()
        except Exception as e:
            self.status_st.SetLabel(str(e))
            self.status_st.SetForegroundColour(wx.RED)
