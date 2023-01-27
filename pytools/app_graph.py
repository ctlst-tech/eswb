import sys
from ds import *
from ew import *
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

ai = EwAttitudeIndicator([
    DataSourceSinus('s1', iphase=0.0, mult=45),
    DataSourceSinus('s2', iphase=1.0, mult=20)
])

rp = EwRelativePosition([
    DataSourceSinus('plane_phi', mult=360),
    DataSourceConst('plane_r', value=60),
    DataSourceSinus('plane_course', iphase=0.0, mult=360),

    DataSourceSinus('base_phi', iphase=0.5, mult=360),
    DataSourceConst('base_r', value=120),
])

mon.add_widget(EwGroup([rp, ai, hi, g3d]))

mon.run()
