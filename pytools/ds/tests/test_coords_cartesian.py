import mymath
from unittest import TestCase

from ds.common import DataSourceCoordsCartesian


# Sample coords:
#
#      •4    |     •1
#            |
#  ----------+-----------
#            |
#      •3    |     •2

class TestDataSourceCartesian(TestCase):
    def test_pt1(self):
        ds = DataSourceCoordsCartesian("cartesian", phi=45, r=mymath.hypot(10, 10), decimals=2)
        [x, y] = ds.read()
        self.assertEqual(10.0, x)
        self.assertEqual(10.0, y)

    def test_pt2(self):
        ds = DataSourceCoordsCartesian("cartesian", phi=45+90, r=mymath.hypot(10, 10), decimals=2)
        [x, y] = ds.read()
        self.assertEqual(10.0, x)
        self.assertEqual(-10.0, y)

    def test_pt3(self):
        ds = DataSourceCoordsCartesian("cartesian", phi=45 + 180, r=mymath.hypot(10, 10), decimals=2)
        [x, y] = ds.read()
        self.assertEqual(-10.0, x)
        self.assertEqual(-10.0, y)

    def test_pt4(self):
        ds = DataSourceCoordsCartesian("cartesian", phi=45 + 270, r=mymath.hypot(10, 10), decimals=2)
        [x, y] = ds.read()
        self.assertEqual(-10.0, x)
        self.assertEqual(10.0, y)
