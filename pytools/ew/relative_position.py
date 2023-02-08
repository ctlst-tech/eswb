import math
from typing import List, Union, Dict

from PyQt5 import QtGui
from PyQt5.QtCore import Qt, QPointF, pyqtSignal
from PyQt5.QtGui import QPainter, QPen, QPixmap, QBrush, QFont

from ds import DataSourceBasic, NoDataStub
from .common import MyQtWidget, rel_path
from .widgets import EwBasic

max_meter_in_px = 5
min_meter_in_px = 0.01
zoom_step = 0.05
ds_tuple_size = 5


def to_polar(x, y, scale=1.0):
    pt = QPointF(x, y)
    r = math.sqrt(pt.x() * pt.x() + pt.y() * pt.y()) / scale
    degrees = math.degrees(math.atan2(-y, x)) + 90
    phi = (degrees + 360) % 360
    return [round(phi, 2), round(r, 3)]


def to_cartesian(r, phi, scale=1.0):
    _phi_rad = math.radians(phi)
    _x = scale * r * math.sin(_phi_rad)
    _y = -(scale * r * math.cos(_phi_rad))
    return [_x, _y]


def rtranslate_point(width, height, pt: QPointF, zero_offset_x=0.0, zero_offset_y=0.0):
    dx, dy = width / 2 + zero_offset_x, height / 2 + zero_offset_y
    return QPointF(pt.x() - dx, - (pt.y() - dy))


def translate_point(width, height, pt: QPointF, zero_offset_x=0.0, zero_offset_y=0.0):
    dx, dy = width / 2 + zero_offset_x, height / 2 + zero_offset_y
    return QPointF(pt.x() + dx, pt.y() + dy)


class EwRelativePosition(MyQtWidget, EwBasic):
    target_changed = pyqtSignal(float, float, name='targetChanged')
    scale_changed = pyqtSignal(float, name='zoomChanged')

    rings = [10, 20, 50, 100, 150, 200, 300, 500]

    class Marker:
        def __init__(self, icon='aim-yellow', custom_icon_path=None):
            icon_path = rel_path(f"images/vehicle/{icon}.svg") if not custom_icon_path else custom_icon_path
            self.svg_icon = EwRelativePosition.mk_svg(icon_path)
            self.phi = 0.0
            self.r = 0.0
            self.alt = 0.0
            self.azimuth = 0.0

        def draw(self, canvas, center, scale):
            [_x, _y] = to_cartesian(self.r, self.phi, scale)
            self.svg_icon.setX(_x + center[0])
            self.svg_icon.setY(_y + center[1])
            self.svg_icon.setRotation(self.azimuth)
            canvas.setPen(QPen(Qt.darkGray, 1, Qt.DotLine, Qt.RoundCap))
            canvas.drawLine(translate_point(canvas.device().width(), canvas.device().height(),
                                            QPointF(0, 0),
                                            center[0], center[1]),
                            translate_point(canvas.device().width(), canvas.device().height(),
                                            QPointF(_x, _y),
                                            center[0], center[1]))

    def __init__(self, data_sources: List[DataSourceBasic], fixed_size=None, **kwargs):
        MyQtWidget.__init__(self, **kwargs)
        EwBasic.__init__(self)

        self.scale_m = 1
        self.markers = []
        self.center = [0.0, 0.0]

        self.set_data_sources(data_sources)
        if fixed_size:
            self.setFixedSize(fixed_size[0], fixed_size[1])

        self.add_markers(data_sources)
        self.layout.addWidget(self)

    def add_markers(self, data_sources):
        count = len(data_sources) // ds_tuple_size
        for i in range(0, count):
            icon = data_sources[i * ds_tuple_size].read()
            if '/' in icon:
                self.markers.append(self.Marker(custom_icon_path=icon))
            else:
                self.markers.append(self.Marker(icon=icon))

    def set_scale(self, scale):
        """  pixels per meter """
        self.scale_m = scale
        # noinspection PyUnresolvedReferences
        self.scale_changed.emit(scale)

    def zoom_in(self):
        # print("[in] meters in px: ", 1 / self.scale_m)
        max_factor = (1 / min_meter_in_px)
        if self.scale_m <= max_factor:
            v = round(self.scale_m + zoom_step, 2)
            self.set_scale(min(v, max_factor))

    def zoom_out(self):
        # print("[out] meters in px: ", 1 / self.scale_m)
        min_factor = (1 / max_meter_in_px)
        if self.scale_m >= min_factor:
            v = round(self.scale_m - zoom_step, 2)
            self.set_scale(max(v, min_factor))

    def wheelEvent(self, event: QtGui.QWheelEvent) -> None:
        delta = event.angleDelta().y()
        if delta > 0:
            self.zoom_in()

        elif delta < 0:
            self.zoom_out()

    def _emit_target_chg(self, x, y):
        pt = rtranslate_point(self.rect().width(), self.rect().height(), QPointF(x, y), self.center[0], self.center[1])
        [phi, r] = to_polar(pt.x(), pt.y(), scale=self.scale_m)
        # noinspection PyUnresolvedReferences
        self.target_changed.emit(phi, r)

    def mousePressEvent(self, event: QtGui.QMouseEvent) -> None:
        x, y = event.x(), event.y()
        self._emit_target_chg(x, y)

    def mouseMoveEvent(self, event: QtGui.QMouseEvent) -> None:
        if event.buttons() == Qt.LeftButton:
            x, y = event.x(), event.y()
            self._emit_target_chg(x, y)

    def draw_markers(self, canvas):
        for m in self.markers:
            m.draw(canvas, self.center, self.scale_m)
            self.draw_item(canvas, m.svg_icon, width=30, height=30)

    def draw_rings(self, canvas):
        for r in self.rings:
            sr = r * self.scale_m
            canvas.drawEllipse(
                translate_point(canvas.device().width(), canvas.device().height(),
                                QPointF(0.0, 0.0), self.center[0],
                                self.center[1]), sr, sr)
            canvas.drawText(
                translate_point(canvas.device().width(), canvas.device().height(),
                                QPointF(sr - 2, 0.0), self.center[0],
                                self.center[1]), f"{r}m")

    def paint_bitmap(self):
        buffer = QPixmap(self.width(), self.height())
        canvas = QPainter(buffer)
        canvas.setRenderHint(QPainter.Antialiasing)
        canvas.setBackgroundMode(Qt.BGMode.OpaqueMode)
        canvas.setBackground(Qt.black)
        canvas.setBrush(QBrush(Qt.black))
        canvas.drawRect(self.rect())

        canvas.setBrush(QBrush(Qt.transparent))
        canvas.setPen(QPen(Qt.darkGray, 0.5, Qt.DashDotLine, Qt.RoundCap))
        canvas.setFont(QFont("Arial", 8, QFont.Bold))

        self.draw_rings(canvas)
        self.draw_markers(canvas)

        canvas.end()
        return buffer

    def paintEvent(self, event):
        buffer = self.paint_bitmap()
        canvas = QPainter(self)
        canvas.drawPixmap(0, 0, buffer)
        canvas.end()

    def radraw_handler(self, vals: List[Union[float, int, str, NoDataStub]], vals_map: Dict):
        count = len(vals) // ds_tuple_size
        def check_stub(d):
            return d if not isinstance(d, NoDataStub) else 0.0

        for i in range(0, count):
            idx = i * ds_tuple_size
            self.markers[i].phi = check_stub(vals[idx + 1])
            self.markers[i].r = check_stub(vals[idx + 2])
            self.markers[i].alt = check_stub(vals[idx + 3])
            self.markers[i].azimuth = check_stub(vals[idx + 4])

        self.repaint()


class EwRelativeAltitide(EwRelativePosition):
    class AltMarker(EwRelativePosition.Marker):
        def __init__(self, icon='aim-yellow'):
            if not icon.startswith("aim"):
                icon = 'aim-yellow'
            super().__init__(icon=icon)

        def draw(self, canvas, center, scale):
            [_x, _y] = to_cartesian(self.r, self.phi, scale)
            __y = -self.alt * scale
            self.svg_icon.setX(_x + center[0])
            self.svg_icon.setY(__y + center[1])
            canvas.setPen(QPen(Qt.darkGray, 1, Qt.DotLine, Qt.RoundCap))
            canvas.drawLine(translate_point(canvas.device().width(), canvas.device().height(),
                                            QPointF(_x, 0),
                                            center[0], center[1]),
                            translate_point(canvas.device().width(), canvas.device().height(),
                                            QPointF(_x, __y),
                                            center[0], center[1]))

    def __init__(self, data_sources: List[DataSourceBasic], **kwargs):
        super().__init__(data_sources, **kwargs)
        self.center = [0.0, self.height() / 2]

    def add_markers(self, data_sources):
        count = len(data_sources) // ds_tuple_size
        for i in range(0, count):
            icon = data_sources[i * ds_tuple_size].read()
            self.markers.append(self.AltMarker(icon=icon))

    def draw_rings(self, canvas):
        for r in self.rings:
            _y = r * self.scale_m
            pt = translate_point(canvas.device().width(),
                                 canvas.device().height(),
                                 QPointF(0, 0),
                                 self.center[0],
                                 self.center[1])
            canvas.drawLine(0, int(pt.y() - _y),
                            canvas.device().width(), int(pt.y() - _y))
            canvas.drawText(0, int(pt.y() - _y), f"{r}m")

    def paintEvent(self, event):
        buffer = self.paint_bitmap()
        self.center = [0.0, self.height() / 2]
        canvas = QPainter(self)
        canvas.drawPixmap(0, 0, buffer)
        canvas.end()
