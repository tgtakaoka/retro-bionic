#ifndef __INST_TLCS90_H__
#define __INST_TLCS90_H__

#include "mems.h"
#include "signals_tlcs90.h"

namespace debugger {
namespace tlcs90 {

struct InstTlcs90 {
    uint8_t matched() const { return _matched; }
    uint8_t nextInstruction() const { return _nexti; }

    static bool valid(uint16_t addr, Mems *mems);

    bool match(const Signals *begin, const Signals *end,
            const Signals *prefetch = nullptr);
    bool matchInterrupt(const Signals *begin, const Signals *end);

    static constexpr uint8_t HALT = 0x01;
    static constexpr uint8_t SWI = 0xFF;
    static constexpr uint16_t ORG_SWI = 0x0010;
    static constexpr uint16_t ORG_NMI = 0x0018;

private:
    uint8_t _matched;
    uint8_t _nexti;

    bool matchSequence(const Signals *begin, const Signals *end,
            const char *seq, const Signals *prefetch);
};

}  // namespace tlcs90
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
