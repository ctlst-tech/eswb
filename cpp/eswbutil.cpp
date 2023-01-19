
#include <chrono>
#include <iostream>
#include <list>
#include <map>
#include <queue>
#include <thread>
#include <vector>

#include "eswb.hpp"

class option;

std::map<std::string, option *> options;

class option {
    std::string alias;
    std::vector<std::string> args;
    unsigned min_args;
    unsigned max_args;

public:
    option(const char *opt_alias, unsigned min_args_, unsigned max_args_)
        : alias(opt_alias), min_args(min_args_), max_args(max_args_) {
        options.insert(std::make_pair("--" + std::string(opt_alias), this));
    }

    option() {}

    bool is(const std::string &als) { return alias == als; }

    void add_args(std::queue<std::string> &args2add) {
        while (!args2add.empty()) {
            std::string a = args2add.front();
            if (a.rfind("--", 0) == 0) {
                // next opt starts
                break;
            }

            args.push_back(a);
            args2add.pop();
        }

        if (args.size() < min_args) {
            throw std::string(
                "Option " + alias + " expects " + std::to_string(min_args) +
                " min arguments, got " + std::to_string(args.size()));
        }
    }

    std::string arg_as_string(unsigned arg_num,
                              const std::string &default_val = "") {
        if (arg_num >= args.size()) {
            return default_val;
        }
        return args[arg_num];
    }

    int arg_as_int(unsigned arg_num, int default_val = 0) {
        if (arg_num >= args.size()) {
            return default_val;
        }
        return std::stoi(args[arg_num]);
    }
};

std::list<option *> parse_options(int argc, char *argv[]) {
    std::list<option *> rv;

    std::queue<std::string> cli_args;
    for (int i = 0; i < argc; i++) {
        cli_args.push(std::string(argv[i]));
    }

    while (!cli_args.empty()) {
        auto o = options.find(cli_args.front());
        if (o != options.end()) {
            cli_args.pop();
            option *new_opt = new option();
            *new_opt = *o->second;
            new_opt->add_args(cli_args);
            rv.push_back(new_opt);
        } else {
            std::cerr << "Unknown option: " << cli_args.front() << std::endl;
            cli_args.pop();
        }
    }

    return rv;
}

void clrsrc() {
    std::cout << char(27) << "[2J" << std::endl;
}

void read_sdtl_bridge_serial(const std::string &path, unsigned baudrate) {
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
        clrsrc();
        monitor_bus.print_tree();
    }
}

void init_options() {
    new option("read_serial", 1, 2);
};

int main(int argc, char *argv[]) {
    init_options();
    try {
        auto opts = parse_options(argc - 1, &argv[1]);

        for (auto o : opts) {
            if (o->is("read_serial")) {
                read_sdtl_bridge_serial(o->arg_as_string(0),
                                        o->arg_as_int(1, 115200));
            }
        }
    } catch (const std::string &s) {
        std::cerr << s << std::endl;
    }
}
