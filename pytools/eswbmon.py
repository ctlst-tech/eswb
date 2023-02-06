#!/usr/bin/env python3
# TODO https://github.com/pyqt/examples

import sys
from typing import Dict, List, Union

from PyQt5 import QtCore

import eswb as e
from ds.datasources import find_data_source
from ew import *
from ds import *
from monitor import Monitor, ApplicationWindow


class DataSourceEswbTopic(DataSourceBasic):
    def __init__(self, name, path, **kwargs):
        super().__init__(name, **kwargs)
        self.topic_handle: e.TopicHandle = e.TopicHandle(name, path)

    def connect(self):
        self.topic_handle.connect()

    def read(self):
        try:
            value = self.mult * self.topic_handle.value()
        except Exception as E:
            value = NoDataStub(E.args[0])
            try:
                self.topic_handle.connect()
            except Exception as E:
                # redefine message
                value.err_msg = E.args[0]

        return value


class EswbApplicationWindow(ApplicationWindow):
    def __init__(self, title="ESWB display", bus: e.Bus = None, tabs=False):
        super().__init__(title=title, tabs=tabs)

        self.bus = bus
        if self.bus:
            self.timer_print_bus = QtCore.QTimer()
            self.timer_print_bus.setInterval(200)
            self.timer_print_bus.timeout.connect(self.print_bus_tree)
            self.timer_print_bus.start()

    def print_bus_tree(self):
        self.bus.update_tree()
        self.bus.topic_tree.print()


class SdtlTelemetryData(EwBasic):
    def radraw_handler(self, vals: List[Union[float, int, str, NoDataStub]], vals_map: Dict):
        pass

    def __init__(self, sdtl_ref: e.SDTLserialService):
        EwBasic.__init__(self)

        self.data_service_rx_stat = [DataSourceEswbTopic(name=t.name, path=t.get_path()) for t in
                                sdtl_ref.stat_topics_service_tree.children]

        self.bytes_receive_source = find_data_source(self.data_service_rx_stat, 'bytes_received')

        self.data_service_channels_stat = {}
        for k in sdtl_ref.stat_topics_channels_trees.keys():
            self.data_service_channels_stat[k] = \
                [DataSourceEswbTopic(name=f'rx_{t.name}', path=t.get_path()) for t in
                 sdtl_ref.stat_topics_channels_trees[k]['rx'].children]
            self.data_service_channels_stat[k] += \
                [DataSourceEswbTopic(name=f'tx_{t.name}', path=t.get_path()) for t in
                 sdtl_ref.stat_topics_channels_trees[k]['tx'].children]

        self.tx_bytes_sum_sources = []

        for cw in self.data_service_channels_stat.items():
            ds = find_data_source(cw[1], 'tx_bytes')
            if ds:
                self.tx_bytes_sum_sources.append(ds)

        self.tx_bytes_sum_data = DataSourceSum('tx_bytes_sum', self.tx_bytes_sum_sources)

        self.rx_speed = DataSourceCalcFilteredRate('rx_speed', self.bytes_receive_source)
        self.tx_speed = DataSourceCalcFilteredRate('tx_speed', self.tx_bytes_sum_data)

        self.rx_speed_fast = DataSourceCalcFilteredRate('rx_speed_fast', self.bytes_receive_source, factor=0.1)
        self.tx_speed_fast = DataSourceCalcFilteredRate('tx_speed_fast', self.tx_bytes_sum_data, factor=0.1)


class SdtlTelemetryWidget(MyQtWidget, SdtlTelemetryData):
    def radraw_handler(self, vals: List[Union[float, int, str, NoDataStub]], vals_map: Dict):
        pass

    def __init__(self, *, sdtl_ref: e.SDTLserialService, **kwargs):
        MyQtWidget.__init__(self, layout_vertical=False, sdtl_ref=sdtl_ref, **kwargs)
        SdtlTelemetryData.__init__(self, sdtl_ref)

        self.rx_stat_table = EwTable(caption='SDTL RX', data_sources=self.data_service_rx_stat)
        self.add_nested(self.rx_stat_table)
        self.layout.addWidget(self.rx_stat_table)

        self.channels_subwidgets = []

        for cw in self.data_service_channels_stat.items():
            self.channels_subwidgets.append(EwTable(caption=cw[0], data_sources=cw[1]))
            self.add_nested(self.channels_subwidgets[-1])
            self.layout.addWidget(self.channels_subwidgets[-1])

        self.rx_rate_chart_widget = EwChart([
            self.rx_speed,
            self.tx_speed
        ], data_range=(0, 10000))

        self.add_nested(self.rx_rate_chart_widget)
        self.layout.addWidget(self.rx_rate_chart_widget)


class SdtlTelemetrySmallWidget(MyQtWidget, SdtlTelemetryData):
    def __init__(self, *, sdtl_ref: e.SDTLserialService, **kwargs):
        MyQtWidget.__init__(self, layout_vertical=False, sdtl_ref=sdtl_ref, **kwargs)
        EwBasic.__init__(self)

        # def __init__(self, *, data_source: DataSourceBasic, max, min = 0, color, **kwargs):
        self.rx_lamp = EwLamp(data_source=self.rx_speed_fast, max=3000, color=(000, 200, 0))
        self.tx_lamp = EwLamp(data_source=self.tx_speed_fast, max=3000, color=(200, 000, 0))

        self.add_nested(self.rx_lamp)
        self.add_nested(self.tx_lamp)

        self.layout.addWidget(self.rx_lamp)
        self.layout.addWidget(self.tx_lamp)


class EswbMonitor(Monitor):
    def __init__(self, *, monitor_bus_name='monitor', argv=None, tabs=False):
        super().__init__(monitor_bus_name=monitor_bus_name, argv=argv)
        self.bus = e.Bus(monitor_bus_name)
        self.app_window = EswbApplicationWindow(bus=self.bus, tabs=tabs)
        self.sdtl_service: e.SDTLserialService | None = None
        pass

    def connect(self):
        self.app_window.connect()

    def show(self):
        self.app_window.show()

    def run(self):
        self.connect()
        self.show()
        sys.exit(self.app.exec_())

    def mkdir(self, dirname):
        self.bus.mkdir(dirname)

    def bridge_sdtl(self, *, path, baudrate, bridge_to):
        sdtl_service_name = 'sdtl_serial'
        self.sdtl_service = e.SDTLserialService(service_name=sdtl_service_name, device_path=path, mtu=0,
                                                baudrate=int(baudrate),
                                                channels=[
                                                    e.SDTLchannel(name='bus_sync', ch_id=1,
                                                                  ch_type=e.SDTLchannelType.rel),
                                                    e.SDTLchannel(name='bus_sync_sk', ch_id=2,
                                                                  ch_type=e.SDTLchannelType.unrel),
                                                ]
                                                )

        self.sdtl_service.start()

        eqrb = e.EQRB_SDTL(sdtl_service_name=sdtl_service_name,
                           replicate_to_path=f'{self.service_bus_name}/{bridge_to}',
                           ch1='bus_sync',
                           ch2='bus_sync_sk')

        eqrb.start()

    def get_stat_widget(self):
        return SdtlTelemetryWidget(sdtl_ref=self.sdtl_service)

    def get_small_widget(self):
        return SdtlTelemetrySmallWidget(sdtl_ref=self.sdtl_service)

    def add_widget(self, w):
        self.app_window.add_ew(w)
