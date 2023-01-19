#include "sdtl.hpp"

void eswb::read_sdtl_bridge_serial(const std::string &path, unsigned baudrate) {
    std::string monitor = "monitor";
    eswb::Bus monitor_bus(monitor, eswb::inter_thread, 2048);
    std::string bus2request = "sens";

    monitor_bus.mkdir(bus2request);

    auto sdtl_bridge =
        eswb::BridgeSDTL(path, baudrate, monitor + "/" + bus2request);
    auto rv = sdtl_bridge.start();
    if (rv != eqrb_rv_ok) {
        std::cerr << "sdtl_bridge start error: " << eqrb_strerror(rv)
                  << std::endl;
        return;
    }

    while (1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        monitor_bus.update_tree();
        eswb::clrsrc();
        monitor_bus.print_tree();
    }
}

void eswb::clrsrc() {
    std::cout << char(27) << "[2J" << std::endl;
}
