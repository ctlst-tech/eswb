from typing import Union, List

from PyQt5.QtWidgets import QLineEdit
from PyQt5.QtWidgets import QLineEdit, QPushButton

from . import NoDataStub
from .datasources import DataSourceBasic

ButtonValuesT = tuple[QPushButton, int]


class DataSourceButtons(DataSourceBasic):
    """
        Example:
        `cmd_ds = DataSourceButtons("buttons", [(cmd.take_off_btn, 1), (cmd.land_btn, 2)])`
    """
    value: int = 0

    def __init__(self, name, cmd_tuples: List[ButtonValuesT]):
        super().__init__(name)
        for c in cmd_tuples:
            [btn, v] = c
            btn.clicked.connect(self.set_value_fn(v))

    def set_value_fn(self, v):
        def _set_val():
            self.value = v

        return _set_val

    def read(self) -> Union[float, int, str, NoDataStub]:
        return self.value

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
