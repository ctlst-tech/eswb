import sys

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

ds_gnss_lat = DataSourceConst(name="gnss_lat", value=57.0764)
ds_gnss_lon = DataSourceConst(name="gnss_lon", value=24.3308)
ds_mag_azimuth = DataSourceSinus(name="az", iphase=0, mult=360)

ds_lat = DataSourceConst(name="gnss_lat", value=57.0784)
ds_lon = DataSourceConst(name="gnss_lat", value=24.3308)
ds_yaw = DataSourceSinus(name="az", iphase=0, mult=25)

m = EwLocationMap([('gnss', ds_gnss_lat, ds_gnss_lon, ds_mag_azimuth),
                   ('est', ds_lat, ds_lon, ds_yaw)])

mon.add_widget(EwGroup([m]))
mon.run()
