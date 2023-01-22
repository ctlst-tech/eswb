from typing import List, Union

from PyQt5.QtCore import Qt
from PyQt5.QtGui import QPalette

from controls import EwBasic
from controls.datasources import DataSourceBasic, NoDataStub

import pyqtgraph.opengl as gl
import numpy as np


# python3 -m pyqtgraph.examples
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

        self.max_len = 1000

        self.points = np.zeros((self.max_len, 3), dtype=np.float32)
        self.sizes = np.zeros((self.max_len), dtype=np.float32)
        self.colors = np.zeros((self.max_len, 4), dtype=np.float32)
        self.cur_pt_idx = 0

        self.graph = gl.GLScatterPlotItem(pos=self.points, size=self.sizes, color=self.colors, pxMode=False)
        self.graph.translate(0, 0, 0)
        self.addItem(self.graph)
        self.setCameraPosition(distance=40)
        self.add_sphere()

    def repaint(self):
        self.graph.setData(pos=self.points)
        self.graph.update()

    def add_sphere(self, pos=(0.0, 0.0, 0.0), color=(0.0, 0.9, 0, 0.1), radius=(10.0)):
        md = gl.MeshData.sphere(rows=20, cols=30, radius=radius)
        m1 = gl.GLMeshItem(
            meshdata=md,
            smooth=True,
            color=color,
            shader="balloon",
            glOptions="additive",
        )
        m1.translate(*pos)
        self.addItem(m1)

    def add_point(self, pos=(1.0, 0.0, 0.0), size=(0.3), color=(1.0, 1.0, 1.0, 0.5)):
        idx = self.cur_pt_idx
        self.points[idx] = pos
        self.sizes[idx] = size
        self.colors[idx] = color

        self.cur_pt_idx = self.cur_pt_idx + 1
        if self.cur_pt_idx == self.max_len:
            self.cur_pt_idx = 0

    def radraw_handler(self, vals: List[Union[float, int, str, NoDataStub]]):
        self.add_point(pos=(vals[0], vals[1], vals[2]),
                       size=(0.5),
                       color=(1.0, 0.0, 0.0, 0.5))
        self.repaint()
