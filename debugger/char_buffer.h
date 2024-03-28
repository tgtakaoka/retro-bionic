#ifndef __DEBUGGER_CHAR_BUFFER_H__
#define __DEBUGGER_CHAR_BUFFER_H__

#include <stdint.h>

namespace debugger {

struct CharBuffer final {
    explicit CharBuffer(uint8_t size);
    explicit CharBuffer(const char *str);
    ~CharBuffer() { delete[] _str; }

    operator const char *() const { return _str; }
    char &operator[](int pos) { return _str[pos]; }

    void hex1(uint8_t pos, uint8_t value);
    void hex4(uint8_t pos, uint8_t value);
    void hex8(uint8_t pos, uint8_t value);
    void hex12(uint8_t pos, uint16_t value);
    void hex16(uint8_t pos, uint16_t value);
    void hex20(uint8_t pos, uint32_t value);
    void hex24(uint8_t pos, uint32_t value);
    void hex32(uint8_t pos, uint32_t value);
    void bits(uint8_t pos, uint8_t value, uint8_t mask, const char *letters);

private:
    char *const _str;
};

}  // namespace debugger

#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
