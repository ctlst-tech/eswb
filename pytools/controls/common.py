from PyQt5 import Qt


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
