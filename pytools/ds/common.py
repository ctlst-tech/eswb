from typing import Union

from mymath import to_cartesian, to_polar
from .datasources import DataSourceBasic, NoDataStub


class DataSourceCoordsCartesian(DataSourceBasic):
    def __init__(self, name, phi, r, scale=1.0, decimals=3, **kwargs):
        super().__init__(name, **kwargs)
        self._phi = phi
        self._r = r
        self._scale = scale
        self._decimals = decimals

    def connect(self):
        pass

    def read(self) -> Union[float, int, str, NoDataStub]:
        return to_cartesian(phi=self._phi,
                            r=self._r,
                            scale=self._scale,
                            decimals=self._decimals)


class DataSourceCoordsPolar(DataSourceBasic):
    def __init__(self, name, x, y, scale=1.0, decimals=3, **kwargs):
        super().__init__(name, **kwargs)
        self._x = x
        self._y = y
        self._scale = scale
        self._decimals = decimals

    def connect(self):
        pass

    def read(self) -> Union[float, int, str, NoDataStub]:
        return to_polar(x=self._x, y=self._y,
                        scale=self._scale,
                        decimals=self._decimals)
