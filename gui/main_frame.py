'''
gui/main_frame.py
Uses wx.aui.AuiManager for dockable panes:
SVG viewer (left)
GCode editor (right)
DRO + Jog (bottom)
Macro panel (tabbed)
All panes are resizable, floatable, and save their state via state_manager.
'''

"""
Single-window application wxPython frame using wx.aui.AuiManager.  
Embedding all panes and handling for flexible docking.
Automatically restores positions from StateManager.
View menu toggles plus connection state logic.
- All panes are embedded (no floating dialogs).
- View menu toggles visibility.
- Telnet setup pane appears when host/port missing or unreachable.
- Layout saved/loaded without maximisation.
"""

import wx
from wx import aui

from core.state_manager import StateManager
from core.fluidnc_client import FluidNCClient
from .dro_panel import DROPanel
from .jog_panel import JogPanel
from .settings_panel import SettingsPanel
from .telnet_setup_panel import TelnetSetupPanel
from .svg_viewer_dummy import SVGViewerDummy


class MainFrame(wx.Frame):
    def __init__(self):
        wx.Frame.__init__(self, None, title="FluidNC_gCodeSender", size=(1200, 800))
        self.state = StateManager.get()

        # AUI manager
        self._mgr = aui.AuiManager(self)
        self._mgr.SetManagedWindow(self)

        # Build client
        self.client = FluidNCClient(
            host=self.state.get_value("telnet/host", ""),
            port=self.state.get_value("telnet/port", 23),
            dro_callback=self._on_dro,
        )
        self.client.on_connect_cb = lambda: wx.CallAfter(self._enter_normal_mode)
        self.client.on_disconnect_cb = lambda: wx.CallAfter(self._enter_setup_mode)

        # Child panes
        self.setup_panel = TelnetSetupPanel(self, on_success=self._enter_normal_mode)
        self.dro_panel = DROPanel(self, self.client)
        self.jog_panel = JogPanel(self, self.client)
        self.settings_panel = SettingsPanel(self)
        self.svg_panel = SVGViewerDummy(self)

        # Menu bar
        mb = wx.MenuBar()
        file_menu = wx.Menu()
        quit_item = file_menu.Append(wx.ID_EXIT, "E&xit\tCtrl-Q")
        self.Bind(wx.EVT_MENU, self.on_quit, quit_item)
        mb.Append(file_menu, "&File")

        view_menu = wx.Menu()
        self.view_items = {}
        for name, panel in (
            ("Settings", self.settings_panel),
            ("DRO", self.dro_panel),
            ("Jog", self.jog_panel),
            ("SVG Viewer", self.svg_panel),
        ):
            item = view_menu.AppendCheckItem(wx.ID_ANY, f"&{name}")
            self.view_items[name] = (item, panel)
            self.Bind(wx.EVT_MENU, self.on_toggle_view, item)
        mb.Append(view_menu, "&View")
        self.SetMenuBar(mb)

        # Restore window geometry
        geo = self.state.get_value("window_geometry")
        if geo and len(geo) == 4:
            self.SetRect(geo)

        # Decide initial mode
        if not self.client.host:
            self._enter_setup_mode()
        else:
            self.client.start()
            if not self.client.connected:
                self._enter_setup_mode()
            else:
                self._enter_normal_mode()

        self.Bind(wx.EVT_CLOSE, self.on_close)

    # ---------- Mode switching ----------
    def _enter_setup_mode(self):
        self.client.auto_reconnect = False
        self.client.stop()

        # Hide every normal pane
        for _, panel in self.view_items.values():
            self._mgr.GetPane(panel).Hide()

        # Show the full-size setup pane
        self._mgr.DetachPane(self.setup_panel)
        self._mgr.AddPane(
            self.setup_panel,
            aui.AuiPaneInfo().Name("setup").Center().CloseButton(False),
        )
        self._mgr.GetPane("setup").Show()
        self._mgr.Update()
        self._sync_view_menu()

    def _enter_normal_mode(self):
        # ---------- Shrink connection bar ----------
        self._mgr.GetPane("setup").Top().Row(0).Layer(0).Fixed().BestSize(
            -1, 30
        ).CaptionVisible(False).CloseButton(False).Show(True)

        # ---------- Build or reload layout ----------
        saved = self.state.get_value("aui_layout", {})
        if saved and saved.get("perspective"):
            self._mgr.LoadPerspective(saved["perspective"], update=False)
        else:
            # FIRST-RUN: no CenterPane() anywhere
            self._mgr.AddPane(
                self.settings_panel,
                aui.AuiPaneInfo()
                .Name("settings").Caption("Settings")
                .Right().BestSize(300, 400).MinSize(250, 300),
            )
            self._mgr.AddPane(
                self.dro_panel,
                aui.AuiPaneInfo()
                .Name("dro").Caption("DRO")
                .Bottom().Row(0).Position(0)
                .BestSize(400, 120).MinSize(200, 100)
                .CloseButton(False),
            )
            self._mgr.AddPane(
                self.jog_panel,
                aui.AuiPaneInfo()
                .Name("jog").Caption("Jog")
                .Bottom().Row(0).Position(1)
                .BestSize(400, 150).MinSize(200, 120)
                .CloseButton(False),
            )
            self._mgr.AddPane(
                self.svg_panel,
                aui.AuiPaneInfo()
                .Name("svg").Caption("SVG Viewer")
                .CentrePane()        # <— REMOVE THIS LINE
                .BestSize(600, 400).MinSize(300, 200)
                .Floatable(False).Resizable(True),
            )

        # Remove any maximise flag before saving
        for name in ("dro", "jog", "settings", "svg"):
            pane = self._mgr.GetPane(name)
            if pane.IsOk():
                pane.MaximizeButton(False).Maximize(False)

        self._mgr.Update()
        self._sync_view_menu()

    # ---------- Menu helpers ----------
    def on_toggle_view(self, evt):
        item = evt.GetEventObject().FindItemById(evt.GetId())
        name = item.GetItemLabel().lstrip("&")
        _, panel = self.view_items[name]
        pane = self._mgr.GetPane(panel)
        pane.Show(not pane.IsShown())
        self._mgr.Update()
        self._sync_view_menu()

    def _sync_view_menu(self):
        for name, (item, panel) in self.view_items.items():
            pane = self._mgr.GetPane(panel)
            item.Check(pane.IsShown() and pane.IsOk())

    # ---------- Events ----------
    def on_quit(self, _evt):
        self.Close()

    def on_close(self, _evt):
        self.client.stop()
        final = self._mgr.SavePerspective()
        final = final.replace("Maximize=1", "Maximize=0")
        self.state.set_value("aui_layout", {"perspective": final})
        self.state.set_value("window_geometry", list(self.GetRect()))
        self.state.save()
        self.state.shutdown()
        self._mgr.UnInit()
        self.Destroy()
    
    # ---------- Telnet DRO ----------
    def _on_dro(self, mpos, wpos):
        wx.CallAfter(self.dro_panel.update_dro, mpos, wpos)
