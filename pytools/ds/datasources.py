from abc import abstractmethod
from typing import List, Union, Dict
import time
import math

from PyQt5.QtWidgets import QLineEdit


class NoDataStub:
    def __init__(self, err_msg):
        self.err_msg = err_msg


class DataSourceBasic:
    def __init__(self, name, *, mult=1, rng=None, demo_mode=False):
        self.name = name
        self.range = rng
        self.mult = mult
        self.demo_mode = demo_mode

    @abstractmethod
    def connect(self):
        pass

    @abstractmethod
    def read(self) -> Union[float, int, str, NoDataStub]:
        pass


class DataSourceSum(DataSourceBasic):
    def __init__(self, name, sources: List[DataSourceBasic], **kwargs):
        super().__init__(name, **kwargs)
        self.data_sources = sources

    def connect(self):
        for s in self.data_sources:
            s.connect()

    def read(self) -> Union[float, int, str, NoDataStub]:
        rv = 0
        for s in self.data_sources:
            v = s.read()
            if isinstance(v, NoDataStub):
                return v
            else:
                rv += v
        return rv


class DataSourceMath(DataSourceBasic):
    def __init__(self, name, sources: List[DataSourceBasic], math_lmb, **kwargs):
        super().__init__(name, **kwargs)
        self.data_sources = sources
        self.lmb = math_lmb

    def connect(self):
        for s in self.data_sources:
            s.connect()

    def read(self) -> Union[float, int, str, NoDataStub]:
        args = []
        for s in self.data_sources:
            v = s.read()
            if isinstance(v, NoDataStub):
                return v
            else:
                args.append(v)

        return self.lmb(args)


class DataSourceEnum(DataSourceBasic):
    def __init__(self, name, sources: List[DataSourceBasic], enum_table: Dict[int, str], **kwargs):
        super().__init__(name, **kwargs)
        self.data_sources = sources
        self.enum_table = enum_table

    def connect(self):
        for s in self.data_sources:
            s.connect()

    def read(self) -> Union[float, int, str, NoDataStub]:
        v = self.data_sources[0].read()
        if isinstance(v, NoDataStub):
            return v
        else:
            rv = ''
            if not isinstance(v, int):
                rv = f'inv type ({v})'
            else:
                if v in self.enum_table:
                    rv = self.enum_table[v]
                else:
                    rv = f'unknown value ({v})'

        return rv


class DataSourceInvert(DataSourceBasic):
    def __init__(self, name, source: DataSourceBasic, **kwargs):
        super().__init__(name, **kwargs)
        self.data_sources = [source]

    def connect(self):
        self.data_sources[0].connect()

    def read(self) -> Union[float, int, str, NoDataStub]:
        v = self.data_sources[0].read()
        if isinstance(v, NoDataStub):
            return v
        else:
            return -v


class DataSourceCalcFilteredRate(DataSourceBasic):
    def __init__(self, name, data_source: DataSourceBasic, factor=0.01, **kwargs):
        super().__init__(name, **kwargs)
        self.data_source = data_source
        self.initial = False
        self.filtered_value = 0
        self.previous_value = 0
        self.previous_time = 0
        self.factor = factor

    def connect(self):
        self.data_source.connect()

    def read(self) -> Union[float, int, str, NoDataStub]:
        current_value = self.data_source.read()

        if isinstance(current_value, NoDataStub):
            return current_value

        curr_time = time.time()

        if not self.initial:
            self.initial = True
            self.filtered_value = 0
        else:
            delta_time = curr_time - self.previous_time
            deriative = (current_value - self.previous_value) / delta_time
            self.filtered_value = self.filtered_value - self.factor * (self.filtered_value - deriative)

        self.previous_value = current_value
        self.previous_time = curr_time

        return self.filtered_value


class DataSourceConst(DataSourceBasic):
    def __init__(self, name, *, value=0.0, **kwargs):
        super().__init__(name, **kwargs)
        self._value = value

    def connect(self):
        pass

    def read(self) -> Union[float, int, str, NoDataStub]:
        return self._value


class DataSourceTimeline(DataSourceBasic):
    def __init__(self, name, **kwargs):
        super().__init__(name, **kwargs)
        self.time = 0
        self.delta_time = 1

    def connect(self):
        pass

    def read(self) -> Union[float, int, str, NoDataStub]:
        self.time += self.delta_time
        return self.time


class DataSourceSinus(DataSourceBasic):
    def __init__(self, name, *, omega=1.0, iphase=0.0, bias=0.0, **kwargs):
        super().__init__(name, **kwargs)
        self.time = 0
        self.delta_time = 0.01
        self.omega = omega
        self.bias = bias
        self.initial_phase = iphase

    def connect(self):
        pass

    def read(self) -> Union[float, int, str, NoDataStub]:
        self.time += self.delta_time
        return self.mult * math.sin(self.initial_phase + self.omega * self.time) + self.bias


def find_data_source(lst: List[DataSourceBasic], name: str):
    for s in lst:
        if s.name == name:
            return s

    return None
