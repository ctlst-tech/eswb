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

