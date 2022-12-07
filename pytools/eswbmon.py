#!/usr/bin/env python3
import time
# TODO https://github.com/pyqt/examples

from abc import abstractmethod
from time import sleep
from typing import List, Union, Tuple
import sys
import math

from PyQt5 import QtWidgets, QtCore
from PyQt5.QtCore import Qt

import pyqtgraph as pg
from random import randint

from PyQt5.QtGui import QPainter, QPen
from PyQt5.QtWidgets import QTableWidget, QTableWidgetItem, QHeaderView, QLabel
from pyqtgraph.Qt import QtGui

import eswb as e
import os



class NoDataStub:
    def __init__(self, err_msg):
        self.err_msg = err_msg


class DataSourceBasic:
    def __init__(self, name, *, mult=1.0, range=None, demo_mode=False):
        self.name = name
        self.range = range
        self.mult = mult
        self.demo_mode = demo_mode

    @abstractmethod
    def connect(self):
        pass

    @abstractmethod
    def read(self) -> Union[float, int, str, NoDataStub]:
        pass


class DataSourceSum(DataSourceBasic):
    def __init__(self, name, sources: List[DataSourceBasic], **kwargs):
        super().__init__(name, **kwargs)
        self.data_sources = sources

    def connect(self):
        for s in self.data_sources:
            s.connect()

    def read(self) -> Union[float, int, str, NoDataStub]:
        rv = 0
        for s in self.data_sources:
            v = s.read()
            if isinstance(v, NoDataStub):
                return v
            else:
                rv += v

        return rv


class DataSourceCalcFilteredRate(DataSourceBasic):
    def __init__(self, name, data_source: DataSourceBasic, **kwargs):
        super().__init__(name, **kwargs)
        self.data_source = data_source
        self.initial = False
        self.filtered_value = 0
        self.previous_value = 0
        self.previous_time = 0

    def connect(self):
        self.data_source.connect()

    def read(self) -> Union[float, int, str, NoDataStub]:
        current_value = self.data_source.read()

        if isinstance(current_value, NoDataStub):
            return current_value

        curr_time = time.time()

        if not self.initial:
            self.initial = True
            self.filtered_value = 0
        else:
            delta_time = curr_time - self.previous_time
            deriative = (current_value - self.previous_value) / delta_time
            self.filtered_value = self.filtered_value - 0.01 * (self.filtered_value - deriative)

        self.previous_value = current_value
        self.previous_time = curr_time

        return self.filtered_value


class DataSourceSinus(DataSourceBasic):
    def __init__(self, name, *, omega=1.0, iphase=0.0, **kwargs):
        super().__init__(name, **kwargs)
        self.time = 0
        self.delta_time = 0.01
        self.omega = omega
        self.initial_phase = iphase

    def connect(self):
        pass

    def read(self) -> Union[float, int, str, NoDataStub]:
        self.time += self.delta_time
        return self.mult * math.sin(self.initial_phase + self.omega * self.time)


class DataSourceEswbTopic(DataSourceBasic):

    def __init__(self, name, path, **kwargs):
        super().__init__(name, **kwargs)
        self.topic_handle: e.TopicHandle = e.TopicHandle(name, path)

    def connect(self):
        self.topic_handle.connect()

    def read(self):
        try:
            value = self.mult * self.topic_handle.value()
        except Exception as E:
            value = NoDataStub(E.args[0])
            try:
                self.topic_handle.connect()
            except Exception as E:
                # redefine message
                value.err_msg = E.args[0]

        return value


class myQtWidget(QtWidgets.QWidget):
    def __init__(self, layout_vertical=True, **kwargs):
        super().__init__(**kwargs)
        self.layout = QtWidgets.QBoxLayout(QtWidgets.QBoxLayout.TopToBottom if layout_vertical else QtWidgets.QBoxLayout.LeftToRight)
        self.setLayout(self.layout)


class ewBasic:
    def __init__(self):
        self.data_sources: List[DataSourceBasic] = []
        self.nested_widgets: List[ewBasic] = []

    def set_data_sources(self, data_sources: List[DataSourceBasic]):
        self.data_sources = data_sources

    def add_nested(self, w):
        self.nested_widgets.append(w)

    @abstractmethod
    def radraw_handler(self, vals: List[Union[float, int, str, NoDataStub]]):
        pass

    def redraw(self):
        for w in self.nested_widgets:
            w.redraw()

        vals = []
        for ds in self.data_sources:
            vals.append(ds.read())

        self.radraw_handler(vals)


class ewTable(myQtWidget, ewBasic):
    def __init__(self, *, caption='', data_sources: List[DataSourceBasic], **kwargs):
        myQtWidget.__init__(self, **kwargs)
        ewBasic.__init__(self)

        self.set_data_sources(data_sources)

        self.table = QTableWidget()

        self.table.setRowCount(len(data_sources))
        self.table.setColumnCount(2)
        # self.table.setColumnCount(3)

        active_color = [0, 100, 0, 255]
        idle_color = self.table.palette().color(QtGui.QPalette.Base).getRgb()

        self.color_blenders: List[colorInterp] = []
        for i in range(0, len(self.data_sources)):
            self.table.setItem(i, 0, QTableWidgetItem(data_sources[i].name))
            self.table.setItem(i, 1, QTableWidgetItem(''))
            self.color_blenders.append(colorInterp(idle_color, active_color, 0.01))


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



    def radraw_handler(self, vals: List[Union[float, int, str, NoDataStub]]):
        for i in range(0, len(self.data_sources)):
            value_item = self.table.item(i, 1)
            color_item = self.table.item(i, 1)
            prev_val = value_item.text()
            new_val = str(vals[i])
            if prev_val == new_val:
                self.color_blenders[i].shift_to_left()
                color_item.setBackground(self.color_blenders[i].color_get())
            else:
                color_item.setBackground(self.color_blenders[i].set_right())

            value_item.setText(new_val)


class colorInterp:
    def __init__(self, c0, c1, step=0.01):
        self.left = c0
        self.right = c1
        self.current = [0.0, 0.0, 0.0, 0.0]
        self.mixture_lever = 0.0
        self.step = step

        self.color_delta = [c1[0] - c0[0],
                          c1[1] - c0[1],
                          c1[2] - c0[2],
                          c1[3] - c0[3]
                            ]

    @staticmethod
    def qcolor(clr):
        return QtGui.QColor(QtGui.qRgba(
            int(clr[0]),
            int(clr[1]),
            int(clr[2]),
            int(clr[3])))

    def color_get(self):
        self.current[0] = self.left[0] + self.color_delta[0] * self.mixture_lever
        self.current[1] = self.left[1] + self.color_delta[1] * self.mixture_lever
        self.current[2] = self.left[2] + self.color_delta[2] * self.mixture_lever
        self.current[3] = self.left[3] + self.color_delta[3] * self.mixture_lever
        return self.qcolor(self.current)

    def shift_to_right(self):
        self.mixture_lever += self.step
        self.mixture_lever = 1.0 if self.mixture_lever > 1.0 else self.mixture_lever

    def shift_to_left(self):
        self.mixture_lever -= self.step
        self.mixture_lever = 0.0 if self.mixture_lever < 0.0 else self.mixture_lever

    def set_left(self):
        self.mixture_lever = 0.0
        return self.color_get()

    def set_right(self):
        self.mixture_lever = 1.0
        return self.color_get()


class ewChart(myQtWidget, ewBasic):

    colors_series = [
        (255, 0, 0),
        (0, 200, 0),
        (0, 0, 255),
        (0, 0, 0),
    ]

    class Plot:
        def __init__(self, *, label, graph, color=None, window=600, **kwargs):
            self.label = label
            self.x = list(range(window))
            self.y = [0.0] * window

            self.parent_graph = graph

            if not color:
                color = ewChart.colors_series[0]

            pen = pg.mkPen(color=color, width=4)
            self.data_line = graph.plot(self.x, self.y, pen=pen)

            self.no_data_message_is_there = False
            self.no_data_message = pg.TextItem('', anchor=(0.5, 0.5), color=color)
            self.no_data_message.setPos(100, 100)
            # self.no_data_message.setZValue(50)
            self.no_data_message.setFont(QtGui.QFont("Arial", 14))

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
        myQtWidget.__init__(self, **kwargs)
        ewBasic.__init__(self)

        self.set_data_sources(data_sources)

        self.graph = pg.PlotWidget(**kwargs)

        if data_range:
            self.graph.setYRange(data_range[0], data_range[1])

        self.layout.addWidget(self.graph)

        # window = 600
        # self.graph.setBackground(QtGui.QColor('white'))

        self.plots: List[ewChart.Plot] = []

        color_i = 0
        for ds in data_sources:
            self.plots.append(ewChart.Plot(label=ds.name, graph=self.graph, color=self.colors_series[color_i]))
            if color_i < len(self.colors_series):
                color_i += 1

    def radraw_handler(self, vals: List[Union[float, int, str, NoDataStub]]):
        for i in range(0, len(self.plots)):
            self.plots[i].update(vals[i])


class ewCursor(myQtWidget, ewBasic):
    colors_series=[
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
                color = ewChart.colors_series[0]

            self.color = color

        margin = 10

        def rel_x(self, x_val):
            return self.margin + (x_val - self.parent.data_range[0][0]) * (self.parent.width() - 2*self.margin) / self.parent.x_range_mod

        def rel_y(self, y_val):
            return self.margin + (y_val - self.parent.data_range[1][0]) * (self.parent.height() - 2*self.margin) / self.parent.y_range_mod

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
                if i == b-1:
                    alpha = 255
                    thickness = 2
                else:
                    alpha = int(i * 100.0 / b)
                    thickness = 1

                painter.setPen(
                    QPen(QtGui.QColor(self.color[0], self.color[1], self.color[2], alpha), thickness, Qt.SolidLine))

                x = self.rel_x(self.data[i][0])
                y = self.rel_y(self.data[i][1])
                d = 8
                d_2 = d/2
                painter.drawEllipse(int(x - d_2), int(y - d_2), int(d), int(d))

    def __init__(self, data_sources: List[Tuple[DataSourceBasic, DataSourceBasic]], data_range=((-1.0, 1.0), (-1.0, 1.0)), *args, **kwargs):
        myQtWidget.__init__(self, **kwargs)
        ewBasic.__init__(self)

        data_sources_list = [ elem for t in data_sources for elem in t]
        self.set_data_sources(data_sources_list)

        self.setFixedSize(180, 180)
        # self.setMinimumHeight(80)
        # self.setMinimumWidth(80)

        self.layout.addWidget(self, stretch=1)

        self.data_range = data_range
        self.x_range_mod = data_range[0][1] - data_range[0][0]
        self.y_range_mod = data_range[1][1] - data_range[1][0]

        self.cursors: List[ewChart.Cursor] = []

        color_i = 0
        for ds in data_sources:
            self.cursors.append(ewCursor.Cursor(parent=self, color=self.colors_series[color_i]))
            if color_i < len(self.colors_series):
                color_i += 1

    def paintEvent(self, QPaintEvent):
        canvas = QPainter(self)

        for c in self.cursors:
            c.redraw(canvas)

        canvas.end()

    def radraw_handler(self, vals: List[Union[float, int, str, NoDataStub]]):
        for i in range(0, len(self.cursors)):
            self.cursors[i].update_data((vals[i*2], vals[i*2 + 1]))

        self.repaint()


class ApplicationWindow(QtWidgets.QMainWindow):
    def __init__(self, bus: e.Bus = None):
        super().__init__()

        self.widgets = []

        # self.setAttribute(QtCore.Qt.WA_DeleteOnClose)
        self.setWindowTitle("ESWB display")

        self.main_widget = QtWidgets.QWidget(self)
        self.main_widget.setFocus()
        self.setCentralWidget(self.main_widget)

        self.main_layout = QtWidgets.QVBoxLayout(self.main_widget)

        self.timer = QtCore.QTimer()
        self.timer.setInterval(20)
        self.timer.timeout.connect(self.redraw)
        self.timer.start()

        self.bus = bus
        if self.bus:
            self.timer_print_bus = QtCore.QTimer()
            self.timer_print_bus.setInterval(200)
            self.timer_print_bus.timeout.connect(self.print_bus_tree)
            self.timer_print_bus.start()

    def add_ew(self, widget):
        self.main_layout.addWidget(widget)
        self.widgets.append(widget)

    def connect(self):
        for w in self.widgets:
            try:
                w.connect()
            except:
                pass

    def print_bus_tree(self):
        self.bus.update_tree()
        self.bus.topic_tree.print()

    def redraw(self):
        for w in self.widgets:
            w.redraw()


def find_data_source(lst: List[DataSourceBasic], name: str):
    for s in lst:
        if s.name == name:
            return s

    return None


class SdtlTelemetryWidget(myQtWidget, ewBasic):
    def __init__(self, sdtl_ref: e.SDTLserialService):
        myQtWidget.__init__(self, layout_vertical=False)
        ewBasic.__init__(self)

        data_service_rx_stat = [DataSourceEswbTopic(name=t.name, path=t.get_path()) for t in sdtl_ref.stat_topics_service_tree.children]

        bytes_receive_source = find_data_source(data_service_rx_stat, 'bytes_received')

        data_service_channels_stat = {}
        for k in sdtl_ref.stat_topics_channels_trees.keys():
            data_service_channels_stat[k] = \
                [DataSourceEswbTopic(name=f'rx_{t.name}', path=t.get_path()) for t in sdtl_ref.stat_topics_channels_trees[k]['rx'].children]
            data_service_channels_stat[k] += \
                [DataSourceEswbTopic(name=f'tx_{t.name}', path=t.get_path()) for t in sdtl_ref.stat_topics_channels_trees[k]['tx'].children]

        self.rx_stat_table = ewTable(caption='SDTL RX', data_sources=data_service_rx_stat)
        self.add_nested(self.rx_stat_table)
        self.layout.addWidget(self.rx_stat_table)

        self.channels_subwidgets = []

        tx_bytes_sum_sources = []

        for cw in data_service_channels_stat.items():
            self.channels_subwidgets.append(ewTable(caption=cw[0], data_sources=cw[1]))
            ds = find_data_source(cw[1], 'tx_bytes')
            if ds:
                tx_bytes_sum_sources.append(ds)
            self.add_nested(self.channels_subwidgets[-1])
            self.layout.addWidget(self.channels_subwidgets[-1])

        self.rx_rate_chart_widget = ewChart([
            DataSourceCalcFilteredRate('rx_speed', bytes_receive_source),
            DataSourceCalcFilteredRate('tx_speed', DataSourceSum('tx_bytes_sum', tx_bytes_sum_sources))
        ], data_range=(0, 10000))

        self.add_nested(self.rx_rate_chart_widget)
        self.layout.addWidget(self.rx_rate_chart_widget)


class Monitor:
    def __init__(self, *, monitor_bus_name='monitor', argv=None):
        super().__init__()
        self.service_bus_name = monitor_bus_name
        self.app = QtWidgets.QApplication(argv)
        self.bus = e.Bus(monitor_bus_name)
        self.app_window = ApplicationWindow(self.bus)
        self.sdtl_service: e.SDTLserialService = None
        pass

    def connect(self):
        self.app_window.connect()

    def show(self):
        self.app_window.show()

    def run(self):
        self.connect()
        self.show()
        sys.exit(self.app.exec_())

    def mkdir(self, dirname):
        self.bus.mkdir(dirname)

    def bridge_sdtl(self, *, path, baudrate, bridge_to):
        sdtl_service_name = 'sdtl_serial'
        self.sdtl_service = e.SDTLserialService(service_name=sdtl_service_name, device_path=path, mtu=0,
                                 baudrate=int(baudrate),
                                 channels=[
                                     e.SDTLchannel(name='bus_sync', ch_id=1, ch_type=e.SDTLchannelType.rel),
                                     e.SDTLchannel(name='bus_sync_sk', ch_id=2, ch_type=e.SDTLchannelType.unrel),
                                 ]
                                 )

        self.sdtl_service.start()

        eqrb = e.EQRB_SDTL(sdtl_service_name=sdtl_service_name,
                         replicate_to_path=f'{self.service_bus_name}/{bridge_to}',
                         ch1='bus_sync',
                         ch2='bus_sync_sk')

        eqrb.start()

    def get_stat_widget(self):
        return SdtlTelemetryWidget(self.sdtl_service)

    def add_widget(self, w):
        self.app_window.add_ew(w)
