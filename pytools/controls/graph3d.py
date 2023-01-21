from typing import List, Union

from PyQt5.QtCore import Qt
from PyQt5.QtGui import QPalette

from controls import EwBasic
from controls.datasources import DataSourceBasic, NoDataStub

import pyqtgraph.opengl as gl
import numpy as np


# https://doc.qt.io/qt-5/qopenglwidget.html
# https://pyqtgraph.readthedocs.io/en/latest/_modules/pyqtgraph/opengl/GLViewWidget.html
class EwGraph3D(gl.GLViewWidget, EwBasic):
    def __init__(self, data_sources: List[DataSourceBasic], parent=None, **kwargs):
        gl.GLViewWidget.__init__(self, parent)
        self.setFocusPolicy(Qt.FocusPolicy.ClickFocus)
        EwBasic.__init__(self)

        self.set_data_sources(data_sources)
        self.setFixedSize(380, 380)

        # set black background
        pal = self.palette()
        pal.setColor(QPalette.Background, Qt.black)
        self.setAutoFillBackground(True)
        self.setPalette(pal)

        self.xgrid = gl.GLGridItem()
        self.xgrid.scale(10, 10, 1)
        self.addItem(self.xgrid)

        self.ax = gl.GLAxisItem()
        self.ax.setSize(100, 100, 100)
        self.addItem(self.ax)

        self.init_model()

        self.setCameraPosition(distance=40)

    def init_model(self):
        # phi = np.linspace(0, 2 * np.pi, 256).reshape(256, 1)  # the angle of the projection in the xy-plane
        # theta = np.linspace(0, np.pi, 256).reshape(-1, 256)  # the angle from the polar axis, ie the polar angle
        # radius = 4

        # Transformation formulae for a spherical coordinate system.
        # x = radius * np.sin(theta) * np.cos(phi)
        # y = radius * np.sin(theta) * np.sin(phi)
        # z = radius * np.cos(theta)
        # points = np.column_stack((x, y, z))
        # grid = pv.StructuredGrid(x, y, z)

        x = np.linspace(-8, 8, 50)
        y = np.linspace(-8, 8, 50)
        z = 0.1 * ((x.reshape(50, 1) ** 2) - (y.reshape(1, 50) ** 2))

        p2 = gl.GLSurfacePlotItem(x=x, y=y, z=z, shader='normalColor')
        p2.translate(-10, -10, 0)
        self.addItem(p2)

    def repaint(self):
        self.paintGL()

    def radraw_handler(self, vals: List[Union[float, int, str, NoDataStub]]):
        self.repaint()
