import pyqtgraph as pg
import pyqtgraph.opengl as gl

pg.mkQApp()

view = gl.GLViewWidget()
view.show()

view.setWindowTitle('pyqtgraph')
view.setCameraPosition(distance=200)

xgrid = gl.GLGridItem()
# ygrid = gl.GLGridItem()
# zgrid = gl.GLGridItem()


# view.addItem(ygrid)
# view.addItem(zgrid)

# xgrid.rotate(90, 0, 1, 0)
# ygrid.rotate(90, 1, 0, 0)
xgrid.scale(10, 10, 1)
# xgrid.scale(0.2, 0.1, 0.1)
# ygrid.scale(0.2, 0.1, 0.1)
# zgrid.scale(0.1, 0.2, 0.1)

view.addItem(xgrid)

if __name__ == '__main__':
    pg.exec()
