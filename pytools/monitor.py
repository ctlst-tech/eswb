import sys
from PyQt5 import QtWidgets, QtCore


class ApplicationWindow(QtWidgets.QMainWindow):
    def __init__(self, *, title="ESWB display", tabs=False):
        super().__init__()

        self.widgets = []

        # self.setAttribute(QtCore.Qt.WA_DeleteOnClose)
        self.setWindowTitle(title)

        self.main_widget = QtWidgets.QWidget(self) if not tabs else QtWidgets.QTabWidget()
        self.main_widget.setFocus()
        self.setCentralWidget(self.main_widget)

        self.main_layout = QtWidgets.QVBoxLayout(self.main_widget)

        self.timer = QtCore.QTimer()
        self.timer.setInterval(20)
        self.timer.timeout.connect(self.redraw)
        self.timer.start()

    def add_ew(self, widget):
        self.main_layout.addWidget(widget)

    def reg_widget(self, widget):
        self.widgets.append(widget)

    def connect(self):
        for w in self.widgets:
            try:
                w.connect()
            except:
                pass

    def redraw(self):
        for w in self.widgets:
            w.redraw()


class Tab(QtWidgets.QWidget):
    def __init__(self, *, parent):
        super().__init__()
        self.parent = parent
        self.main_layout = QtWidgets.QVBoxLayout(self)

    def add_widget(self, w):
        self.main_layout.addWidget(w)
        self.parent.app_window.reg_widget(w)


class Monitor:
    def __init__(self, *, monitor_bus_name='monitor', argv=None, tabs=False):
        super().__init__()
        self.app = QtWidgets.QApplication(argv)
        self.app_window = ApplicationWindow(tabs=tabs)
        self.service_bus_name = monitor_bus_name
        self.tab_widget = None
        pass

    def connect(self):
        self.app_window.connect()

    def show(self):
        self.app_window.show()

    def run(self):
        self.connect()
        self.show()
        sys.exit(self.app.exec_())

    def add_widget(self, w):
        self.app_window.add_ew(w)
        self.app_window.reg_widget(w)

    def add_tab(self, name):
        tab = Tab(parent=self)
        self.app_window.main_widget.addTab(tab, name)
        return tab


class ArgParser:
    def __init__(self):
        import argparse

        self.parser = argparse.ArgumentParser('ESWB monitor tool')

        self.parser.add_argument(
            '--serdev',
            action='store',
            default='/dev/ttyUSB0',
            type=str,
            help='Serial interface device path',
        )

        self.parser.add_argument(
            '--serbaud',
            action='store',
            default='115200',
            type=int,
            help='Serial interface baudrate',

        )

        self.args = self.parser.parse_args(sys.argv[1:])