import math
import os.path
from abc import abstractmethod
from typing import List, Union, Dict, Tuple

import pyqtgraph as pg
from PyQt5 import Qt
from PyQt5.QtCore import Qt
from PyQt5.QtGui import QPen, QPainter, QFont, QPalette, QColor
from PyQt5.QtWidgets import QTableWidget, QTableWidgetItem, QHeaderView, QLabel

from ds import DataSourceBasic, NoDataStub
from .common import MyQtWidget, ColorInterp


def rel_path(path, base_path=None):
    if not base_path:
        base_path = os.path.dirname(__file__)
    return f'{base_path}/../{path}'


class EwBasic:
    def __init__(self):
        self.data_sources: List[DataSourceBasic] = []
        self.nested_widgets: List[EwBasic] = []

    def set_data_sources(self, data_sources: List[DataSourceBasic]):
        self.data_sources = data_sources

    def add_nested(self, w):
        self.nested_widgets.append(w)

    @abstractmethod
    def radraw_handler(self, vals: List[Union[float, int, str, NoDataStub]], vals_map: Dict):
        pass

    def redraw(self):
        for w in self.nested_widgets:
            w.redraw()

        vals = []
        vals_map = dict()

        for ds in self.data_sources:
            v = ds.read()
            vals.append(v)
            vals_map[ds.name] = v

        self.radraw_handler(vals, vals_map)


class EwGroup(MyQtWidget, EwBasic):
    def radraw_handler(self, vals: List[Union[float, int, str, NoDataStub]], vals_map: Dict):
        pass

    def __init__(self, widgets: List[EwBasic | MyQtWidget]):
        MyQtWidget.__init__(self, layout_vertical=False)
        EwBasic.__init__(self)

        self.group_widgets = widgets

        for w in self.group_widgets:
            self.nested_widgets.append(w)
            self.layout.addWidget(w)


class EwTable(MyQtWidget, EwBasic):
    def __init__(self, *, caption='', data_sources: List[DataSourceBasic], **kwargs):
        MyQtWidget.__init__(self, **kwargs)
        EwBasic.__init__(self)

        self.set_data_sources(data_sources)

        self.table = QTableWidget()

        self.table.setRowCount(len(data_sources))
        self.table.setColumnCount(2)
        # self.table.setColumnCount(3)

        active_color = [0, 100, 0, 255]
        idle_color = self.table.palette().color(QPalette.Base).getRgb()

        self.color_blenders: List[ColorInterp] = []
        for i in range(0, len(self.data_sources)):
            self.table.setItem(i, 0, QTableWidgetItem(data_sources[i].name))
            self.table.setItem(i, 1, QTableWidgetItem(''))
            self.color_blenders.append(ColorInterp(idle_color, active_color, 0.01))

        self.table.horizontalHeader().setStretchLastSection(True)
        self.table.horizontalHeader().setSectionResizeMode(QHeaderView.Stretch)
        self.table.horizontalHeader().setVisible(False)
        # self.table.verticalHeader().setSectionResizeMode(QHeaderView.Stretch)
        self.table.verticalHeader().setVisible(False)

        height = 0
        # height = self.table.horizontalScrollBar().height() + self.table.horizontalHeader().height()
        for i in range(self.table.rowCount()):
            height += self.table.verticalHeader().sectionSize(i)

        self.table.setMinimumHeight(height)

        if caption:
            self.layout.addWidget(QLabel(caption))

        self.layout.addWidget(self.table)

    def radraw_handler(self, vals: List[Union[float, int, str, NoDataStub]], vals_map: Dict):
        for i in range(0, len(self.data_sources)):
            value_item = self.table.item(i, 1)
            color_item = self.table.item(i, 1)
            prev_val = value_item.text()
            val = vals[i]

            if isinstance(val, float):
                val = '{:10.6f}'.format(val)

            new_val = str(val) if not isinstance(val, NoDataStub) else 'NO DATA'
            if prev_val == new_val:
                self.color_blenders[i].shift_to_left()
                color_item.setBackground(self.color_blenders[i].color_get())
            else:
                color_item.setBackground(self.color_blenders[i].set_right())

            value_item.setText(new_val)


class EwLamp(MyQtWidget, EwBasic):
    def __init__(self, *, data_source: DataSourceBasic, max, min=0, color, **kwargs):
        MyQtWidget.__init__(self, **kwargs)
        EwBasic.__init__(self)

        self.set_data_sources([data_source])
        self.setFixedSize(50, 50)

        self.max = max
        self.min = min

        self.k = 1 / (max - min)
        self.b = min

        max_color = [*list(color), 255]
        min_color = self.palette().color(QPalette.Base).getRgb()

        self.color_blender = ColorInterp(min_color, max_color, 0.01)

        self.value = 0.0
        self.not_valid = False

        self.layout.addWidget(self)

    def paintEvent(self, event):
        canvas = QPainter(self)

        if self.not_valid:
            pass
        else:
            v = self.k * self.value + self.b
            self.color_blender.set_lever(v)
            color = self.color_blender.color_get()
            # canvas.setBackground(QBrush(color))
            canvas.fillRect(event.rect(), color)

        canvas.end()

    def radraw_handler(self, vals: List[Union[float, int, str, NoDataStub]], vals_map: Dict):
        v = vals[0]

        if isinstance(v, NoDataStub):
            self.not_valid = True
        else:
            self.not_valid = False
            self.value = vals[0]

        self.repaint()


class EwChart(MyQtWidget, EwBasic):
    colors_series = [
        (255, 0, 0),
        (0, 200, 0),
        (0, 0, 255),
        (0, 200, 200),
        (200, 200, 0),
        (200, 0, 200),
    ]

    class Plot:
        def __init__(self, *, label, graph, color=None, window=600, **kwargs):
            self.label = label
            self.x = list(range(window))
            self.y = [0.0] * window

            self.parent_graph = graph

            if not color:
                color = EwChart.colors_series[0]

            pen = pg.mkPen(color=color, width=4)
            self.data_line = graph.plot(self.x, self.y, pen=pen)

            self.no_data_message_is_there = False
            self.no_data_message = pg.TextItem('', anchor=(0.5, 0.5), color=color)
            self.no_data_message.setPos(100, 100)
            # self.no_data_message.setZValue(50)
            self.no_data_message.setFont(QFont("Arial", 14))

        def update(self, val):
            if isinstance(val, NoDataStub):
                if not self.no_data_message_is_there:
                    self.no_data_message.setText(val.err_msg)
                    # self.parent_graph.addItem(self.no_data_message)
                    self.no_data_message_is_there = True
                val = 0.0
            else:
                if self.no_data_message_is_there:
                    # self.parent_graph.removeItem(self.no_data_message)
                    self.no_data_message_is_there = False

            self.x = self.x[1:]
            self.x.append(self.x[-1] + 1)

            self.y = self.y[1:]
            self.y.append(val)

            self.data_line.setData(self.x, self.y)

    def __init__(self, data_sources: List[DataSourceBasic], data_range=None, **kwargs):
        MyQtWidget.__init__(self, **kwargs)
        EwBasic.__init__(self)

        self.set_data_sources(data_sources)

        self.graph = pg.PlotWidget(**kwargs)

        if data_range:
            self.graph.setYRange(data_range[0], data_range[1])

        self.layout.addWidget(self.graph)

        # window = 600
        # self.graph.setBackground(QtGui.QColor('white'))

        self.plots: List[EwChart.Plot] = []

        color_i = 0
        for ds in data_sources:
            self.plots.append(EwChart.Plot(label=ds.name, graph=self.graph, color=self.colors_series[color_i]))
            if color_i < len(self.colors_series):
                color_i += 1

    def radraw_handler(self, vals: List[Union[float, int, str, NoDataStub]], vals_map: Dict):
        for i in range(0, len(self.plots)):
            self.plots[i].update(vals[i])


class EwCursor(MyQtWidget, EwBasic):
    colors_series = [
        (255, 0, 0),
        (0, 200, 0),
        (0, 0, 255),
        (0, 0, 0),
    ]

    class Cursor:
        def __init__(self, *, parent, color=None, tail_len=100, **kwargs):
            self.parent = parent
            self.data = [(0.0, 0.0)]
            self.tail_len = tail_len
            if not color:
                color = EwChart.colors_series[0]

            self.color = color

        margin = 10

        def rel_x(self, x_val):
            return self.margin + (x_val - self.parent.data_range[0][0]) * (
                    self.parent.width() - 2 * self.margin) / self.parent.x_range_mod

        def rel_y(self, y_val):
            return self.margin + (y_val - self.parent.data_range[1][0]) * (
                    self.parent.height() - 2 * self.margin) / self.parent.y_range_mod

        def update_data(self, val: tuple):
            if isinstance(val[0], NoDataStub) or isinstance(val[1], NoDataStub):
                return

            if self.data[-1] != val:
                self.data.append(val)

            if len(self.data) > self.tail_len:
                self.data = self.data[1:]

        def redraw(self, painter):
            b = len(self.data)
            for i in range(0, b):
                if i == b - 1:
                    alpha = 255
                    thickness = 2
                    d = 8
                else:
                    alpha = int(i * 100.0 / b)
                    thickness = 1
                    d = 2

                painter.setPen(
                    QPen(QColor(self.color[0], self.color[1], self.color[2], alpha), thickness, Qt.SolidLine))

                x = self.rel_x(self.data[i][0])
                y = self.rel_y(self.data[i][1])
                d_2 = d / 2
                painter.drawEllipse(int(x - d_2), int(y - d_2), int(d), int(d))

    def __init__(self, data_sources: List[Tuple[DataSourceBasic, DataSourceBasic]],
                 data_range=((-1.0, 1.0), (-1.0, 1.0)), *args, **kwargs):
        MyQtWidget.__init__(self, **kwargs)
        EwBasic.__init__(self)

        data_sources_list = [elem for t in data_sources for elem in t]
        self.set_data_sources(data_sources_list)

        self.setFixedSize(180, 180)
        # self.setMinimumHeight(80)
        # self.setMinimumWidth(80)

        self.layout.addWidget(self)

        self.data_range = data_range
        self.x_range_mod = data_range[0][1] - data_range[0][0]
        self.y_range_mod = data_range[1][1] - data_range[1][0]

        self.cursors: List[EwCursor.Cursor] = []

        color_i = 0
        for ds in data_sources:
            self.cursors.append(EwCursor.Cursor(parent=self, color=self.colors_series[color_i]))
            if color_i < len(self.colors_series):
                color_i += 1

    def paintEvent(self, event):
        canvas = QPainter(self)

        for c in self.cursors:
            c.redraw(canvas)

        canvas.end()

    def radraw_handler(self, vals: List[Union[float, int, str, NoDataStub]], vals_map: Dict):
        for i in range(0, len(self.cursors)):
            self.cursors[i].update_data((vals[i * 2], vals[i * 2 + 1]))

        self.repaint()


class EwHeadingIndicator(MyQtWidget, EwBasic):
    # SAMPLE_TODO copy and paste this class , rename properly

    def __init__(self, data_sources: List[DataSourceBasic], **kwargs):
        MyQtWidget.__init__(self, **kwargs)
        EwBasic.__init__(self)

        # SAMPLE_TODO pass list of data to
        self.set_data_sources(data_sources)

        # SAMPLE_TODO define widget size policy
        self.setFixedSize(180, 180)
        # self.setMinimumHeight(80)
        # self.setMinimumWidth(80)

        self.svgBack = self.mk_svg(rel_path("images/hi/hi_face.svg"))
        self.svgFace = self.mk_svg(rel_path("images/hi/hi_case.svg"))
        self._heading = 0.0
        self.layout.addWidget(self)

    def paintEvent(self, event):
        canvas = QPainter(self)
        self.svgFace.setRotation(self._heading)
        self.draw_item(canvas, self.svgBack)
        self.draw_item(canvas, self.svgFace)
        canvas.end()

    def set_heading(self, h):
        if isinstance(h, NoDataStub):
            self._heading = 0
            return

        self._heading = h

    def radraw_handler(self, vals: List[Union[float, int, str, NoDataStub]], vals_map: Dict):
        self.set_heading(vals[0])
        self.repaint()


class EwAttitudeIndicator(MyQtWidget, EwBasic):
    def __init__(self, data_sources: List[DataSourceBasic], **kwargs):
        MyQtWidget.__init__(self, **kwargs)
        EwBasic.__init__(self)

        self.set_data_sources(data_sources)
        self.setFixedSize(180, 180)

        self.svgBack = self.mk_svg(rel_path("images/ai/ai_back.svg"), -30)
        self.svgFace = self.mk_svg(rel_path("images/ai/ai_face.svg"))
        self.svgRing = self.mk_svg(rel_path("images/ai/ai_ring.svg"))
        self.svgCase = self.mk_svg(rel_path("images/ai/ai_case.svg"))

        self._roll = 0.0
        self._pitch = 0.0
        self._faceDeltaX_old = 0.0
        self._faceDeltaY_old = 0.0

        self.reset()

        self.layout.addWidget(self)

    def set_roll(self, roll):
        if isinstance(roll, NoDataStub):
            self._roll = 0
            return

        self._roll = roll

        if self._roll < -180.0:
            self._roll = -180.0

        if self._roll > 180.0:
            self._roll = 180.0

    def set_pitch(self, pitch):
        if isinstance(pitch, NoDataStub):
            self._pitch = 90
            return

        self._pitch = pitch

        if self._pitch < -25.0:
            self._pitch = -25.0

        if self._pitch > 25.0:
            self._pitch = 25.0

    def reset(self):
        self._roll = 0.0
        self._pitch = 0.0
        self._faceDeltaX_old = 0.0
        self._faceDeltaY_old = 0.0

    def paintEvent(self, event):
        canvas = QPainter(self)

        _width = canvas.device().width()
        _height = canvas.device().height()

        _br = self.svgFace.boundingRect()
        _originalWidth = _br.width()
        _originalHeight = _br.height()

        _scaleX = _width / _originalWidth
        _scaleY = _height / _originalHeight

        roll_rad = math.pi * self._roll / 180.0
        delta = 1.7 * self._pitch

        _faceDeltaX_new = _scaleX * delta * math.sin(roll_rad)
        _faceDeltaY_new = _scaleY * delta * math.cos(roll_rad)

        offs_x = _faceDeltaX_new - self._faceDeltaX_old
        offs_y = _faceDeltaY_new - self._faceDeltaY_old

        self.svgBack.setRotation(-self._roll)
        self.svgFace.setRotation(-self._roll)
        self.svgRing.setRotation(-self._roll)
        self.svgFace.moveBy(offs_x, offs_y)

        self.draw_item(canvas, self.svgBack)
        self.draw_item(canvas, self.svgFace)
        self.draw_item(canvas, self.svgRing)
        self.draw_item(canvas, self.svgCase)

        self._faceDeltaX_old = _faceDeltaX_new
        self._faceDeltaY_old = _faceDeltaY_new

        canvas.end()

    def radraw_handler(self, vals: List[Union[float, int, str, NoDataStub]], vals_map: Dict):
        self.set_roll(vals[0])
        self.set_pitch(vals[1])
        self.repaint()
