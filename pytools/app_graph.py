import sys
from controls import *
from controls.relative_position import *
from controls.datasources import *
from controls.graph3d import EwGraph3D
from monitor import Monitor
from controls.ewvista import EwPyVista

mon_bus_name = 'monitor'
telemetry_dir_name = 'telemetry'

mon = Monitor(monitor_bus_name=mon_bus_name, argv=sys.argv)

# manc_xy = ewCursor([(DataSourceSinus('s1', iphase=0.0), DataSourceSinus('s2', iphase=1.0))])
# mon.add_widget(ewGroup([manc_xy]))
hi = EwHeadingIndicator([DataSourceSinus('s1', iphase=0.0, mult=360)])

g3d = EwGraph3D([])
# v = EwPyVista([])

mon.add_widget(EwGroup([hi, g3d]))

mon.run()
