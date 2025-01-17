#include "neg.hpp"
#include <stdio.h>

void neg(const int16_t value) {
    auto negation = static_cast<uint16_t>(value);
    const auto f = neg16(negation);
    printf("neg16(%6d) = %6d %s\n", value, static_cast<int16_t>(negation),
            f.str());
}

int main() {
    neg(0);
    neg(1);
    neg(-1);
    neg(INT16_MAX);
    neg(INT16_MIN + 1);

    return 0;
}

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
