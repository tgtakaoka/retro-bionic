#include "mul.hpp"
#include <stdio.h>

void mul(const int16_t multiplicand, const int16_t multiplier) {
    auto product = multiplicand;
    mul16(product, multiplier);
    printf("mul16(%6d, %6d) = %6d\n", multiplicand, multiplier, product);
}

int main() {
    mul(100, 300);
    mul(-200, 100);
    mul(100, -300);
    mul(-200, -100);
    mul(300, -200);

    return 0;
}

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
