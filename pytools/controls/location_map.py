from typing import List, Union

from PyQt5.QtGui import QPainter

from pytools.controls import MyQtWidget, EwBasic
from pytools.controls.datasources import DataSourceBasic, NoDataStub


class EwLocationMap(MyQtWidget, EwBasic):
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

        self.layout.addWidget(self)

    def paintEvent(self, event):
        canvas = QPainter(self)

        # SAMPLE_TODO drawing code
        canvas.end()

    def radraw_handler(self, vals: List[Union[float, int, str, NoDataStub]]):
        # SAMPLE_TODO pass upcoming data to widget's state
        self.repaint()