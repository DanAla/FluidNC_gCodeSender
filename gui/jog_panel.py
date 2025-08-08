'''
gui/jog_panel.py
Homing buttons (per axis)
Custom homing sequences (editable)
Jog speed presets
'''

"""
jog_panel.py
Jog buttons and homing controls.
"""

import wx
from core.state_manager import StateManager


class JogPanel(wx.Panel):
    def __init__(self, parent, client):
        wx.Panel.__init__(self, parent)
        self.client = client
        self.state = StateManager.get()

        # Jog speed slider
        self.speed_slider = wx.Slider(self, value=1000, minValue=100,
                                      maxValue=5000, style=wx.SL_HORIZONTAL)
        speed_sizer = wx.StaticBoxSizer(wx.VERTICAL, self, "Jog Speed (mm/min)")
        speed_sizer.Add(self.speed_slider, 0, wx.EXPAND | wx.ALL, 5)

        # Axis jog buttons
        axes = ["X", "Y", "Z"]
        gs = wx.GridSizer(3, 3, 2, 2)
        for ax in axes:
            gs.Add(wx.Button(self, label=f"-{ax}"), flag=wx.EXPAND)
            gs.Add(wx.StaticText(self, label=ax), flag=wx.ALIGN_CENTER)
            gs.Add(wx.Button(self, label=f"+{ax}"), flag=wx.EXPAND)

        # Homing
        homing_box = wx.StaticBoxSizer(wx.HORIZONTAL, self, "Homing")
        for ax in axes:
            homing_box.Add(wx.Button(self, label=f"Home {ax}"), 0, wx.ALL, 2)
        self.home_all_btn = wx.Button(self, label="Home All")
        homing_box.Add(self.home_all_btn, 0, wx.ALL, 2)

        # Main sizer
        main = wx.BoxSizer(wx.VERTICAL)
        main.Add(speed_sizer, 0, wx.EXPAND | wx.ALL, 5)
        main.Add(gs, 0, wx.EXPAND | wx.ALL, 5)
        main.Add(homing_box, 0, wx.EXPAND | wx.ALL, 5)
        self.SetSizer(main)

        # Bindings
        self.Bind(wx.EVT_BUTTON, self.on_jog)
        self.Bind(wx.EVT_BUTTON, self.on_home)

    # ---------- Events ----------
    def on_jog(self, evt: wx.CommandEvent):
        btn = evt.GetEventObject()
        label = btn.GetLabel()
        if len(label) != 2:
            return
        sign = 1 if label[0] == "+" else -1
        axis = label[1]
        speed = self.speed_slider.GetValue()
        dist = 1.0  # mm
        cmd = f"$J=G91 {axis}{sign*dist} F{speed}"
        self.client.send_gcode_line(cmd)

    def on_home(self, evt: wx.CommandEvent):
        btn = evt.GetEventObject()
        label = btn.GetLabel()
        if label == "Home All":
            cmd = "$H"
        else:
            axis = label[-1]
            cmd = f"$H{axis}"
        self.client.send_gcode_line(cmd)


