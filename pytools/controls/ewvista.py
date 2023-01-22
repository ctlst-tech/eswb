from typing import List, Union

from PyQt5.QtCore import Qt
from PyQt5.QtGui import QPainter
from pyvistaqt import QtInteractor
from PyQt5.QtGui import QPalette

from controls import MyQtWidget, EwBasic
from controls.datasources import DataSourceBasic, NoDataStub
import pyvista as pv
import numpy as np


# https://docs.pyvista.org/examples/00-load/index.html
class EwPyVista(MyQtWidget, EwBasic):
    def __init__(self, data_sources: List[DataSourceBasic], **kwargs):
        MyQtWidget.__init__(self, **kwargs)
        EwBasic.__init__(self)

        self.set_data_sources(data_sources)

        # set black background
        pal = self.palette()
        pal.setColor(QPalette.Background, Qt.black)
        self.setAutoFillBackground(True)
        self.setPalette(pal)

        self.plotter = QtInteractor(self)
        self.plotter.setFixedSize(380, 380)
        self.layout.addWidget(self.plotter.interactor)

        self.generate_mesh()

    def generate_mesh(self):
        # Make data
        x = np.arange(-10, 10, 0.25)
        y = np.arange(-10, 10, 0.25)
        x, y = np.meshgrid(x, y)
        r = np.sqrt(x ** 2 + y ** 2)
        z = np.sin(r)

        grid = pv.StructuredGrid(x, y, z)

        # sphere = pv.Sphere()
        self.plotter.add_mesh(grid, show_edges=True)
        self.plotter.reset_camera()

    def radraw_handler(self, vals: List[Union[float, int, str, NoDataStub]]):
        self.plotter.repaint()
