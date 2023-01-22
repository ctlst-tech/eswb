#!/usr/bin/env python3
# TODO https://github.com/pyqt/examples

import sys

from PyQt5 import QtCore

import eswb as e
from controls import MyQtWidget, EwBasic, EwTable, EwChart
from controls.datasources import *
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
    def __init__(self, title="ESWB display", bus: e.Bus = None):
        super().__init__(title)

        self.bus = bus
        if self.bus:
            self.timer_print_bus = QtCore.QTimer()
            self.timer_print_bus.setInterval(200)
            self.timer_print_bus.timeout.connect(self.print_bus_tree)
            self.timer_print_bus.start()

    def print_bus_tree(self):
        self.bus.update_tree()
        self.bus.topic_tree.print()


class SdtlTelemetryWidget(MyQtWidget, EwBasic):
    def radraw_handler(self, vals: List[Union[float, int, str, NoDataStub]]):
        pass

    def __init__(self, sdtl_ref: e.SDTLserialService):
        MyQtWidget.__init__(self, layout_vertical=False)
        EwBasic.__init__(self)

        data_service_rx_stat = [DataSourceEswbTopic(name=t.name, path=t.get_path()) for t in
                                sdtl_ref.stat_topics_service_tree.children]

        bytes_receive_source = find_data_source(data_service_rx_stat, 'bytes_received')

        data_service_channels_stat = {}
        for k in sdtl_ref.stat_topics_channels_trees.keys():
            data_service_channels_stat[k] = \
                [DataSourceEswbTopic(name=f'rx_{t.name}', path=t.get_path()) for t in
                 sdtl_ref.stat_topics_channels_trees[k]['rx'].children]
            data_service_channels_stat[k] += \
                [DataSourceEswbTopic(name=f'tx_{t.name}', path=t.get_path()) for t in
                 sdtl_ref.stat_topics_channels_trees[k]['tx'].children]

        self.rx_stat_table = EwTable(caption='SDTL RX', data_sources=data_service_rx_stat)
        self.add_nested(self.rx_stat_table)
        self.layout.addWidget(self.rx_stat_table)

        self.channels_subwidgets = []

        tx_bytes_sum_sources = []

        for cw in data_service_channels_stat.items():
            self.channels_subwidgets.append(EwTable(caption=cw[0], data_sources=cw[1]))
            ds = find_data_source(cw[1], 'tx_bytes')
            if ds:
                tx_bytes_sum_sources.append(ds)
            self.add_nested(self.channels_subwidgets[-1])
            self.layout.addWidget(self.channels_subwidgets[-1])

        self.rx_rate_chart_widget = EwChart([
            DataSourceCalcFilteredRate('rx_speed', bytes_receive_source),
            DataSourceCalcFilteredRate('tx_speed', DataSourceSum('tx_bytes_sum', tx_bytes_sum_sources))
        ], data_range=(0, 10000))

        self.add_nested(self.rx_rate_chart_widget)
        self.layout.addWidget(self.rx_rate_chart_widget)


class EswbMonitor(Monitor):
    def __init__(self, *, monitor_bus_name='monitor', argv=None):
        super().__init__(monitor_bus_name=monitor_bus_name, argv=argv)
        self.bus = e.Bus(monitor_bus_name)
        self.app_window = EswbApplicationWindow(bus=self.bus)
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
        return SdtlTelemetryWidget(self.sdtl_service)

    def add_widget(self, w):
        self.app_window.add_ew(w)
