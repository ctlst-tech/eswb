#!/usr/bin/env python3

import sys
from ds import *
from ew import *
from monitor import Monitor

mon_bus_name = 'monitor'
telemetry_dir_name = 'telemetry'

mon = Monitor(monitor_bus_name=mon_bus_name, argv=sys.argv)

rp = EwRelativePosition([
    DataSourceSinus('plane_phi', mult=360),
    DataSourceConst('plane_r', value=60),
    DataSourceSinus('plane_course', iphase=0.0, mult=360),

    DataSourceSinus('base_phi', iphase=0.5, mult=360),
    DataSourceConst('base_r', value=120),

    DataSourceSinus('target_phi', iphase=-0.5, mult=360),
    DataSourceConst('target_r', value=160),
])



mon.add_widget(EwGroup([rp]))

mon.app_window.resize(800, 800)

mon.run()
