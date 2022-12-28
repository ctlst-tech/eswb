# ESWB - Embedded Software Bus

ESWB is a portable pub/sub middleware that creates a uniform way of functions to communicate between each other: 
inside thread, between threads, between processes. ESWB is designed to be the major and the only candidate to do inter process 
communication inside project. Its purpose is to define architecture template, engage reusability and provide tools to build and debug 
complex embedded systems like drones and the rest of the robotics.

ESWB adresses the following problems:
- software connectivity and IPC calls are too diverse, and diversity rises over time, architecture drifts;
- eventually project might get into OS jail;
- functions are coupled together, and it is hard to test them anywhere but the target device
  with a limited debugging scope;
- developers have to create own data logging and telemetry services.

ESWB is the foundation of [**c-atom**](https://github.com/ctlst-tech/c-atom) library.

## Documentation

[Check it here](https://docs.ctlst.app/eswb/intro.html)

## Build

ESWB uses [Catch2](https://github.com/catchorg/Catch2) as a testing framework. Here is a quick tip on installing it:

```shell
git clone https://github.com/catchorg/Catch2.git
cd Catch2
git checkout v3.0.1
cmake -Bbuild -H. -DBUILD_TESTING=OFF
sudo cmake --build build/ --target install
```
