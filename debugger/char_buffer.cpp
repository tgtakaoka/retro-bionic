#include "char_buffer.h"
#include <string.h>

namespace debugger {

namespace {
constexpr char toHex4(uint8_t val) {
    return val < 10 ? val + '0' : val - 10 + 'A';
}
constexpr char toOct3(uint8_t val) {
    return val + '0';
}
}  // namespace

CharBuffer::CharBuffer(uint8_t size) : _str(new char[size + 1]) {
    _str[0] = 0;
}

CharBuffer::CharBuffer(const char *str) : _str(new char[strlen(str) + 1]) {
    strcpy(_str, str);
}

void CharBuffer::hex1(uint8_t pos, uint8_t val) {
    _str[pos] = val ? '1' : '0';
}

void CharBuffer::hex4(uint8_t pos, uint8_t val) {
    _str[pos] = toHex4(val & 0xF);
}

void CharBuffer::hex8(uint8_t pos, uint8_t val) {
    hex4(pos, val >> 4);
    hex4(pos + 1, val);
}

void CharBuffer::hex12(uint8_t pos, uint16_t val) {
    hex4(pos, val >> 8);
    hex8(pos + 1, val);
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

void CharBuffer::oct3(uint8_t pos, uint8_t val) {
    _str[pos] = toOct3(val & 7);
}

void CharBuffer::oct6(uint8_t pos, uint8_t val) {
    oct3(pos, val >> 3);
    oct3(pos + 1, val);
}

void CharBuffer::oct12(uint8_t pos, uint16_t val) {
    oct6(pos, val >> 6);
    oct6(pos + 2, val);
}

void CharBuffer::oct15(uint8_t pos, uint16_t val) {
    oct3(pos, val >> 12);
    oct12(pos + 1, val);
}

void CharBuffer::oct18(uint8_t pos, uint32_t val) {
    oct6(pos, val >> 12);
    oct12(pos + 4, val);
}

void CharBuffer::oct24(uint8_t pos, uint32_t val) {
    oct12(pos, val >> 12);
    oct12(pos + 4, val);
}

void CharBuffer::bits(
        uint8_t pos, uint16_t value, uint16_t mask, const char *letters) {
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
