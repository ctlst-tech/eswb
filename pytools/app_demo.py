#!/usr/bin/env python3

import sys
from datasources import *
from controls import *
from controls.relative_position import *
from monitor import Monitor

mon_bus_name = 'monitor'
telemetry_dir_name = 'telemetry'

mon = Monitor(monitor_bus_name=mon_bus_name, argv=sys.argv)

# manc_xy = ewCursor([(DataSourceSinus('s1', iphase=0.0), DataSourceSinus('s2', iphase=1.0))])
# mon.add_widget(ewGroup([manc_xy]))

ai = EwAttitudeIndicator([
    DataSourceSinus('s1', iphase=0.0, mult=45),
    DataSourceSinus('s2', iphase=1.0, mult=20)
])

hi = EwHeadingIndicator([DataSourceSinus('s1', iphase=0.0, mult=360)])

rp = EwRelativePosition([
    DataSourceSinus('plane_phi', mult=360),
    DataSourceConst('plane_r', value=60),
    DataSourceSinus('plane_course', iphase=0.0, mult=360),

    DataSourceSinus('base_phi', iphase=0.5, mult=360),
    DataSourceConst('base_r', value=120),
])

mon.add_widget(EwGroup([rp, ai, hi]))

mon.run()
