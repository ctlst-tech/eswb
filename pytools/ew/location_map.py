import folium

from pathlib import Path
from typing import List, Union, Dict, Tuple
import io

from PyQt5.QtWidgets import QPushButton

from branca.element import Element
from jinja2 import Template
from PyQt5 import QtCore
from PyQt5.QtWebEngineWidgets import QWebEngineView

from ds import DataSourceBasic, NoDataStub, DataSourceConst

from .widgets import EwBasic
from .common import MyQtWidget

class JsWrapperProxy:
    def __init__(self, web_view):
        self.web_view = web_view
        pass

    def _call_js(self, script: str):
        self.web_view.page().runJavaScript(script)

    def zoom_to(self, zoom=14):
        self._call_js(f"_zoomTo({zoom})")

    def jump_to(self, loc, zoom=14, anim_duration=0):
        self._call_js(f"_jumpTo({loc}, {zoom}, {anim_duration})")

    def add_marker(self, _id, url, loc, width=40, height=40):
        self._call_js(f"_addMarker('{_id}', '{url}', {loc}, {width}, {height})")

    def move_marker(self, _id, loc):
        self._call_js(f"_moveMarker('{_id}', {loc})")

    def rotate_marker(self, _id, angle):
        self._call_js(f"_rotateMarker('{_id}', {angle})")

    def remove_marker(self, _id):
        self._call_js(f"_removeMarker('{_id}')")

    def add_poly(self, _id, latlngs, color='#ff0000'):
        self._call_js(f"_addPolyline('{_id}', {latlngs}, '{color}')")

    def update_poly(self, _id, latlngs):
        self._call_js(f"_updatePolyline('{_id}', {latlngs})")

    def remove_poly(self, _id):
        self._call_js(f"_removePolyline('{_id}')")

    def fit_poly(self, _id):
        self._call_js(f"_fitPolyline('{_id}')")


class EwLocationMap(MyQtWidget, EwBasic):
    # SAMPLE_TODO copy and paste this class , rename properly

    marker_style_variants = [
        {'w': 40, 'h': 40, 'color': '#ff0000'},
        {'w': 20, 'h': 30, 'color': '#00ff00'},
        {'w': 10, 'h': 10, 'color': '#0000ff'},
    ]

    class Marker:
        def __init__(self, parenting_js, name, initial_loc, *, icon='plane', style=None, trace_len=200):
            self.js = parenting_js
            self.name = name
            self.loc = initial_loc
            self.rotation = 0

            self.width = 40
            self.height = 40
            self.trace_color = '#ff0000'

            if style:
                self.width = style['w']
                self.height = style['h']
                self.trace_color = style['color']

            if icon == 'plane':
                self.icon_path = f"https://ctlst.ams3.cdn.digitaloceanspaces.com/airplane.svg"
            elif icon == 'aim':
                self.icon_path = f"https://ctlst.ams3.cdn.digitaloceanspaces.com/aim_map_black.svg"
            else:
                self.icon_path = f"https://ctlst.ams3.cdn.digitaloceanspaces.com/airplane.svg"

            self.show_marker()

            self.trace: List[List[float, float]] = []
            self.trace_len = trace_len

            self.trace_is_on = False

        def get_marker_rotation(self):
            return self.rotation

        def rotate_marker(self, angle):
            self.rotation = angle
            self.js.rotate_marker(self.name, self.rotation)

        def show_marker(self):
            self.js.add_marker(self.name, self.icon_path, self.loc, self.width, self.height)

        def hide_marker(self, _id):
            self.js.remove_marker(self.name)

        def move_marker(self, loc):
            self.loc = loc

            if loc != [0.0, 0.0]:
                if self.trace:
                    if self.trace[-1] != loc:
                        self.trace.append(loc)
                    else:
                        pass
                else:
                    self.trace.append(loc)

            if len(self.trace) > self.trace_len:
                self.trace = self.trace[1:]

            self.js.move_marker(self.name, self.loc)
            self.js.rotate_marker(self.name, self.rotation)

        def add_trace(self, latlngs, color='#ff0000'):
            self.js.add_poly(self.name, latlngs, color)

        def fit_trace(self):
            self.js.fit_poly(self.name)

        def hide_trace(self):
            self.js.remove_poly(self.name)

        def redraw_trace(self, latlngs):
            self.js.update_poly(self.name, latlngs)

        def update_trace(self):
            if not self.trace_is_on:
                self.hide_trace()
                self.add_trace(self.trace, self.trace_color)
                self.trace_is_on = True
            else:
                self.redraw_trace(self.trace)

    def __init__(self, data_sources: List[Tuple[str, DataSourceBasic, DataSourceBasic, DataSourceBasic]], *,
                 window_size=None, with_control=False, **kwargs):
        MyQtWidget.__init__(self, **kwargs)
        EwBasic.__init__(self)

        self.location_counter = 0
        self.animation_duration = 0
        self.location = [57.0764, 24.3308]
        self.zoom_level = 13
        self.rotations = {}

        if window_size:
            self.setFixedSize(window_size[0], window_size[1])

        self.map = folium.Map(location=self.location,
                              zoom_start=self.zoom_level, control_scale=True,
                              tiles='cartodbpositron')

        js_path = f"{str(Path(__file__).resolve().parent)}/js/map-helper.j2"
        self._add_script(js_path)

        data = io.BytesIO()
        self.map.save(data, close_file=False)
        self.web_view = QWebEngineView()
        self.web_view.setHtml(data.getvalue().decode())

        self.markers = []

        def on_load_finished(data_sources):
            def callback():
                data_sources_list = []
                for d in data_sources:
                    l = len(self.markers)
                    style = self.marker_style_variants[l if l < len(self.marker_style_variants) else self.marker_style_variants[-1]]
                    self.markers.append(self.Marker(self.js, d[0], [57.0764, 24.3308], style=style, icon='aim'))
                    dr = d[3]
                    if not dr:
                        dr = DataSourceConst('rotation', value=0)

                    data_sources_list += [d[1], d[2], dr]
                self.set_data_sources(data_sources_list)

            return callback


        self.web_view.loadFinished.connect(on_load_finished(data_sources))

        self.js = JsWrapperProxy(self.web_view)

        if with_control:
            on_marker_btn = QPushButton()
            on_marker_btn.setText("On marker")
            on_marker_btn.clicked.connect(self.map_to_marker)
            self.layout.addWidget(on_marker_btn)

        self.layout.addWidget(self.web_view)

    def map_to_marker(self):
        self.center_map(self.markers[0].loc)

    def _add_script(self, js_path):
        with open(js_path) as js_data:
            js_temp = Template(js_data.read())
            self.map.get_root().script.add_child(
                Element(js_temp.render(
                    id=self.map.get_name()
                )))

    def paintEvent(self, event):
        pass

    def center_map(self, loc):
        self.location = loc
        self.js.jump_to(self.location, self.zoom_level, self.animation_duration)

    def zoom_to(self, z):
        self.zoom_level = z
        self.js.zoom_to(self.zoom_level)

    def set_animation_duration(self, d):
        self.animation_duration = d

    def radraw_handler(self, vals: List[Union[float, int, str, NoDataStub]], vals_map: Dict):
        for i in range(0, len(self.markers)):
            if not isinstance(vals[i * 3], NoDataStub) and not isinstance(vals[i * 3 + 1], NoDataStub):
                self.markers[i].move_marker([vals[i * 3], vals[i * 3 + 1]])
                self.markers[i].update_trace()

            if not isinstance(vals[i * 3 + 2], NoDataStub):
                self.markers[i].rotate_marker(vals[i * 3 + 2])
