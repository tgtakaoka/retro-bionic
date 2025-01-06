#include "char_buffer.h"
#include <string.h>

namespace debugger {

void CharBuffer::set(const char *str) {
    strncpy(_str, str, _max);
    _str[_max] = 0;
}

namespace {
constexpr char toHex4(uint_fast8_t val) {
    return val < 10 ? val + '0' : val - 10 + 'A';
}
constexpr char toOct3(uint_fast8_t val) {
    return val + '0';
}
}  // namespace

void CharBuffer::hex1(uint_fast8_t pos, uint_fast8_t val) {
    _str[pos] = val ? '1' : '0';
}

void CharBuffer::hex4(uint_fast8_t pos, uint_fast8_t val) {
    _str[pos] = toHex4(val & 0xF);
}

void CharBuffer::hex8(uint_fast8_t pos, uint_fast8_t val) {
    hex4(pos, val >> 4);
    hex4(pos + 1, val);
}

void CharBuffer::hex12(uint_fast8_t pos, uint_fast16_t val) {
    hex4(pos, val >> 8);
    hex8(pos + 1, val);
}

void CharBuffer::hex16(uint_fast8_t pos, uint_fast16_t val) {
    hex8(pos, val >> 8);
    hex8(pos + 2, val);
}

void CharBuffer::hex20(uint_fast8_t pos, uint_fast32_t val) {
    hex4(pos, val >> 16);
    hex16(pos + 1, val);
}

void CharBuffer::hex24(uint_fast8_t pos, uint_fast32_t val) {
    hex8(pos, val >> 16);
    hex16(pos + 2, val);
}

void CharBuffer::hex32(uint_fast8_t pos, uint_fast32_t val) {
    hex16(pos, val >> 16);
    hex16(pos + 4, val);
}

void CharBuffer::oct3(uint_fast8_t pos, uint_fast8_t val) {
    _str[pos] = toOct3(val & 7);
}

void CharBuffer::oct6(uint_fast8_t pos, uint_fast8_t val) {
    oct3(pos, val >> 3);
    oct3(pos + 1, val);
}

void CharBuffer::oct12(uint_fast8_t pos, uint_fast16_t val) {
    oct6(pos, val >> 6);
    oct6(pos + 2, val);
}

void CharBuffer::oct15(uint_fast8_t pos, uint_fast16_t val) {
    oct3(pos, val >> 12);
    oct12(pos + 1, val);
}

void CharBuffer::oct18(uint_fast8_t pos, uint_fast32_t val) {
    oct6(pos, val >> 12);
    oct12(pos + 4, val);
}

void CharBuffer::oct24(uint_fast8_t pos, uint_fast32_t val) {
    oct12(pos, val >> 12);
    oct12(pos + 4, val);
}

void CharBuffer::bits(uint_fast8_t pos, uint_fast16_t value, uint_fast16_t mask,
        const char *letters) {
    while (mask && *letters) {
        const auto c = *letters++;
        _str[pos++] = (value & mask) ? c : (c == '1' ? '0' : '_');
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
