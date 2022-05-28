#!/usr/bin/env python3

from ctypes import *
import platform
import os
import c_eswb as ce

libname = 'libeswb'

sys = platform.system()

if sys == 'Linux':
    libname += '.so'
elif sys == 'Darwin':
    libname += '.dylib'
else:

    raise "Not supported platform"

# libpath = '../cmake-build-debug/src/lib'
# fullpath = os.path.dirname(os.path.abspath(__file__)) + os.path.sep + libpath + os.path.sep + libname

try:
    eswb = CDLL(libname)
    # eswb = CDLL(fullpath)
    # libc = CDLL('libc.so')
    print("Successfully loaded ", eswb)
except Exception as e:
    print(e)
    print('Make sure lib is installed')


eswb.eswb_strerror.restype = c_char_p

def cstr(s: str):
    _s = s.encode("utf-8")
    return c_char_p(_s)

def pstr(s):
    return s.decode("utf-8")

def eswb_exception(text, errcode = 0):
    if errcode == 0:
        errcode_text = ''
    else:
        errmsg = eswb.eswb_strerror(errcode)
        errcode_text = ': ' + pstr(errmsg)

    return Exception(f'{text}{errcode_text}')

def eswb_index(i: int):
    return c_uint32(i)


def get_value_from_buf(topic_type, data_ref):
    def castNvalue(ref, p_c_type):
        return cast(ref, POINTER(p_c_type)).contents.value

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

    def connect(self):
        c_td = c_int(0)
        rv = eswb.eswb_subscribe(cstr(self.path), byref(c_td))
        if rv != 0:
            raise eswb_exception(f'eswb_subscribe to {self.path} failed', rv)
        self.td = c_td

        # eswb_rv_t
        # eswb_get_topic_params(eswb_topic_descr_t td, topic_params_t * params);
        topic_params = ce.topic_params_t()

        rv = eswb.eswb_get_topic_params(self.td, byref(topic_params))
        if rv != 0:
            raise eswb_exception(f'eswb_get_topic_params failed', rv)

        self.type = topic_params.type

    def value(self):
        rv = eswb.eswb_read(self.td, self.data_ref)
        if rv != 0:
            raise eswb_exception("eswb_read failed", rv)

        return get_value_from_buf(self.type, self.data_ref)



class Topic:
    def __init__(self, topic: POINTER(ce.topic_t)):
        self.name = pstr(topic.contents.name)
        self.type = topic.contents.type
        self.data_ref = topic.contents.data
        self.first_child = None
        self.next_sibling = None
        self.parent = None

    def add_child(self, t):
        if self.first_child is None:
            self.first_child = t
        else:
            self.first_child.add_sibling(t)

        t.parent = self

    def add_sibling(self, t):
        if self.next_sibling is None:
            self.next_sibling = t
        else:
            n = self.next_sibling
            while n.next_sibling is not None:
                n = n.next_sibling

            n.next_sibling = t

        t.parent = self.parent

    # def iterator(self, call, n = None):

    def raw_value(self):
        return get_value_from_buf(self.type, self.data_ref)

    def print(self, show_types = False):
        def print_node(t, indent = 0):
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
            if t.first_child is not None:
                print_node(t.first_child, indent+1)
            if t.next_sibling is not None:
                print_node(t.next_sibling, indent)

        print_node(self)


class Bus:
    def __init__(self, name: str, topics_num: int = 256):
        rv = eswb.eswb_create(cstr(name), ce.eswb_non_synced, topics_num)
        if rv != 0:
            raise eswb_exception("eswb_create failed", rv)

        self.bus_path = 'nsb:/' + name + '/'
        c_td = c_int(0)
        rv = eswb.eswb_subscribe(cstr(self.bus_path), byref(c_td))
        if rv != 0:
            raise eswb_exception("eswb_subscribe failed", rv)
        self.root_td = c_td
        self.topic_tree = None

    def mkdir(self, dirname, path=''):
        path = self.bus_path + path + '/'
        rv = eswb.eswb_mkdir(cstr(path), cstr(dirname))
        if rv != 0:
            raise eswb_exception(f'eswb_mkdir for {dirname} at {path} failed', rv)

    def eq_enable(self, queue_size: int, buffer_size: int):
        rv = eswb.eswb_event_queue_enable(self.root_td,
                                          c_uint32(queue_size),
                                          c_uint32(buffer_size))
        if rv != 0:
            raise eswb_exception(f'eswb_event_queue_enable failed', rv)

    def eq_order(self, topic_mask: str, sub_ch: int):
        rv = eswb.eswb_event_queue_order_topic(self.root_td, cstr(topic_mask), eswb_index(sub_ch))

        if rv != 0:
            raise eswb_exception(f'eswb_event_queue_order_topic failed', rv)

    def update_tree(self):
        eswb.local_bus_topics_list.restype = POINTER(ce.topic_t)
        root_topic_raw = eswb.local_bus_topics_list(ce.eswb_topic_descr_t(abs(self.root_td.value)))

        root_topic = Topic(root_topic_raw)

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


class EQRBtcp:
    def __init__(self, bus2replicate: str, replicate_to_path: str):
        self.bus2replicate = bus2replicate
        self.replicate_to_path = replicate_to_path

        self.client_handler = c_void_p(0)
        eswb.eqrb_tcp_client_create(byref(self.client_handler))

    def connect(self, addr, repl_map_size=1024):
        err_msg = create_string_buffer(256)
        rv = eswb.eqrb_tcp_client_connect(self.client_handler, cstr(addr),
                                            cstr(self.bus2replicate),
                                            cstr(self.replicate_to_path),
                                            repl_map_size,
                                            err_msg)
        if rv != 0:
            raise Exception (f'connect failed: {ce.eqrb_rv_t__enumvalues[rv]} ({rv}): {pstr(err_msg.value)}') #pstr(libc.strerror(skt_err))

    def close(self):
        eswb.eqrb_tcp_client_close(self.client_handler)

class EQRBserial:
    def __init__(self, replicate_to_path: str):
        self.replicate_to_path = replicate_to_path

    def connect(self, path, baudrate, repl_map_size=1024):
        rv = eswb.eqrb_serial_client_connect(cstr(path), baudrate,
                                                cstr(self.replicate_to_path),
                                                repl_map_size)

        if rv != 0:
            raise Exception (f'connect failed: {ce.eqrb_rv_t__enumvalues[rv]} ({rv})') #pstr(libc.strerror(skt_err))


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
        required=True
    )

    subparsers = parser.add_subparsers(dest='command')
    print_tree = subparsers.add_parser('print', help='print bus state')
    print_tree.add_argument(
        '--wtypes',
        help='show types of topics',
        action='store_true'
    )

    args = parser.parse_args(command_line)


    service_bus_name = 'service'
    b = Bus(service_bus_name)

    bus2request = args.bus
    subdir = re.sub('.+:/', '', bus2request)
    b.mkdir(subdir)

    if args.mtype == 'tcp':
        client = EQRBtcp(bus2request, f'nsb:/{service_bus_name}/{subdir}')
        client.connect(args.path)
    elif args.mtype == 'serial':
        (path, baudrate) = args.path.split(':')
        serial_client = EQRBserial(f'nsb:/{service_bus_name}/{subdir}')
        serial_client.connect(path, int(baudrate))

    if args.command == 'print':
        show_types = args.wtypes
        while True:
            time.sleep(0.2)
            b.update_tree()
            print(chr(27) + "[2J")
            b.topic_tree.print(show_types=show_types)


if __name__ == "__main__":
    main()

