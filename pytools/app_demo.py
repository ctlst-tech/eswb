#!/usr/bin/env python3

import sys
from datasources import *
from controls import *
from monitor import Monitor

mon_bus_name = 'monitor'
telemetry_dir_name = 'telemetry'

mon = Monitor(monitor_bus_name=mon_bus_name, argv=sys.argv)

# manc_xy = ewCursor([(DataSourceSinus('s1', iphase=0.0), DataSourceSinus('s2', iphase=1.0))])
# mon.add_widget(ewGroup([manc_xy]))

ai = ewAttitudeIndicator([
    DataSourceSinus('s1', iphase=0.0, mult=45),
    DataSourceSinus('s2', iphase=1.0, mult=20)
])

hi = ewHeadingIndicator([DataSourceSinus('s1', iphase=0.0, mult=360)])

mon.add_widget(ewGroup([ai, hi]))

mon.run()
