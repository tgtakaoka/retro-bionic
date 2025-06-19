#ifndef __INST_I8096_H__
#define __INST_I8096_H__

#include "mems_i8096.h"
#include "signals_i8096.h"

namespace debugger {
namespace i8096 {

struct InstI8096 final {
    bool set(uint16_t pc, MemsI8096 *mems);
    uint_fast8_t opc() const { return _opc; }
    uint_fast8_t instLength() const;

    static constexpr uint16_t ORG_RESET = 0x2080;
    static constexpr uint16_t VEC_EXTINT = 0x200E;
    static constexpr uint16_t VEC_TRAP = 0x2010;

    static constexpr uint8_t TRAP = 0xF7;
#define SJMP(disp) (0x20 | ((disp >> 8) & 7)), (disp & 0xFF)

    bool match(SignalsI8096 *begin, const SignalsI8096 *end, MemsI8096 *mems);
    bool isEnd() const { return _insufficient; }
    uint_fast8_t nexti() const { return _nexti; }

    struct Table {
        uint8_t len;
        uint8_t seq;
    };

private:
    uint_fast8_t _opc;
    uint_fast8_t _len;
    bool _indexed;
    bool _insufficient;
    uint_fast8_t _nexti;

    const Table *get(uint16_t pc, MemsI8096 *mens);
    static bool indexAddressing(uint_fast8_t opc);

    bool matchSequence(
            SignalsI8096 *begin, const SignalsI8096 *end, const char *seq);
    bool matchInterrupt(SignalsI8096 *begin, const SignalsI8096 *end) const;
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
