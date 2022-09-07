//
// Created by Ivan Makarov on 23/3/22.
//

#include "tooling.h"

std::string gen_name() {
    static int index = 1;

    int i = index;
    std::string name = "";
    char c[1];
    do {
        c[0] = i % 5;
        i = i / 5;
        c[0] += 'a';
        name = std::string(c) + name;
    } while (i > 0);

    index++;

    return name;
}

PseudoTopic *gen_folder(PseudoTopic *f) {
    auto rv = new PseudoTopic(gen_name());
    if (f != nullptr) {
        rv->add_subtopic(f);
    }
    return rv;
}


void gen_data(uint8_t *d, size_t s) {
    for (size_t i = 0; i < s; i++) {
        d[i] = rand();
    }
}


PseudoTopic *create_bus_and_arbitrary_hierarchy(eswb_type_t bus_type, const std::string &bus_name) {

    std::string prefix = eswb_get_bus_prefix(bus_type);

    eswb_rv_t rv = eswb_create(bus_name.c_str(), bus_type, 100);
    REQUIRE(rv == eswb_e_ok);

    auto *bus = new PseudoTopic(bus_name);

    bus->set_path_prefix(prefix);

    bus->add_subtopic(gen_folder(gen_folder(gen_folder())));
    bus->add_subtopic(gen_folder(gen_folder(gen_folder())));
    bus->add_subtopic(gen_folder(gen_folder(gen_folder())));

    auto f = gen_folder();
    for (int i = 0; i < 6; i++) {
        f->add_subtopic(gen_folder());
    }
    bus->add_subtopic(f);
    bus->create_as_dirs(false);

    return bus;
}
