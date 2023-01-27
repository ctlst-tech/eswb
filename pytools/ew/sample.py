from typing import List, Union, Dict

from PyQt5.QtGui import QPainter

from ds import DataSourceBasic, NoDataStub

from .common import MyQtWidget
from .widgets import EwBasic


class EwPaintSample(MyQtWidget, EwBasic):
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

    def radraw_handler(self, vals: List[Union[float, int, str, NoDataStub]], vals_map: Dict):
        # SAMPLE_TODO pass upcoming data to widget's state
        self.repaint()
