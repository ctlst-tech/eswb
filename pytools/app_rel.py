#!/usr/bin/env python3

import sys
from ds import *
from ew import *
from monitor import Monitor

mon_bus_name = 'monitor'
telemetry_dir_name = 'telemetry'

mon = Monitor(monitor_bus_name=mon_bus_name, argv=sys.argv)
zero = DataSourceConst('zero', value=0)

rp = EwRelativePosition([
    DataSourceConst("boat", value="airplane-white"),
    DataSourceSinus('plane_phi', mult=360),
    DataSourceConst('plane_r', value=60),
    DataSourceConst('zero', value=20),  # alt
    DataSourceSinus('plane_course', iphase=0.0, mult=360),

    DataSourceConst("boat", value="aim-yellow"),
    DataSourceSinus('base_phi', iphase=0.5, mult=360),
    DataSourceConst('base_r', value=120),
    zero,  # alt
    zero,  # course

    DataSourceConst("boat", value="boat-white"),  # icon
    DataSourceSinus('target_phi', iphase=-0.5, mult=360),
    DataSourceConst('target_r', value=160),
    zero,  # alt
    zero,  # course
])

mon.add_widget(EwGroup([rp]))

mon.app_window.resize(800, 800)

mon.run()
