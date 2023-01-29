import sys
import os

from PyQt5.QtWidgets import QPushButton

from ds import *
from ew import *
from monitor import Monitor

# DEBUG_PORT = '5588'
# DEBUG_URL = 'http://127.0.0.1:%s' % DEBUG_PORT
# os.environ['QTWEBENGINE_REMOTE_DEBUGGING'] = DEBUG_PORT
# sys.argv.append("--disable-web-security")

mon_bus_name = 'monitor'
telemetry_dir_name = 'telemetry'

mon = Monitor(monitor_bus_name=mon_bus_name, argv=sys.argv)

m = EwLocationMap([
    DataSourceConst('riga', value=(57.0764, 24.3308)),
    DataSourceConst('berlin', value=(52.52472910525925, 13.375974062388432)),
    DataSourceConst('paris', value=(48.865383664217475, 2.3363903920362357)),
    DataSourceTimeline('time')
])


def center_map():
    m.center_map([52.52472910525925, 13.375974062388432])


# -------- Vector (aka airplane)

def add_marker(_id):
    return lambda: m.add_marker(_id, [52.52472910525925, 13.375974062388432], 24, 24)


def rotate_marker(_id):
    return lambda: m.rotate_marker(_id, m.get_marker_rotation(_id) + 15)


dlon = {
    "1": 52.52472910525925,
    "2": 52.52472910525925
}

offs = {
    "1": 0.00080,
    "2": -0.00090
}


def move_marker(_id):
    def _move():
        dlon[_id] = dlon[_id] + offs[_id]
        m.move_marker(_id, [dlon[_id], 13.375974062388432])

    return _move


def rm_marker(_id):
    return lambda: m.remove_marker(_id)


demo_poly = [
    [45.51, -122.68],
    [37.77, -122.43],
    [34.04, -118.2]
]


# --------- Path
def add_poly(_id):
    def _add():
        m.add_poly(_id, demo_poly, color='#0000ff')

    return _add


def fit_poly(_id):
    def _fit():
        m.fit_poly(_id)

    return _fit


def rm_poly(_id):
    def _rm():
        m.remove_poly(_id)

    return _rm


button1 = QPushButton()
button1.setText("Center location")
button1.move(0, 0)
button1.resize(150, 40)
button1.clicked.connect(center_map)

button2 = QPushButton()
button2.setText("Add airplane")
button2.move(160, 0)
button2.resize(150, 40)
button2.clicked.connect(add_marker("1"))

button3 = QPushButton()
button3.setText("Rotate airplane")
button3.move(320, 0)
button3.resize(150, 40)
button3.clicked.connect(rotate_marker("1"))

button4 = QPushButton()
button4.setText("Move airplane")
button4.move(470, 0)
button4.resize(150, 40)
button4.clicked.connect(move_marker("1"))

button12 = QPushButton()
button12.setText("Add airplane #2")
button12.move(160, 50)
button12.resize(150, 40)
button12.clicked.connect(add_marker("2"))

button13 = QPushButton()
button13.setText("Rotate airplane #2")
button13.move(320, 50)
button13.resize(150, 40)
button13.clicked.connect(rotate_marker("2"))

button14 = QPushButton()
button14.setText("Move airplane #2")
button14.move(470, 50)
button14.resize(150, 40)
button14.clicked.connect(move_marker("2"))

button15 = QPushButton()
button15.setText("Remove airplane #2")
button15.move(470, 50)
button15.resize(150, 40)
button15.clicked.connect(rm_marker("2"))

button22 = QPushButton()
button22.setText("Add poly")
button22.move(160, 100)
button22.resize(150, 40)
button22.clicked.connect(add_poly("1"))

button23 = QPushButton()
button23.setText("Fit poly")
button23.move(320, 100)
button23.resize(150, 40)
button23.clicked.connect(fit_poly("1"))

button24 = QPushButton()
button24.setText("Remove poly")
button24.move(470, 100)
button24.resize(150, 40)
button24.clicked.connect(rm_poly("1"))

mon.add_widget(EwGroup([m]))
mon.add_button(button1)
mon.add_button(button2)
mon.add_button(button3)
mon.add_button(button4)

mon.add_button(button12)
mon.add_button(button13)
mon.add_button(button14)

mon.add_button(button22)
mon.add_button(button23)
mon.add_button(button24)
mon.run()
