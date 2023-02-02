from typing import Union

from PyQt5.QtWidgets import QLineEdit

from . import NoDataStub
from .datasources import DataSourceBasic


class DataSourceWidget(DataSourceBasic):
    def connect(self):
        pass

    def read(self) -> Union[float, int, str, NoDataStub]:
        return self.value

    def __init__(self, name, widget: QLineEdit):
        super().__init__(name)
        self.widget = widget
        self.value = 0.0
        self.widget.textChanged.connect(self._widget_edited)

    def _widget_edited(self, val):
        self.set_value(self.float(val))

    def set_value(self, v):
        self.value = v

    @staticmethod
    def float(s: str):
        try:
            return float(s)
        except ValueError:
            return 0.0
