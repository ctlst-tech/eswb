#!/usr/bin/env python3
from eswbmon import *

mon = EswbMonitor(argv=sys.argv)

#
# socat -d -d pty,link=/tmp/vserial1,raw,echo=0 pty,link=/tmp/vserial2,raw,echo=0
#

# mon.bridge(bus2replicate='itb:/conversions')
# mon.bridge(bus2replicate='itb:/generators')

mon.add_widget(EwChart(path='generators/sin/out', data_sources=[]))
mon.add_widget(EwChart(path='generators/saw/out', data_sources=[]))
mon.add_widget(EwChart(name='lin_freq', path='conversions/lin_freq/out', data_sources=[]))

mon.run()
