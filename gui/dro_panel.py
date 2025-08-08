'''
gui/dro_panel.py
Real-time DRO from FluidNC
Manual GCode input
Jog buttons (X, Y, Z, A) with speed slider
'''

"""
dro_panel.py
Shows machine and work coordinates in real time.
Allows manual G-Code input with history.
"""

import wx
from core.state_manager import StateManager


class DROPanel(wx.Panel):
    def __init__(self, parent, client):
        wx.Panel.__init__(self, parent)
        self.client = client
        self.state = StateManager.get()

        # Widgets
        self.mpos_text = wx.StaticText(self, label="MPos: --------")
        self.wpos_text = wx.StaticText(self, label="WPos: --------")
        self.cmd_text = wx.TextCtrl(self, style=wx.TE_PROCESS_ENTER)
        self.history = wx.ListBox(self, style=wx.LB_SINGLE)
        self.send_btn = wx.Button(self, label="Send")

        # Layout
        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(self.mpos_text, 0, wx.ALL, 5)
        sizer.Add(self.wpos_text, 0, wx.ALL, 5)
        sizer.Add(wx.StaticLine(self), 0, wx.EXPAND | wx.ALL, 5)
        sizer.Add(wx.StaticText(self, label="Manual G-Code:"), 0, wx.ALL, 5)
        hs = wx.BoxSizer(wx.HORIZONTAL)
        hs.Add(self.cmd_text, 1, wx.ALL | wx.EXPAND, 2)
        hs.Add(self.send_btn, 0, wx.ALL, 2)
        sizer.Add(hs, 0, wx.EXPAND)
        sizer.Add(self.history, 1, wx.EXPAND | wx.ALL, 2)
        self.SetSizer(sizer)

        # Events
        self.Bind(wx.EVT_BUTTON, self.on_send, self.send_btn)
        self.Bind(wx.EVT_TEXT_ENTER, self.on_send, self.cmd_text)

        # Restore history
        hist = self.state.get_value("gcode_history", [])
        self.history.AppendItems(hist)

    # ---------- Public ----------
    def update_dro(self, mpos, wpos):
        self.mpos_text.SetLabel(f"MPos: {mpos}")
        self.wpos_text.SetLabel(f"WPos: {wpos}")

    # ---------- Events ----------
    def on_send(self, _evt):
        cmd = self.cmd_text.GetValue().strip()
        if not cmd:
            return
        self.client.send_gcode_line(cmd)
        # Add to history
        if self.history.FindString(cmd) == wx.NOT_FOUND:
            self.history.Append(cmd)
            with self.state._lock:
                self.state.set_value("gcode_history",
                                     list(self.history.GetStrings()))
        self.cmd_text.Clear()
