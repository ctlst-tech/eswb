import sys
from controls import *
from controls.datasources import *
from controls.graph3d import EwGraph3D
from monitor import Monitor

mon_bus_name = 'monitor'
telemetry_dir_name = 'telemetry'

mon = Monitor(monitor_bus_name=mon_bus_name, argv=sys.argv)

hi = EwHeadingIndicator([DataSourceSinus('s1', iphase=0.0, mult=360)])

g3d = EwGraph3D([
    DataSourceSinus('s1', iphase=0.0, mult=10.),
    DataSourceSinus('s2', iphase=0.3, mult=10.),
    DataSourceSinus('s3', iphase=0.6, mult=10.)
])

mon.add_widget(EwGroup([hi, g3d]))

mon.run()
