#include "sub.hpp"
#include <stdio.h>

void sub(const int16_t minuend, const int16_t subtrahend) {
    auto subtraction = minuend;
    const auto f = sub16(subtraction, subtrahend);
    printf("sub16(%6d, %6d) = %6d %s\n", minuend, subtrahend, subtraction,
            f.str());
}

int main() {
    sub(-18000, -28000);
    sub(-18000, 18000);
    sub(-28000, -18000);

    return 0;
}

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
