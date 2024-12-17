#ifndef __INST_MC6800_H__
#define __INST_MC6800_H__

#include "mems.h"
#include "signals_mc6800.h"

namespace debugger {
namespace mc6800 {

struct InstMc6800 {
    InstMc6800(Mems *mems) : _mems(mems) {}
    virtual ~InstMc6800() {}

    virtual bool match(
            const Signals *begin, const Signals *end, const Signals *prefetch);
    virtual bool matchInterrupt(const Signals *begin, const Signals *end);
    uint8_t matched() const { return _matched; }
    int8_t nextInstruction() const { return _nexti; }

    static constexpr uint8_t NOP = 0x01;
    static constexpr uint8_t BRA = 0x20;
    static constexpr uint8_t BRA_HERE = 0xFE;
    static constexpr uint8_t SWI = 0x3F;

    virtual uint16_t vec_swi() const { return 0xFFFA; }
    static constexpr uint16_t VEC_RESET = 0xFFFE;

protected:
    Mems *_mems;
    uint8_t _matched;
    int8_t _nexti;

    bool matchSequence(const Signals *begin, const Signals *end,
            const char *seq, const Signals *prefetch);
    virtual const char *opcSequence(uint16_t addr) const;
    virtual const char *instSequence(uint8_t opc) const { return ""; }
    virtual const char *intrSequence() const { return ""; }
    virtual uint16_t vectorBase() const { return 0xFFF8; }
};

}  // namespace mc6800
}  // namespace debugger
#endif
// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
