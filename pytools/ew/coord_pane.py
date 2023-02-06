from typing import List, Union, Dict

from PyQt5.QtCore import Qt
from PyQt5.QtWidgets import QLabel, QLineEdit, QGroupBox, QFormLayout

from ds import DataSourceBasic, NoDataStub
from ew import MyQtWidget, EwBasic


class EwCoordPane(MyQtWidget, EwBasic):
    def __init__(self, data_sources: List[DataSourceBasic], title=None, fixed_size=None, **kwargs):
        MyQtWidget.__init__(self, **kwargs)
        EwBasic.__init__(self)

        if fixed_size:
            self.setFixedSize(fixed_size[0], fixed_size[1])

        self.set_data_sources(data_sources)

        self.y_lines = [10, 46, 82, 118]
        self.lbl_len = 100
        self.line_len = 100
        self.line_height = 32
        self.padding_left = 20
        self.radius = 0
        self.phi = 0
        self.alt = 0

        self.form_layout = QFormLayout()
        self.group = QGroupBox(title, self)
        self.group.setLayout(self.form_layout)
        self.layout.addWidget(self.group)

        self.radius_lbl = QLabel(self.group)
        self.radius_lbl.setText('Radius (m):')
        self.radius_lbl.resize(self.lbl_len, self.line_height)

        self.radius_line = QLineEdit(self.group)
        self.radius_line.resize(self.line_len, 32)
        self.radius_line.setReadOnly(len(data_sources) > 0)

        self.form_layout.addRow(self.radius_lbl, self.radius_line)

        self.phi_lbl = QLabel(self)
        self.phi_lbl.setText('Phi (deg):')
        self.phi_lbl.resize(self.lbl_len, self.line_height)

        self.phi_line = QLineEdit(self)
        self.phi_line.setReadOnly(len(data_sources) > 0)
        self.phi_line.resize(self.line_len, 32)

        self.form_layout.addRow(self.phi_lbl, self.phi_line)

        self.altitude_lbl = QLabel(self)
        self.altitude_lbl.setText('Altitude (m):')
        self.altitude_lbl.resize(self.lbl_len, self.line_height)

        self.altitude_line = QLineEdit(self)
        self.altitude_line.resize(self.line_len, 32)
        self.altitude_line.setReadOnly(len(data_sources) > 0)

        self.form_layout.addRow(self.altitude_lbl, self.altitude_line)

        # noinspection PyUnresolvedReferences
        self.radius_line.textEdited.connect(self._radius_edited)
        # noinspection PyUnresolvedReferences
        self.radius_line.editingFinished.connect(self._radius_editing_finish)

        # noinspection PyUnresolvedReferences
        self.phi_line.textEdited.connect(self._phi_edited)
        # noinspection PyUnresolvedReferences
        self.phi_line.editingFinished.connect(self._phi_editing_finish)

        # noinspection PyUnresolvedReferences
        self.altitude_line.textEdited.connect(self._alt_edited)
        # noinspection PyUnresolvedReferences
        self.altitude_line.editingFinished.connect(self._alt_editing_finish)

        self._phi_editing_finish()
        self._radius_editing_finish()
        self._alt_editing_finish()

        self.layout.addWidget(self)

    @staticmethod
    def float(s: str):
        try:
            return float(s)
        except ValueError:
            return 0.0

    def _radius_editing_finish(self):
        self.radius_line.setText(self.fmt_float(self.radius))
        self.radius_line.setCursorPosition(0)

    def _phi_editing_finish(self):
        self.phi_line.setText(self.fmt_float(self.phi))
        self.phi_line.setCursorPosition(0)

    def _alt_editing_finish(self):
        self.altitude_line.setText(self.fmt_float(self.alt))
        self.altitude_line.setCursorPosition(0)

    def _alt_edited(self, val):
        self.set_alt(self.float(val))

    def _radius_edited(self, val):
        self.set_radius(self.float(val))

    def _phi_edited(self, val):
        self.set_phi(self.float(val))

    def set_radius(self, v):
        self.radius = v

    def set_alt(self, v):
        self.alt = v

    def set_phi(self, v):
        self.phi = v

    def update_alt(self, a):
        self.set_alt(a)
        self._alt_editing_finish()

    def update_radius(self, v):
        self.set_radius(v)
        self._radius_editing_finish()

    def update_phi(self, v):
        self.set_phi(v)
        self._phi_editing_finish()

    @staticmethod
    def fmt_float(val):
        return str(round(val, 2))

    def radraw_handler(self, vals: List[Union[float, int, str, NoDataStub]], vals_map: Dict):
        if len(self.data_sources) > 0:
            self.set_phi(vals[0])
            self._phi_editing_finish()
            self.set_radius(vals[1])
            self._radius_editing_finish()
            self.set_alt(vals[2])
            self._alt_editing_finish()


class EwCoordCoursePane(EwCoordPane):
    def __init__(self, data_sources: List[DataSourceBasic], **kwargs):
        super().__init__(data_sources, **kwargs)

        self.course = 0

        self.course_lbl = QLabel(self)
        self.course_lbl.setText('Course (deg):')

        self.course_edit = QLineEdit(self)
        self.course_edit.setAlignment(Qt.AlignLeft)
        self.course_edit.setReadOnly(len(data_sources) > 0)
        self.course_edit.resize(self.line_len, self.line_height)

        self.form_layout.addRow(self.course_lbl, self.course_edit)

        # noinspection PyUnresolvedReferences
        self.course_edit.textEdited.connect(self._course_edited)
        # noinspection PyUnresolvedReferences
        self.course_edit.editingFinished.connect(self._course_editing_finish)

    def _course_edited(self, val):
        self.set_course(self.float(val))

    def _course_editing_finish(self):
        self.course_edit.setText(self.fmt_float(self.course))
        self.course_edit.setCursorPosition(0)

    def set_course(self, v):
        self.course = v

    def radraw_handler(self, vals: List[Union[float, int, str, NoDataStub]], vals_map: Dict):
        if len(self.data_sources) > 0:
            self.set_phi(vals[0])
            self._phi_editing_finish()
            self.set_radius(vals[1])
            self._radius_editing_finish()
            self.set_alt(vals[2])
            self._alt_editing_finish()
            self.set_course(vals[3])
            self._course_editing_finish()

