from PyQt5 import Qt, QtWidgets, QtSvg
from PyQt5.QtCore import QPointF, QRectF
import os.path


class ColorInterp:
    def __init__(self, c0, c1, step=0.01):
        self.left = c0
        self.right = c1
        self.current = [0.0, 0.0, 0.0, 0.0]
        self.mixture_lever = 0.0
        self.step = step

        self.color_delta = [c1[0] - c0[0],
                            c1[1] - c0[1],
                            c1[2] - c0[2],
                            c1[3] - c0[3]
                            ]

    @staticmethod
    def qcolor(clr):
        return Qt.QColor(Qt.qRgba(
            int(clr[0]),
            int(clr[1]),
            int(clr[2]),
            int(clr[3])))

    def color_get(self):
        self.current[0] = self.left[0] + self.color_delta[0] * self.mixture_lever
        self.current[1] = self.left[1] + self.color_delta[1] * self.mixture_lever
        self.current[2] = self.left[2] + self.color_delta[2] * self.mixture_lever
        self.current[3] = self.left[3] + self.color_delta[3] * self.mixture_lever
        return self.qcolor(self.current)

    def shift_to_right(self):
        self.mixture_lever += self.step
        self.mixture_lever = 1.0 if self.mixture_lever > 1.0 else self.mixture_lever

    def shift_to_left(self):
        self.mixture_lever -= self.step
        self.mixture_lever = 0.0 if self.mixture_lever < 0.0 else self.mixture_lever

    def set_lever(self, lv):
        self.mixture_lever = lv
        self.mixture_lever = 1.0 if self.mixture_lever > 1.0 else self.mixture_lever
        self.mixture_lever = 0.0 if self.mixture_lever < 0.0 else self.mixture_lever

    def set_left(self):
        self.mixture_lever = 0.0
        return self.color_get()

    def set_right(self):
        self.mixture_lever = 1.0
        return self.color_get()


class MyQtWidget(QtWidgets.QWidget):
    def __init__(self, layout_vertical=True, **kwargs):
        super().__init__(**kwargs)
        self.layout = QtWidgets.QBoxLayout(
            QtWidgets.QBoxLayout.TopToBottom if layout_vertical else QtWidgets.QBoxLayout.LeftToRight)
        self.setLayout(self.layout)

    @staticmethod
    def mk_svg(file, z_val=0):
        item = QtSvg.QGraphicsSvgItem(file)
        item.setCacheMode(QtWidgets.QGraphicsItem.CacheMode.NoCache)
        item.setZValue(z_val)
        return item

    @staticmethod
    def translate_point(canvas, pt: QPointF, zero_offset_x=0, zero_offset_y=0):
        cwidth = canvas.device().width()
        cheight = canvas.device().height()
        dx, dy = cwidth / 2 + zero_offset_x, cheight / 2 + zero_offset_y
        return QPointF(pt.x() + dx, pt.y() + dy)

    @staticmethod
    def draw_item(canvas, item, width=0, height=0, zero_offset_x=0, zero_offset_y=0):
        canvas.save()

        cwidth = canvas.device().width()
        cheight = canvas.device().height()

        width = cwidth if width == 0 else width
        height = cheight if height == 0 else height

        dx, dy = cwidth / 2 + zero_offset_x + item.x(), cheight / 2 + zero_offset_y + item.y()

        canvas.translate(dx, dy)
        canvas.rotate(item.rotation())

        # item.x()
        item.renderer().render(canvas, QRectF(-width / 2, -height / 2, width, height))
        canvas.restore()


def rel_path(path, base_path=None):
    if not base_path:
        base_path = os.path.dirname(__file__)
    return f'{base_path}/../{path}'
