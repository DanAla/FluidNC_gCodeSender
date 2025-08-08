"""
gui\svg_viewer_dummy.py
"""




import wx

class SVGViewerDummy(wx.Panel):
    def __init__(self, parent):
        super().__init__(parent)
        s = wx.BoxSizer(wx.VERTICAL)
        s.Add(wx.StaticText(self, label="SVG Viewer Placeholder"), 1,
              wx.ALIGN_CENTER | wx.ALL, 20)
        self.SetSizer(s)
