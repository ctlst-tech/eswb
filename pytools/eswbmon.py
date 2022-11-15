#!/usr/bin/env python3

# TODO https://github.com/pyqt/examples

from abc import abstractmethod
from time import sleep
from typing import List
import sys

from PyQt5 import QtWidgets, QtCore
import pyqtgraph as pg
from random import randint

from PyQt5.QtWidgets import QTableWidget
from pyqtgraph.Qt import QtGui

import eswb as e
import os

service_bus_name = 'monitor'


class ewBasic(QtWidgets.QWidget):
    def __init__(self, path, name=None, abs_path=False, *args, **kwargs):
        super(ewBasic, self).__init__(*args, **kwargs)

        self.topic_handles: List[e.TopicHandle] = []

        if not abs_path:
            path = service_bus_name + '/' + path

        if not name:
            name = os.path.basename(os.path.normpath(path))

        self.topic_handles.append(e.TopicHandle(name, path))
        self.layout = QtWidgets.QBoxLayout(QtWidgets.QBoxLayout.LeftToRight)
        self.setLayout(self.layout)

    @abstractmethod
    def update_handler(self, vals: List[float]):
        pass

    def connect(self):
        for t in self.topic_handles:
            t.connect()

    def update(self):
        vals = []
        for t in self.topic_handles:
            try:
                value = t.value()
            except:
                value = 0
                try:
                    t.connect()
                except:
                    pass

            vals.append(value)

        self.update_handler(vals)


class ewTable(ewBasic):
    def __init__(self, *args, **kwargs):
        super(ewChart, self).__init__(*args, **kwargs)

        self.table = QTableWidget

class ewChart(ewBasic):
    def __init__(self, *args, **kwargs):
        super(ewChart, self).__init__(*args, **kwargs)
        self.graph = pg.PlotWidget()

        self.layout.addWidget(self.graph)

        window = 600

        self.x = list(range(window))
        self.y = [0.0] * window

        pen = pg.mkPen(color=(0, 0, 0), width=2)
        self.data_line = self.graph.plot(self.x, self.y, pen=pen)
        self.graph.setBackground(QtGui.QColor('white'))

    def update_handler(self, vals: List[float]):

        self.x = self.x[1:]
        self.x.append(self.x[-1] + 1)

        self.y = self.y[1:]
        self.y.append(vals[0])

        self.data_line.setData(self.x, self.y)


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
        self.timer.setInterval(10)
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
            w.update()


class Monitor():
    def __init__(self, argv=None):
        super().__init__()
        self.app = QtWidgets.QApplication(argv)
        self.bus = e.Bus(service_bus_name)
        self.app_window = ApplicationWindow(self.bus)
        self.eqrb_clients = []
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
        sdtl = e.SDTLserialService(service_name=sdtl_service_name, device_path=path, mtu=0,
                                 baudrate=int(baudrate),
                                 channels=[
                                     e.SDTLchannel(name='bus_sync', ch_id=1, ch_type=e.SDTLchannelType.rel),
                                     e.SDTLchannel(name='bus_sync_sk', ch_id=2, ch_type=e.SDTLchannelType.unrel),
                                 ]
                                 )

        sdtl.start()

        eqrb = e.EQRB_SDTL(sdtl_service_name=sdtl_service_name,
                         replicate_to_path=f'{service_bus_name}/{bridge_to}',
                         ch1='bus_sync',
                         ch2='bus_sync_sk')

        eqrb.start()

    def add_widget(self, w):
        self.app_window.add_ew(w)


if __name__ == "__main__":
    import sys
    mon = Monitor(sys.argv)

    mon.mkdir('conversions')
    mon.mkdir('generators')

    mon.bridge(bus2replicate='itb:/conversions')
    mon.bridge(bus2replicate='itb:/generators')

    mon.add_widget(ewChart(path='generators/sin/out'))
    mon.add_widget(ewChart(path='generators/saw/out'))
    mon.add_widget(ewChart(name='lin_freq', path='conversions/lin_freq/out'))

    mon.run()
