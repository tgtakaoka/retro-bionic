#ifndef __INST_I8096_H__
#define __INST_I8096_H__

#include <stdint.h>

namespace debugger {
namespace i8096 {

struct MemsI8096;

struct InstI8096 final {
    InstI8096(MemsI8096 *mems);

    bool set(uint16_t pc);
    uint_fast8_t instLength() const;
    uint_fast8_t busCycles() const;


    static constexpr uint16_t ORG_RESET = 0x2080;
    static constexpr uint16_t VEC_TRAP = 0x2010;

    /** NOP */
    static constexpr uint8_t NOP = 0xFD;
    /** Load immediate */
    static constexpr uint8_t LD = 0xA1;
    static constexpr uint8_t LDB = 0xB1;
    /** Store absolute */
    static constexpr uint8_t ST = 0xC3;
    static constexpr uint8_t STB = 0xC7;
    /** Jump */
    static constexpr uint8_t SJMP(int16_t disp) {
        return 0x20 | ((disp >> 8) & 7);
    }
    static constexpr uint8_t LJMP = 0xE7;
    /** Save/Restore PSW */
    static constexpr uint16_t PUSHF = 0xF2;
    static constexpr uint16_t POPF = 0xF3;

private:
    MemsI8096 *const _mems;
    uint_fast8_t _entry;
    bool _indexed;
};

}  // namespace i8096
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
