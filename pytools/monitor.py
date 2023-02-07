import sys
from typing import List

from PyQt5 import QtWidgets, QtCore
from PyQt5.QtCore import QObject, pyqtSignal, QThreadPool, QRunnable, pyqtSlot
from PyQt5.QtWidgets import QPushButton, QApplication


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

    def add_ew(self, widget, stretch=0):
        self.main_layout.addWidget(widget, stretch=stretch)

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

    def add_widget(self, w, stretch=0):
        self.main_layout.addWidget(w, stretch=stretch)
        self.parent.app_window.reg_widget(w)


class WorkerSignals(QObject):
    close = pyqtSignal()
    error = pyqtSignal(tuple)
    result = pyqtSignal(object)


class Worker(QRunnable):
    """
    Worker thread
    """
    def __init__(self, job, sleep=0.1):
        super().__init__()
        self.job = job
        self.sleep = sleep if sleep > 0.01 else 0.01
        self.signals = WorkerSignals()
        self.signals.close.connect(self.terminate)
        self.finished = False

    def terminate(self):
        self.finished = True

    def process_events(self):
        QApplication.processEvents()
        if self.finished:
            return False

        return True

    @pyqtSlot()
    def run(self):
        """
        Your code goes in this function
        """
        print("Thread start")
        self.job()
        self.process_events()
        print("Thread finished")


class Monitor:
    def __init__(self, *, monitor_bus_name='monitor', argv=None, tabs=False):
        super().__init__()
        self.app = QtWidgets.QApplication(argv)
        self.app_window = ApplicationWindow(tabs=tabs)
        self.service_bus_name = monitor_bus_name
        self.tab_widget = None
        self.app.setStyleSheet("QLineEdit[readOnly=\"true\"] {color: #808080; background-color: #F0F0F0; padding: 0}")
        self.app.aboutToQuit.connect(self.on_quit)
        self.runnables: List[Worker] = []

    def on_quit(self):
        for r in self.runnables:
            r.signals.close.emit()

    def run_task(self, runnable: QRunnable):
        self.runnables.append(runnable)
        pool = QThreadPool.globalInstance()
        pool.start(runnable)

    def report_progress(self):
        pass

    def connect(self):
        self.app_window.connect()

    def show(self):
        self.app_window.show()

    def run(self):
        self.connect()
        self.show()
        sys.exit(self.app.exec_())

    def add_widget(self, w, stretch=0):
        self.app_window.add_ew(w, stretch=stretch)
        self.app_window.reg_widget(w)

    def add_button(self, btn: QPushButton):
        btn.setParent(self.app_window.main_widget)

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
