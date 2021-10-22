#include "string_util.h"

char *outText(char *p, const char *text) {
    while ((*p = *text++) != 0)
        p++;
    return p;
}

static char toHex4(uint8_t val) {
    val &= 0xf;
    return val < 10 ? val + '0' : val - 10 + 'A';
}

char *outHex8(char *p, uint8_t val) {
    *p++ = toHex4(val >> 4);
    *p++ = toHex4(val);
    *p = 0;
    return p;
}

char *outHex16(char *p, uint16_t val) {
    p = outHex8(p, static_cast<uint8_t>(val >> 8));
    return outHex8(p, static_cast<uint8_t>(val));
}

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4: