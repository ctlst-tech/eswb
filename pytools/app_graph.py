import pyqtgraph as pg
import pyqtgraph.opengl as gl

pg.mkQApp()

view = gl.GLViewWidget()
view.show()

xgrid = gl.GLGridItem()
ygrid = gl.GLGridItem()
zgrid = gl.GLGridItem()
view.addItem(xgrid)
view.addItem(ygrid)
view.addItem(zgrid)

xgrid.rotate(90, 0, 1, 0)
ygrid.rotate(90, 1, 0, 0)

xgrid.scale(0.2, 0.1, 0.1)
ygrid.scale(0.2, 0.1, 0.1)
zgrid.scale(0.1, 0.2, 0.1)