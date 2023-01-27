import sys
from controls import *
from controls.datasources import *
from location_map import EwLocationMap
from monitor import Monitor
from relative_position import EwRelativePosition

# DEBUG_PORT = '5588'
# DEBUG_URL = 'http://127.0.0.1:%s' % DEBUG_PORT
# os.environ['QTWEBENGINE_REMOTE_DEBUGGING'] = DEBUG_PORT
# sys.argv.append("--disable-web-security")

mon_bus_name = 'monitor'
telemetry_dir_name = 'telemetry'

mon = Monitor(monitor_bus_name=mon_bus_name, argv=sys.argv)

hi = EwHeadingIndicator([DataSourceSinus('s1', iphase=0.0, mult=360)])

ai = EwAttitudeIndicator([
    DataSourceSinus('s1', iphase=0.0, mult=45),
    DataSourceSinus('s2', iphase=0.5, mult=20)
])

rp = EwRelativePosition([
    DataSourceSinus('plane_phi', mult=360),
    DataSourceConst('plane_r', value=60),
    DataSourceSinus('plane_course', iphase=0.0, mult=360),

    DataSourceSinus('base_phi', iphase=0.5, mult=360),
    DataSourceConst('base_r', value=120),
])

m = EwLocationMap([
    DataSourceConst('riga', value=(57.0764, 24.3308)),
    DataSourceConst('berlin', value=(52.52472910525925, 13.375974062388432)),
    DataSourceConst('paris', value=(48.865383664217475, 2.3363903920362357)),
    DataSourceTimeline('time')
])


mon.add_widget(EwGroup([rp, ai, hi, m]))

mon.run()

