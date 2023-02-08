import math

import mymath
from unittest import TestCase

from ds.common import DataSourceCoordsPolar


# Sample coords:
#
#      •4    |     •1
#            |
#  ----------+-----------
#            |
#      •3    |     •2

class TestDataSourcePolar(TestCase):
    def test_pt1(self):
        decimals = 3
        polar_ds = DataSourceCoordsPolar("to_polar", 10, 10, decimals=decimals)
        [phi, r] = polar_ds.read()
        self.assertEqual(45.0, phi)
        self.assertEqual(round(math.hypot(10, 10), decimals), r)

    def test_pt2(self):
        decimals = 3
        polar_ds = DataSourceCoordsPolar("to_polar", x=10, y=-10, decimals=decimals)
        [phi, r] = polar_ds.read()
        self.assertEqual(phi, 45.0 + 90.0)
        self.assertEqual(round(math.hypot(10, 10), decimals), r)

    def test_pt3(self):
        decimals = 3
        polar_ds = DataSourceCoordsPolar("to_polar", x=-10, y=-10, decimals=decimals)
        [phi, r] = polar_ds.read()
        self.assertEqual(phi, 45.0 + 180.0)
        self.assertEqual(r, round(math.hypot(10, 10), decimals))

    def test_pt4(self):
        decimals = 3
        polar_ds = DataSourceCoordsPolar("to_polar", x=-10, y=10, decimals=decimals)
        [phi, r] = polar_ds.read()
        self.assertEqual(phi, 45.0 + 270.0)
        self.assertEqual(round(math.hypot(10, 10), decimals), r)
