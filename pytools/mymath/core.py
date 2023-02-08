import math

from PyQt5.QtCore import QPointF


def to_polar(x, y, scale=1.0, decimals=3):
    pt = QPointF(x, y)
    r = math.sqrt(pt.x() * pt.x() + pt.y() * pt.y()) / scale
    degrees = math.degrees(math.atan2(-y, x)) + 90
    phi = (degrees + 360) % 360
    return [round(phi, decimals), round(r, decimals)]


def to_cartesian(r, phi, scale=1.0, decimals=6):
    _phi_rad = math.radians(phi)
    _x = scale * r * math.sin(_phi_rad)
    _y = scale * r * math.cos(_phi_rad)
    return [round(_x, decimals), round(_y, decimals)]


def translate_point(width, height, pt: QPointF, zero_offset_x=0.0, zero_offset_y=0.0):
    dx, dy = width / 2 + zero_offset_x, height / 2 + zero_offset_y
    return QPointF(pt.x() + dx, pt.y() + dy)


def rev_translate_point(width, height, pt: QPointF, zero_offset_x=0.0, zero_offset_y=0.0):
    dx, dy = width / 2 + zero_offset_x, height / 2 + zero_offset_y
    return QPointF(pt.x() - dx, - (pt.y() - dy))
