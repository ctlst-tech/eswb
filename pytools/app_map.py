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

def add_vector(_id):
    return lambda: m.add_marker(_id, [52.52472910525925, 13.375974062388432])


def rotate_vector(_id):
    return lambda: m.rotate_marker(_id, m.get_marker_rotation(_id) + 15)


dlon = {
    "1": 52.52472910525925,
    "2": 52.52472910525925
}

offs = {
    "1": 0.00080,
    "2": -0.00090
}


def move_vector(_id):
    def _move():
        dlon[_id] = dlon[_id] + offs[_id]
        m.move_marker(_id, [dlon[_id], 13.375974062388432])

    return _move


#
# def remove_vector(v):
#     m.rm_vector(v)


# --------- Marker (aka aim)

def add_marker(_id):
    mr = m.add_marker(_id, [52.52472910525925, 13.375974062388432])
    return mr


# def update_marker(mr):
#     m.update_marker(mr, [52.52472910525925, 13.375974062388432])
#
#
# def remove_marker(mr):
#     m.rm_marker(mr)


# --------- Path
def add_path():
    m.set_path([[52.52472910525925, 13.375974062388432],
                [52.52472910525925, 13.375974062388432],
                [52.52472910525925, 13.375974062388432],
                [52.52472910525925, 13.375974062388432]])


def rm_path(p):
    m.clear_path()


button1 = QPushButton()
button1.setText("Center location")
button1.move(0, 0)
button1.resize(150, 40)
button1.clicked.connect(center_map)

button2 = QPushButton()
button2.setText("Add airplane")
button2.move(160, 0)
button2.resize(150, 40)
button2.clicked.connect(add_vector("1"))

button3 = QPushButton()
button3.setText("Rotate airplane")
button3.move(320, 0)
button3.resize(150, 40)
button3.clicked.connect(rotate_vector("1"))

button4 = QPushButton()
button4.setText("Move airplane")
button4.move(470, 0)
button4.resize(150, 40)
button4.clicked.connect(move_vector("1"))

button12 = QPushButton()
button12.setText("Add airplane #2")
button12.move(160, 50)
button12.resize(150, 40)
button12.clicked.connect(add_vector("2"))

button13 = QPushButton()
button13.setText("Rotate airplane #2")
button13.move(320, 50)
button13.resize(150, 40)
button13.clicked.connect(rotate_vector("2"))

button14 = QPushButton()
button14.setText("Move airplane #2")
button14.move(470, 50)
button14.resize(150, 40)
button14.clicked.connect(move_vector("2"))

mon.add_widget(EwGroup([m]))
mon.add_button(button1)
mon.add_button(button2)
mon.add_button(button3)
mon.add_button(button4)

mon.add_button(button12)
mon.add_button(button13)
mon.add_button(button14)
mon.run()
