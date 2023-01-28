import math
from typing import List, Union, Dict

from PyQt5.QtCore import Qt, QPointF
from PyQt5.QtGui import QPainter, QPalette, QPainterPath, QPen

from ds import DataSourceBasic, NoDataStub

from .common import MyQtWidget, rel_path
from .widgets import EwBasic


class EwRelativePosition(MyQtWidget, EwBasic):
    # SAMPLE_TODO copy and paste this class , rename properly

    def __init__(self, data_sources: List[DataSourceBasic], fixed_size=None, **kwargs):
        MyQtWidget.__init__(self, **kwargs)
        EwBasic.__init__(self)

        # SAMPLE_TODO pass list of data to
        self.set_data_sources(data_sources)

        # SAMPLE_TODO define widget size policy
        if fixed_size:
            self.setFixedSize(fixed_size[0], fixed_size[1])
        # self.setMinimumHeight(80)
        # self.setMinimumWidth(80)

        self.svgAirplaneWhite = self.mk_svg(rel_path("images/vehicle/airplane-white.svg"))
        self.svgAim = self.mk_svg(rel_path("images/vehicle/aim.svg"))
        self.svgCircle = self.mk_svg(rel_path("images/ehsi/ehsi_back.svg"))
        self.svgCross = self.mk_svg(rel_path("images/eadi/eadi_adi_fpmx.svg"))

        self.path = QPainterPath()

        # set black background
        pal = self.palette()
        pal.setColor(QPalette.Background, Qt.black)
        self.setAutoFillBackground(True)
        self.setPalette(pal)

        self._plane_phi = 0.0
        self._plane_r = 0.0
        self._plane_course = 0.0

        self._base_phi = 0.0
        self._base_r = 0.0

        self.layout.addWidget(self)

    def paintEvent(self, event):
        canvas = QPainter(self)
        canvas.setRenderHint(QPainter.Antialiasing)
        canvas.setBackgroundMode(Qt.BGMode.TransparentMode)

        self.svgAirplaneWhite.setRotation(self._plane_course)

        _plane_phi_rad = math.pi * (self._plane_phi + 90) / 180.0
        _base_phi_rad = math.pi * (self._base_phi + 90) / 180.0

        _plane_x = self._plane_r * math.sin(_plane_phi_rad)
        _plane_y = self._plane_r * math.cos(_plane_phi_rad)

        _base_x = self._base_r * math.sin(_base_phi_rad)
        _base_y = self._base_r * math.cos(_base_phi_rad)

        self.svgAirplaneWhite.setX(_plane_x)
        self.svgAirplaneWhite.setY(_plane_y)

        self.svgAim.setX(_base_x)
        self.svgAim.setY(_base_y)

        self.draw_item(canvas, self.svgCircle, width=90, height=90)
        self.draw_item(canvas, self.svgAim, width=30, height=30)
        self.draw_item(canvas, self.svgAirplaneWhite, width=30, height=30)

        canvas.setPen(QPen(Qt.green, 1, Qt.DashDotLine, Qt.RoundCap))
        canvas.drawLine(self.translate_point(canvas, QPointF(0, 0)),
                        self.translate_point(canvas, QPointF(_plane_x, _plane_y)))

        canvas.setPen(QPen(Qt.yellow, 1, Qt.DashDotLine, Qt.RoundCap))
        canvas.drawLine(self.translate_point(canvas, QPointF(0.0, 0.0)),
                        self.translate_point(canvas, QPointF(_base_x, _base_y)))

        canvas.end()

    def set_plane_phi(self, phi):
        self._plane_phi = phi

    def set_plane_r(self, r):
        self._plane_r = r

    def set_plane_course(self, angle):
        self._plane_course = angle

    def set_base_phi(self, phi):
        self._base_phi = phi

    def set_base_r(self, r):
        self._base_r = r

    def radraw_handler(self, vals: List[Union[float, int, str, NoDataStub]], vals_map: Dict):
        self.set_plane_phi(vals[0])
        self.set_plane_r(vals[1])
        self.set_plane_course(vals[2])

        self.set_base_phi(vals[3])
        self.set_base_r(vals[4])

        self.repaint()
