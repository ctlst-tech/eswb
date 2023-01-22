from sanic import Sanic

from controls.datasources import DataSourceSinus, DataSourceConst
from ws_channel import WsStreamChannel, WsMonitor

app = Sanic("streamer")
app.config.WEBSOCKET_MAX_SIZE = 2 ** 20
app.config.WEBSOCKET_PING_INTERVAL = 20
app.config.WEBSOCKET_PING_TIMEOUT = 20

chan1 = WsStreamChannel([
    DataSourceSinus('plane_phi', mult=360),
    DataSourceConst('plane_r', value=60),
    DataSourceSinus('plane_course', iphase=0.0, mult=360),
    DataSourceSinus('base_phi', iphase=0.5, mult=360),
    DataSourceConst('base_r', value=120),
], freq=1)

chan2 = WsStreamChannel([
    DataSourceConst('plane_r', value=160),
    DataSourceSinus('plane_phi', mult=360),
    DataSourceSinus('base_phi', iphase=0.5, mult=360),
], freq=20)

app.add_task(chan1.loop())
app.add_task(chan2.loop())

mon = WsMonitor(app)
mon.add_channel(chan1, "/chan1")
mon.add_channel(chan2, "/chan2")



