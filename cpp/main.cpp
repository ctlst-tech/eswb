
#include <chrono>
#include <thread>
#include <iostream>

#include "eswb.hpp"

void clrsrc () {
    std::cout << char(27) << "[2J" << std::endl;
}

int main(int argc, char* argv[]) {
    eswb::Bus service_bus("service", eswb::inter_thread, 2048);

    std::string bus2request = "conversions";

    auto p = service_bus.mkdir(bus2request);

    eswb::Bridge br("itb:/" + bus2request, p);
    br.connect("127.0.0.1");

    while(1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        service_bus.update_tree();
        clrsrc();
        service_bus.print_tree();
    }
}
