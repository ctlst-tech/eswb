import asyncio
import json
import time

from typing import List, Union
from sanic import Request, Websocket, Sanic
from controls import DataSourceBasic


class WsStreamChannel:
    def __init__(self, data_sources: List[DataSourceBasic], freq=1.0):
        self.data_sources: List[DataSourceBasic] = data_sources
        self.freq = freq
        self.buffer = []

    def clear_buffer(self):
        self.buffer = []

    def set_data_sources(self, data_sources: List[DataSourceBasic]):
        self.data_sources = data_sources

    def get_frame(self):
        vals = {"timestamp": time.time()}
        for ds in self.data_sources:
            vals[ds.name] = ds.read()
        return vals

    async def loop(self):
        while True:
            await asyncio.sleep(1 / self.freq)
            self.buffer.append(self.get_frame())


class WsMonitor:
    def __init__(self, app: Sanic):
        self._app = app
        self._handlers = []

    @staticmethod
    def feed(chan: WsStreamChannel):
        async def loop(request: Request, ws: Websocket):
            while True:
                json_str = json.dumps({
                    "data": chan.buffer
                })
                chan.clear_buffer()

                await ws.send(json_str)
                # TODO: freq
                await asyncio.sleep(1)

        return loop

    def add_channel(self, chan: WsStreamChannel, path):
        loop = self.feed(chan)
        h = self._app.add_websocket_route(loop, path)
        self._handlers.append([h, chan])

