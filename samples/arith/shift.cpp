#include "shift.hpp"
#include <stdio.h>

void shift(const uint16_t value) {
    auto v = value;
    auto f = right16(v);
    printf("right16(0x%04X) = 0x%04X %s\n", value, v, f.str());
    v = value;
    f = left16(v);
    printf(" left16(0x%04X) = 0x%04X %s\n", value, v, f.str());
}

int main() {
    shift(0);
    shift(1);
    shift(UINT16_MAX);
    shift(INT16_MAX);
    shift(INT16_MIN);

    return 0;
}

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
