from typing import List, Union, Dict

from PyQt5.QtWidgets import QFormLayout, QGroupBox, QPushButton, QVBoxLayout

from ds import NoDataStub, DataSourceBasic
from ew import MyQtWidget, EwBasic


class EwCommandPane(MyQtWidget, EwBasic):
    def __init__(self, data_sources: List[DataSourceBasic], title=None, fixed_size=None, **kwargs):
        MyQtWidget.__init__(self, **kwargs)
        EwBasic.__init__(self)

        if fixed_size:
            self.setFixedSize(fixed_size[0], fixed_size[1])

        self.set_data_sources(data_sources)

        self.form_layout = QVBoxLayout()
        self.group = QGroupBox(title, self)
        self.group.setLayout(self.form_layout)
        self.layout.addWidget(self.group)

        self.idle_btn = QPushButton(self.group)
        self.idle_btn.setText("Idle")
        self.form_layout.addWidget(self.idle_btn, 1)

        self.take_off_btn = QPushButton(self.group)
        self.take_off_btn.setText("Take off!")
        self.form_layout.addWidget(self.take_off_btn, 1)

        self.land_btn = QPushButton(self.group)
        self.land_btn.setText("Land")
        self.form_layout.addWidget(self.land_btn, 1)

    def radraw_handler(self, vals: List[Union[float, int, str, NoDataStub]], vals_map: Dict):
        pass
