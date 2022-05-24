#!/usr/bin/env python3

# TODO https://github.com/pyqt/examples

from abc import abstractmethod
from time import sleep
from typing import List

from PyQt5 import QtWidgets, QtCore
import pyqtgraph as pg
from random import randint

import eswb as e

class ewBasic(QtWidgets.QWidget):
    def __init__(self,name, path, *args, **kwargs):
        super(ewBasic, self).__init__(*args, **kwargs)

        self.topics = []
        self.topics.append(e.TopicHandle(name, path))

    @abstractmethod
    def update_handler(self, vals: List[float]):
        pass

    def connect(self):
        for t in self.topics:
            t.connect()

    def update(self):
        vals = []
        for t in self.topics:
            vals.append(t.value())

        self.update_handler(vals)


class ewChart(ewBasic):
    def __init__(self, *args, **kwargs):
        super(ewChart, self).__init__(*args, **kwargs)
        self.graph = pg.PlotWidget()

        layout = QtWidgets.QBoxLayout(QtWidgets.QBoxLayout.LeftToRight)
        layout.addWidget(self.graph)

        self.x = list(range(100))
        self.y = [randint(0, 100) for _ in range(100)]

        pen = pg.mkPen(color=(255, 0, 0))
        self.data_line = self.graph.plot(self.x, self.y, pen=pen)

        self.setLayout(layout)

    def update_handler(self, vals: List[float]):

        self.x = self.x[1:]
        self.x.append(self.x[-1] + 1)

        self.y = self.y[1:]
        self.y.append(int(vals[0]))

        self.data_line.setData(self.x, self.y)


class ApplicationWindow(QtWidgets.QMainWindow):
    def __init__(self):
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

    def add_ew(self, widget):
        self.main_layout.addWidget(widget)
        self.widgets.append(widget)

    def connect(self):
        for w in self.widgets:
            w.connect()

    def redraw(self):
        for w in self.widgets:
            w.update()


if __name__ == "__main__":
    import sys
    app = QtWidgets.QApplication(sys.argv)
    eswb_display = ApplicationWindow()

    b = e.Bus('service')
    b.mkdir('vehicle')

    client = e.EQRBtcp('itb:/vehicle', f'nsb:/service/vehicle')
    client.connect('127.0.0.1')

    eswb_display.add_ew(ewChart(name='rpm', path='nsb:/service/vehicle/engine/rpm'))
    eswb_display.add_ew(ewChart(name='cht', path='nsb:/service/vehicle/engine/cht'))
    eswb_display.add_ew(ewChart(name='egt', path='nsb:/service/vehicle/engine/egt'))
    eswb_display.add_ew(ewChart(name='oil_press', path='nsb:/service/vehicle/engine/oil_press'))

    eswb_display.show()

    sleep(0.2)

    b.update_tree()
    t = b.get_topics_tree()

    eswb_display.connect()

    sys.exit(app.exec_())
