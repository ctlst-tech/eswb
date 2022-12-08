# Python based tools for ESWB

## eswb.py 

Works as library and standalone tool to read ESWB bus via EQRB protocol.

## eswbmon.py

Library for displaying ESWB parameters via PyQt5.

Build and install ESWB library first:
```
cmake -B build
sudo cmake --build ./build --target install -j8
```

## Dependencies installation Ubuntu

```
sudo apt-get install python3-pyqt5
pip3 install pyqtgraph
```

## eswb_test_dummy_demo.py

Sample script that using eswbmon.py for interacting with eswb_test_dummy 
installable testing executable (TODO need to be acuatlized for SDTL based EQRB).
