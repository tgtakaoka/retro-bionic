#include "div.hpp"
#include <stdio.h>

void udiv(const uint16_t dividend, const uint16_t divisor) {
    auto quotient = dividend;
    uint16_t remainder;
    udiv16(quotient, divisor, remainder);
    printf("udiv16(%6u, %6u) = %6u ... %6u\n", dividend, divisor, quotient,
            remainder);
}

void div(const int16_t dividend, const int16_t divisor) {
    auto quotient = dividend;
    uint16_t remainder;
    div16(quotient, divisor, remainder);
    printf("div16(%6d, %6d) = %6d ... %6u\n", dividend, divisor, quotient,
            remainder);
}

int main() {
    udiv(12345, 10);
    udiv(30000, 100);
    div(30000, 100);
    div(-200, 100);
    div(-30000, -200);
    div(-30000, 78);

    return 0;
}

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
