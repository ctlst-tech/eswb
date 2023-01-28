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


class EwLocationMap(MyQtWidget, EwBasic):
    # SAMPLE_TODO copy and paste this class , rename properly

    def __init__(self, data_sources: List[DataSourceBasic], *,
                 start_loc=(57.0764, 24.3308),
                 window_size=None,
                 **kwargs):
        MyQtWidget.__init__(self, **kwargs)
        EwBasic.__init__(self)
        self.location_counter = 0
        self.animation_duration = 1.2
        self.location = list(start_loc)
        self.map_zoom = 13

        self.set_data_sources(data_sources)

        if window_size:
            self.setFixedSize(window_size[0], window_size[1])

        self.map = folium.Map(location=self.location,
                              zoom_start=self.map_zoom, control_scale=True,
                              tiles='cartodbpositron')

        self.marker = folium.Marker(
            location=self.location,  # coordinates for the marker (Earth Lab at CU Boulder)
            popup='',  # pop-up label for the marker
            icon=folium.Icon()
        )
        self.marker.add_to(self.map)

        js_path = f"{str(Path(__file__).resolve().parent)}/js/map-helper.j2"
        js_file = QtCore.QUrl.fromLocalFile(js_path).toString()

        with open(js_path) as js_data:
            js_temp = Template(js_data.read())
            self.map.get_root().script.add_child(
                Element(js_temp.render(
                    id=self.map.get_name(),
                    marker_id=self.marker.get_name()
                )))

        data = io.BytesIO()
        self.map.save(data, close_file=False)
        self.webView = QWebEngineView()
        self.webView.setHtml(data.getvalue().decode())
        self.layout.addWidget(self.webView)

    def paintEvent(self, event):
        pass

    def update_web_view(self):
        script = f"jumpTo({list(self.location)}, {self.map_zoom}, {self.animation_duration})"
        # print(f"run: {script}")
        self.webView.page().runJavaScript(script)

    def set_location(self, loc=(0.0, 0.0)):
        if self.location != list(loc):
            self.location = list(loc)
            self.update_web_view()

    def set_zoom(self, z):
        if self.map_zoom != z:
            self.map_zoom = z
            self.update_web_view()

    def set_animation_duration(self, d):
        self.animation_duration = d

    def increment_counter(self):
        self.location_counter = self.location_counter + 1
        if self.location_counter > 2:
            self.location_counter = 0

    def radraw_handler(self, vals: List[Union[float, int, str, NoDataStub]], vals_map: Dict):

        lat = vals[0]
        lon = vals[1]
        alt = vals[2]

        self.set_location(loc=(lat, lon))

        self.repaint()
