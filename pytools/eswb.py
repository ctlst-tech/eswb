#!/usr/bin/env python3

from ctypes import *
import platform
import os
from typing import List

import c_eswb_wrappers as ce


def cstr(s: str):
    _s = s.encode("utf-8")
    return ce.String(_s)


def pstr(s):
    return s.decode("utf-8")


def eswb_exception(text, errcode = 0):
    if errcode == 0:
        errcode_text = ''
    else:
        errmsg = ce.eswb_strerror(errcode)
        errcode_text = ': ' + pstr(errmsg)

    return Exception(f'{text}{errcode_text}')


def eswb_index(i: int):
    return c_uint32(i)


def get_value_from_buf(topic_type, data_ref):
    def castNvalue(ref, p_c_type):
        if p_c_type:
            return cast(ref, POINTER(p_c_type)).contents.value
            # return 'commented out'
        else:
            return 'invalid'

    none_stub = None

    if topic_type == ce.tt_none:
        return none_stub
    elif topic_type == ce.tt_float:
        return castNvalue(data_ref, c_float)
    elif topic_type == ce.tt_double:
        return castNvalue(data_ref, c_double)
    elif topic_type == ce.tt_uint64:
        return castNvalue(data_ref, c_uint64)
    elif topic_type == ce.tt_int64:
        return castNvalue(data_ref, c_int64)
    elif topic_type == ce.tt_uint32:
        return castNvalue(data_ref, c_uint32)
    elif topic_type == ce.tt_int32:
        return castNvalue(data_ref, c_int32)
    elif topic_type == ce.tt_uint16:
        return castNvalue(data_ref, c_uint16)
    elif topic_type == ce.tt_int16:
        return castNvalue(data_ref, c_int16)
    elif topic_type == ce.tt_uint8:
        return castNvalue(data_ref, c_uint8)
    elif topic_type == ce.tt_int8:
        return castNvalue(data_ref, c_int8)
    elif topic_type == ce.tt_string:
        return pstr(cast(data_ref, c_char_p).value)
    elif topic_type == ce.tt_struct:
        return none_stub
    elif topic_type == ce.tt_fifo:
        return none_stub
    elif topic_type == ce.tt_byte_buffer:
        return none_stub
    elif topic_type == ce.tt_dir:
        return none_stub
    elif topic_type == ce.tt_event_queue:
        return none_stub


class TopicHandle:
    def __init__(self, name: str, path: str):
        self.name = name
        self.path = path
        self.td = c_int(0)
        self.type = ce.tt_none
        self.data_ref = pointer(c_int(20)) # FIXME
        self.connected = False

    def connect(self):
        c_td = c_int(0)
        rv = ce.eswb_connect(cstr(self.path), byref(c_td))
        if rv != 0:
            raise eswb_exception(f'eswb_connect to {self.path} failed', rv)
        self.td = c_td

        # eswb_rv_t
        # eswb_get_topic_params(eswb_topic_descr_t td, topic_params_t * params);
        topic_params = ce.topic_params_t()

        rv = ce.eswb_get_topic_params(self.td, byref(topic_params))
        if rv != 0:
            raise eswb_exception(f'eswb_get_topic_params failed', rv)

        self.type = topic_params.type

    def value(self):
        rv = ce.eswb_read(self.td, self.data_ref)
        if rv != 0:
            raise eswb_exception("eswb_read failed", rv)

        rv = get_value_from_buf(self.type, self.data_ref)
        return rv


class Topic:
    def __init__(self, topic: POINTER(ce.topic_t)):
        self.name = pstr(topic.contents.name)
        self.type = topic.contents.type
        self.data_ref = topic.contents.data
        self.children: List[Topic] = []
        self.parent: Topic = None

    def add_child(self, t):
        self.children.append(t)

        t.parent = self

    def add_sibling(self, t):
        self.parent.add_child(t)

    def raw_value(self):
        return get_value_from_buf(self.type, self.data_ref)

    def print(self, show_types=False):
        def print_node(t, indent=0):
            spaces = ' ' * indent * 2
            value = t.raw_value()
            if value is not None:
                if type(value) == float:
                    value = f'{value:.3f}'
                elif type(value) == str:
                    value = f'\"{value}\"'
            else:
                value = ''

            # print(f'{spaces} Name {t.name} Type {ce.c__EA_topic_data_type_t__enumvalues[t.type]} Value {value}')
            # print(f'{spaces} {t.name} = {value:>10}       ({ce.c__EA_topic_data_type_t__enumvalues[t.type]})')
            n = spaces + t.name
            str2show = f'{n:<20} = {value:>15}'
            if show_types:
                str2show += f'    {ce.c__EA_topic_data_type_t__enumvalues[t.type]}'

            print(str2show)
            # if t.first_child is not None:
            #     print_node(t.first_child, indent+1)
            # if t.next_sibling is not None:
            #     print_node(t.next_sibling, indent)
            for t in t.children:
                print_node(t, indent+1)

        print_node(self)


class Bus:
    def __init__(self, name: str, *, bus_type=ce.eswb_non_synced, topics_num: int = 256):
        rv = ce.eswb_create(cstr(name), bus_type, topics_num)
        if rv != 0:
            raise eswb_exception("eswb_create failed", rv)

        self.bus_path = name + '/'
        c_td = c_int(0)
        rv = ce.eswb_connect(cstr(self.bus_path), byref(c_td))
        if rv != 0:
            raise eswb_exception("eswb_connect failed", rv)
        self.root_td = c_td
        self.topic_tree = None

        # FIXME accessing directly a local bus. Dirty.
        self.local_bus_topics_list = ce._libs["eswb"].get("local_bus_topics_list", "cdecl")
        self.local_bus_topics_list.argtypes = [ce.eswb_topic_descr_t]
        self.local_bus_topics_list.restype = ce.topic_t

    def mkdir(self, dirname, path=''):
        path = self.bus_path + path + '/'
        rv = ce.eswb_mkdir(cstr(path), cstr(dirname))
        if rv != 0:
            raise eswb_exception(f'eswb_mkdir for {dirname} at {path} failed', rv)

    def eq_enable(self, queue_size: int, buffer_size: int):
        rv = ce.eswb_event_queue_enable(self.root_td, c_uint32(queue_size), c_uint32(buffer_size))
        if rv != 0:
            raise eswb_exception(f'eswb_event_queue_enable failed', rv)

    def eq_order(self, topic_mask: str, sub_ch: int):
        rv = ce.eswb_event_queue_order_topic(self.root_td, cstr(topic_mask), eswb_index(sub_ch))

        if rv != 0:
            raise eswb_exception(f'eswb_event_queue_order_topic failed', rv)

    def update_tree(self):

        self.local_bus_topics_list.restype = POINTER(ce.topic_t)
        root_topic_raw = self.local_bus_topics_list(ce.eswb_topic_descr_t(abs(self.root_td.value)))

        root_topic = Topic(root_topic_raw)

        #
        # next_tid = ce.eswb_topic_id_t(0)
        # topic_info = ce.topic_extract_t()
        #
        # ce.eswb_get_next_topic_info(self.root_td, byref(next_tid), byref(topic_info))

        def process_children(rt: Topic, raw: POINTER(ce.topic_t)):
            n = raw.contents.first_child
            while n:
                tn = Topic(n)
                rt.add_child(tn)
                process_children(tn, n)
                n = n.contents.next_sibling

        process_children(root_topic, root_topic_raw)
        self.topic_tree = root_topic

    def get_topics_tree(self):
        return self.topic_tree


def EQRBexception(text, errcode=0):
    if errcode == 0:
        errcode_text = ''
    else:
        errmsg = ce.eqrb_strerror(errcode)
        errcode_text = ': ' + pstr(errmsg)

    return Exception(f'{text}: {errcode_text}')


class EQRB_SDTL:
    def __init__(self, *, sdtl_service_name: str, replicate_to_path: str, ch1: str, ch2: str):
        self.sdtl_service_name = sdtl_service_name
        self.replicate_to_path = replicate_to_path
        self.c_replicate_to_path = cstr(replicate_to_path)
        self.c_ch1 = cstr(ch1)
        self.c_ch2 = cstr(ch2)
        self.repl_map_size = 1024

    def start(self):

        rv = ce.eqrb_sdtl_client_connect(cstr(self.sdtl_service_name),
                                         self.c_ch1,
                                         self.c_ch2,
                                         self.c_replicate_to_path,
                                         self.repl_map_size
                                         )

        if rv != 0:
            EQRBexception('failed', rv)


class SDTLchannelType:
    unrel = "unrel"
    rel = "rel"


def SDTLexception(text, errcode=0):
    if errcode == 0:
        errcode_text = ''
    else:
        errmsg = ce.sdtl_strerror(errcode)
        errcode_text = ': ' + pstr(errmsg)

    return Exception(f'{text}{errcode_text}')


class SDTLchannel:
    def __init__(self, *, name: str, ch_id: int, ch_type: str):
        self.name = name
        self.id = ch_id
        self.type = ch_type

    def register(self, service):
        cfg = ce.sdtl_channel_cfg_t()
        cfg.name = cstr(self.name)
        cfg.id = self.id
        cfg.mtu_override = 0
        cfg.type = ce.SDTL_CHANNEL_RELIABLE if self.type == SDTLchannelType.rel else ce.SDTL_CHANNEL_UNRELIABLE

        rv = ce.sdtl_channel_create(service, byref(cfg))
        if rv != 0:
            raise SDTLexception(f'sdtl_channel_create failed', rv)


class SDTLserialService:

    def __init__(self, *, service_name: str, device_path: str,
                 mtu: int, baudrate: int, channels: List[SDTLchannel],):
        self.service_name = service_name
        self.c_service_name = cstr(service_name)  # need to be persistent pointer
        self.device_path = device_path
        self.channels = channels
        self.mtu = mtu
        self.baudrate = baudrate

        self.service_bus_name = 'sdtl'
        self.service_bus = Bus(self.service_bus_name, bus_type=ce.eswb_inter_thread, topics_num=512)

        self.service_ref = POINTER(ce.sdtl_service_t)()

        rv = ce.sdtl_service_init_w(byref(self.service_ref),
                                    self.c_service_name,
                                    cstr(self.service_bus_name),
                                    self.mtu,
                                    8,  # max ch num
                                    cstr('serial')
                                    )
        if rv != 0:
            raise SDTLexception(f'sdtl_service_init failed', rv)

        for c in self.channels:
            c.register(self.service_ref)

        pass

    def start(self):
        media_params = ce.sdtl_media_serial_params_t()
        media_params.baudrate = self.baudrate

        rv = ce.sdtl_service_start(self.service_ref,
                                   cstr(self.device_path),
                                   byref(media_params)
                                   )

        if rv != 0:
            raise SDTLexception(f'sdtl_service_start failed', rv)

    def stop(self):
        pass


def main(command_line=None):
    import time
    import argparse
    import re

    parser = argparse.ArgumentParser('ESWB python based connectivity tool')
    # parser.add_argument(
    #     '--debug',
    #     action='store_true',
    #     help='Print debug info'
    # )

    parser.add_argument(
        '--mtype',
        help='connect to replication data via',
        choices=['tcp', 'serial', 'file'],
        dest='mtype',
        required=True
    )

    parser.add_argument(
        '--path',
        help='host:port / serial_device_path:baudrate / file path',
        dest='path',
        required=True,
    )

    parser.add_argument(
        '--bus',
        help='bus to request (if applicable)',
        dest='bus',
        required=False
    )

    subparsers = parser.add_subparsers(dest='command')
    print_tree = subparsers.add_parser('print', help='print bus state')
    print_tree.add_argument(
        '--wtypes',
        help='show types of topics',
        action='store_true'
    )
    print_tree.add_argument(
        '--debug',
        help='Used for not suppresing EQRB debug info',
        action='store_true'
    )

    args = parser.parse_args(command_line)

    service_bus_name = 'service'
    b = Bus(service_bus_name)

    bus2request = args.bus
    subdir = re.sub('.+:/', '', bus2request)
    b.mkdir(subdir)

    if args.mtype == 'serial':
        (path, baudrate) = args.path.split(':')

        sdtl_service_name = 'sdtl_serial'

        sdtl = SDTLserialService(service_name=sdtl_service_name, device_path=path, mtu=0,
                                 baudrate=int(baudrate),
                                 channels=[
                                     SDTLchannel(name='bus_sync', ch_id=1, ch_type=SDTLchannelType.rel),
                                     SDTLchannel(name='bus_sync_sk', ch_id=2, ch_type=SDTLchannelType.unrel),
                                 ]
                                 )

        sdtl.start()

        eqrb = EQRB_SDTL(sdtl_service_name=sdtl_service_name,
                         replicate_to_path=f'nsb:/{service_bus_name}/{subdir}',
                         ch1='bus_sync',
                         ch2='bus_sync_sk')

        eqrb.start()

    if args.command == 'print':
        show_types = args.wtypes
        period = 2.0 if args.debug else 0.2
        while True:
            time.sleep(period)
            b.update_tree()
            if not args.debug:
                print(chr(27) + "[2J")
            b.topic_tree.print(show_types=show_types)


if __name__ == "__main__":
    main()

