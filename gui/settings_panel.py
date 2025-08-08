"""
gui\settings_panel.py
A simple property-grid-like editor that reflects the *entire*
settings.json file.  Changes are written back immediately.
"""

import wx
import wx.propgrid as pg
from core.state_manager import StateManager


class SettingsPanel(wx.Panel):
    def __init__(self, parent):
        super().__init__(parent)
        self.state = StateManager.get()

        self.pg = pg.PropertyGridManager(
            self,
            style=pg.PG_TOOLBAR | pg.PG_DESCRIPTION
        )
        self.fill_grid()

        # Layout
        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(self.pg, 1, wx.EXPAND)
        self.SetSizer(sizer)

        # Events
        self.Bind(pg.EVT_PG_CHANGED, self.on_change)

    def fill_grid(self):
        """Populate grid from current JSON."""
        self.pg.Clear()
        data = self.state.get_value("", {})
        self._add_recursive("", data)

    def _add_recursive(self, prefix: str, node):
        """Recursively add nested dicts as categories."""
        for key, value in node.items():
            full_key = f"{prefix}.{key}" if prefix else key
            if isinstance(value, dict):
                cat = self.pg.Append(pg.PropertyCategory(key))
                self._add_recursive(full_key, value)
            else:
                self.pg.SetPropertyValue(
                    self.pg.Append(pg.StringProperty(key, value=str(value))),
                    str(value)
                )

    def on_change(self, evt):
        """Reflect grid change back into JSON."""
        prop = evt.GetProperty()
        if not prop or prop.IsCategory():
            return
        path = prop.GetName()
        value = prop.GetValueAsString()
        # Build nested dict
        keys = path.split(".")
        with self.state._lock:
            d = self.state._data
            for k in keys[:-1]:
                d = d.setdefault(k, {})
            try:
                # cast to int / float if possible
                if value.lower() in ("true", "false"):
                    value = value.lower() == "true"
                elif "." in value:
                    value = float(value)
                else:
                    value = int(value)
            except ValueError:
                pass  # keep as string
            d[keys[-1]] = value
            self.state.save()
