import math
from typing import List, Union, Dict

from PyQt5 import QtGui
from PyQt5.QtCore import Qt, QPointF, pyqtSignal
from PyQt5.QtGui import QPainter, QPalette, QPainterPath, QPen, QPixmap, QImage, QBrush, QFont

from ds import DataSourceBasic, NoDataStub

from .common import MyQtWidget, rel_path
from .widgets import EwBasic


class EwRelativePosition(MyQtWidget, EwBasic):
    def __init__(self, data_sources: List[DataSourceBasic], fixed_size=None, **kwargs):
        MyQtWidget.__init__(self, **kwargs)
        EwBasic.__init__(self)

        self.scale_m = 1

        self.set_data_sources(data_sources)
        if fixed_size:
            self.setFixedSize(fixed_size[0], fixed_size[1])

        self.svg_plane_white = self.mk_svg(rel_path("images/vehicle/airplane-white.svg"))
        self.svg_circle = self.mk_svg(rel_path("images/ehsi/ehsi_back.svg"))
        self.svg_aim_green = self.mk_svg(rel_path("images/vehicle/aim-green.svg"))
        self.svg_aim_yellow = self.mk_svg(rel_path("images/vehicle/aim-yellow.svg"))

        self.path = QPainterPath()

        # set black background
        # pal = self.palette()
        # pal.setColor(QPalette.Background, Qt.black)
        # self.setAutoFillBackground(True)
        # self.setPalette(pal)

        self._plane_phi = 0.0
        self._plane_r = 0.0
        self._plane_course = 0.0

        self._base_phi = 0.0
        self._base_r = 0.0

        self._target_phi = 0.0
        self._target_r = 0.0

        self.layout.addWidget(self)

    def set_scale(self, scale):
        """  pixels per meter """
        self.scale_m = scale

    def to_abs(self, r, phi):
        _phi_rad = math.pi * (phi + 90) / 180.0
        _x = self.scale_m * r * math.sin(_phi_rad)
        _y = self.scale_m * r * math.cos(_phi_rad)
        return [_x, _y]

    max_meter_in_px = 5
    min_meter_in_px = 0.01
    zoom_step = 0.05

    def zoom_in(self):
        # print("[in] meters in px: ", 1 / self.scale_m)
        max_factor = (1 / self.min_meter_in_px)
        if self.scale_m <= max_factor:
            v = round(self.scale_m + self.zoom_step, 2)
            self.set_scale(min(v, max_factor))

    def zoom_out(self):
        # print("[out] meters in px: ", 1 / self.scale_m)
        min_factor = (1 / self.max_meter_in_px)
        if self.scale_m >= min_factor:
            v = round(self.scale_m - self.zoom_step, 2)
            self.set_scale(max(v, min_factor))

    def wheelEvent(self, event: QtGui.QWheelEvent) -> None:
        delta = event.angleDelta().y()
        if delta > 0:
            self.zoom_in()

        elif delta < 0:
            self.zoom_out()

    def paint_bitmap(self):
        buffer = QPixmap(self.width(), self.height())
        canvas = QPainter(buffer)
        canvas.setRenderHint(QPainter.Antialiasing)
        canvas.setBackgroundMode(Qt.BGMode.OpaqueMode)
        canvas.setBackground(Qt.black)
        canvas.setBrush(QBrush(Qt.black))
        canvas.drawRect(self.rect())

        # center
        # self.draw_item(canvas, self.svg_circle, width=90, height=90)

        canvas.setBrush(QBrush(Qt.transparent))
        canvas.setPen(QPen(Qt.darkGray, 0.5, Qt.DashDotLine, Qt.RoundCap))
        canvas.setFont(QFont("Arial", 8, QFont.Bold))
        for r in [10, 20, 50, 100, 150, 200, 300, 500]:
            sr = r * self.scale_m
            canvas.drawEllipse(self.translate_point(canvas, QPointF(0.0, 0.0)), sr, sr)
            canvas.drawText(self.translate_point(canvas, QPointF(sr - 2, 0.0)), f"{r}m")


        [_plane_x, _plane_y] = self.to_abs(self._plane_r, self._plane_phi)
        self.svg_plane_white.setX(_plane_x)
        self.svg_plane_white.setY(_plane_y)
        self.svg_plane_white.setRotation(self._plane_course)
        self.draw_item(canvas, self.svg_plane_white, width=30, height=30)

        canvas.setPen(QPen(Qt.blue, 1, Qt.DotLine, Qt.RoundCap))
        canvas.drawLine(self.translate_point(canvas, QPointF(0, 0)),
                        self.translate_point(canvas, QPointF(_plane_x, _plane_y)))

        [_base_x, _base_y] = self.to_abs(self._base_r, self._base_phi)
        self.svg_aim_yellow.setX(_base_x)
        self.svg_aim_yellow.setY(_base_y)
        self.draw_item(canvas, self.svg_aim_yellow, width=30, height=30)

        canvas.setPen(QPen(Qt.yellow, 1, Qt.DotLine, Qt.RoundCap))
        canvas.drawLine(self.translate_point(canvas, QPointF(0.0, 0.0)),
                        self.translate_point(canvas, QPointF(_base_x, _base_y)))

        [_target_x, _target_y] = self.to_abs(self._target_r, self._target_phi)
        self.svg_aim_green.setX(_target_x)
        self.svg_aim_green.setY(_target_y)
        self.draw_item(canvas, self.svg_aim_green, width=30, height=30)

        canvas.setPen(QPen(Qt.green, 1, Qt.DotLine, Qt.RoundCap))
        canvas.drawLine(self.translate_point(canvas, QPointF(0.0, 0.0)),
                        self.translate_point(canvas, QPointF(_target_x, _target_y)))

        canvas.end()
        return buffer

    def paintEvent(self, event):
        buffer = self.paint_bitmap()
        canvas = QPainter(self)
        canvas.drawPixmap(0, 0, buffer)
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

    def set_target_phi(self, phi):
        self._target_phi = phi

    def set_target_r(self, r):
        self._target_r = r

    def radraw_handler(self, vals: List[Union[float, int, str, NoDataStub]], vals_map: Dict):
        self.set_plane_phi(vals[0])
        self.set_plane_r(vals[1])
        self.set_plane_course(vals[2])

        self.set_base_phi(vals[3])
        self.set_base_r(vals[4])

        self.set_target_phi(vals[5])
        self.set_target_r(vals[6])

        self.repaint()
