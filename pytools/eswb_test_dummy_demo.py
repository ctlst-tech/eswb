#!/usr/bin/env python3

import sys
from eswbmon import *

mon = Monitor(sys.argv)

#
# socat -d -d pty,link=/tmp/vserial1,raw,echo=0 pty,link=/tmp/vserial2,raw,echo=0
#

mon.bridge(bus2replicate='itb:/conversions')
mon.bridge(bus2replicate='itb:/generators')

mon.add_widget(ewChart(path='generators/sin/out'))
mon.add_widget(ewChart(path='generators/saw/out'))
mon.add_widget(ewChart(name='lin_freq', path='conversions/lin_freq/out'))

mon.run()
