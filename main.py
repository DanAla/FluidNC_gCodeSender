#!/usr/bin/env python3
"""
FluidNC_gCodeSender\\main.py
Entry point for FluidNC_gCodeSender
"""

import wx
from gui.main_frame import MainFrame


def main():
    app = wx.App(False)
    frame = MainFrame()
    frame.Show()
    app.MainLoop()


if __name__ == "__main__":
    main()
