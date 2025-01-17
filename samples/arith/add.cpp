#include "add.hpp"
#include <stdio.h>

void add(const int16_t summand, const int16_t addend) {
    auto addition = summand;
    const auto f = add16(addition, addend);
    printf("add16(%6d, %6d) = %6d %s\n", summand, addend, addition, f.str());
}

int main() {
    add(18000, 28000);
    add(18000, -18000);
    add(-18000, -18000);

    return 0;
}

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
