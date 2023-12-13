#include "char_buffer.h"
#include <string.h>

namespace debugger {

namespace {
constexpr char toHex4(uint8_t val) {
    return val < 10 ? val + '0' : val - 10 + 'A';
}
}  // namespace

CharBuffer::CharBuffer(uint8_t size) : _str(new char[size + 1]) {
    _str[0] = 0;
}

CharBuffer::CharBuffer(const char *str) : _str(new char[strlen(str) + 1]) {
    strcpy(_str, str);
}

void CharBuffer::hex4(uint8_t pos, uint8_t val) {
    _str[pos] = toHex4(val & 0xF);
}

void CharBuffer::hex8(uint8_t pos, uint8_t val) {
    hex4(pos, val >> 4);
    hex4(pos + 1, val);
}

void CharBuffer::hex16(uint8_t pos, uint16_t val) {
    hex8(pos, val >> 8);
    hex8(pos + 2, val);
}

void CharBuffer::hex20(uint8_t pos, uint32_t val) {
    hex4(pos, val >> 16);
    hex16(pos + 1, val);
}

void CharBuffer::hex24(uint8_t pos, uint32_t val) {
    hex8(pos, val >> 16);
    hex16(pos + 2, val);
}

void CharBuffer::hex32(uint8_t pos, uint32_t val) {
    hex16(pos, val >> 16);
    hex16(pos + 4, val);
}

void CharBuffer::bits(
        uint8_t pos, uint8_t value, uint8_t mask, const char *letters) {
    while (mask && *letters) {
        _str[pos++] = (value & mask) ? *letters++ : '_';
        mask >>= 1;
    }
}

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
