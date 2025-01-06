#ifndef __DEBUGGER_CHAR_BUFFER_H__
#define __DEBUGGER_CHAR_BUFFER_H__

#include <stdint.h>

namespace debugger {

struct CharBuffer final {
    explicit CharBuffer(const char *str) { set(str); }
    void set(const char *str);

    operator const char *() const { return _str; }
    char &operator[](int pos) { return _str[pos]; }

    void hex1(uint_fast8_t pos, uint_fast8_t value);
    void hex4(uint_fast8_t pos, uint_fast8_t value);
    void hex8(uint_fast8_t pos, uint_fast8_t value);
    void hex12(uint_fast8_t pos, uint_fast16_t value);
    void hex16(uint_fast8_t pos, uint_fast16_t value);
    void hex20(uint_fast8_t pos, uint_fast32_t value);
    void hex24(uint_fast8_t pos, uint_fast32_t value);
    void hex32(uint_fast8_t pos, uint_fast32_t value);
    void oct3(uint_fast8_t pos, uint_fast8_t value);
    void oct6(uint_fast8_t pos, uint_fast8_t value);
    void oct12(uint_fast8_t pos, uint_fast16_t value);
    void oct15(uint_fast8_t pos, uint_fast16_t value);
    void oct18(uint_fast8_t pos, uint_fast32_t value);
    void oct24(uint_fast8_t pos, uint_fast32_t value);
    void bits(uint_fast8_t pos, uint_fast16_t value, uint_fast16_t mask,
            const char *letters);

private:
    static constexpr auto _max = 79;
    char _str[_max + 1];
};

}  // namespace debugger

#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
