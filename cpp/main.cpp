
#include <chrono>
#include <thread>
#include <iostream>

#include "eswb.hpp"

void clrsrc () {
    std::cout << char(27) << "[2J" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string monitor = "monitor";
    eswb::Bus monitor_bus(monitor, eswb::inter_thread, 2048);

    std::string bus2request = "sens";

    monitor_bus.mkdir(bus2request);

    auto sdtl_bridge = eswb::BridgeSDTL("/tmp/vserial2", 115200, monitor + "/" + bus2request);
    sdtl_bridge.start();

    while(1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        monitor_bus.update_tree();
        clrsrc();
        monitor_bus.print_tree();
    }
}
