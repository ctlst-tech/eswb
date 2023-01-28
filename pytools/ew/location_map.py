import folium

from pathlib import Path
from typing import List, Union, Dict
import io

from branca.element import Element
from jinja2 import Template
from PyQt5 import QtCore
from PyQt5.QtWebEngineWidgets import QWebEngineView

from ds import DataSourceBasic, NoDataStub

from .widgets import EwBasic
from .common import MyQtWidget


class JsWrapperProxy:
    def __init__(self, web_view):
        self.web_view = web_view
        pass

    def _call_js(self, script: str):
        self.web_view.page().runJavaScript(script)

    def add_icon(self, _id, url, loc):
        self._call_js(f"_addIcon({_id}, '{url}', {loc})")

    def move_icon(self, _id, loc):
        self._call_js(f"_moveIcon({_id}, {loc})")

    def rotate_icon(self, _id, angle):
        self._call_js(f"_rotateIcon({_id}, {angle})")

    def zoom_to(self, zoom=14):
        self._call_js(f"_zoomTo({zoom})")

    def jump_to(self, loc, zoom=14, anim_duration=0):
        self._call_js(f"_jumpTo({loc}, {zoom}, {anim_duration})")


class EwLocationMap(MyQtWidget, EwBasic):
    # SAMPLE_TODO copy and paste this class , rename properly

    def __init__(self, data_sources: List[DataSourceBasic], *,
                 window_size=None,
                 **kwargs):
        MyQtWidget.__init__(self, **kwargs)
        EwBasic.__init__(self)

        self.location_counter = 0
        self.animation_duration = 0
        self.location = [57.0764, 24.3308]
        self.zoom_level = 13
        self.rotations = {}

        self.markers = []
        self.vectors = []
        self.path = []
        self.vector_path = f"https://ctlst.ams3.cdn.digitaloceanspaces.com/airplane.svg"

        self.set_data_sources(data_sources)

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
        self.js = JsWrapperProxy(self.web_view)

        self.layout.addWidget(self.web_view)

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

    def add_marker(self, _id, loc):
        self.js.add_icon(_id, self.vector_path, loc)
        self.rotations[_id] = 0

    def get_marker_rotation(self, _id):
        return self.rotations[_id]

    def rotate_marker(self, _id, angle):
        self.rotations[_id] = angle
        self.js.rotate_icon(_id, self.rotations[_id])

    def move_marker(self, _id, loc):
        self.js.move_icon(_id, loc)
        self.js.rotate_icon(_id, self.rotations[_id])

    def set_path(self, path):
        self.path = path

    def clear_path(self):
        self.path = []

    def set_animation_duration(self, d):
        self.animation_duration = d

    def radraw_handler(self, vals: List[Union[float, int, str, NoDataStub]], vals_map: Dict):
        lat = vals[0]
        lon = vals[1]
        alt = vals[2]
