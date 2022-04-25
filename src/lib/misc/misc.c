//
// Created by goofy on 06.12.2020.
//

#include <string.h>

#include "misc.h"

char *indent(int depth) {
    static char rv[60];
    rv[0] = 0;

    for (int i=0; i < depth; i++) {
        strcat(rv, "    ");
    }

    return rv;
}
