#ifndef __INST_INS8070_H__
#define __INST_INS8070_H__

#include <stdint.h>
#include "match_memory.h"
#include "match_walker.h"
#include "signals_ins8070.h"

namespace debugger {
namespace ins8070 {

// Effective address type
// Assumption:
// - no instruction on internal RAM.
// - two bytes operands are fit in internal RAM.
enum AddrMode : uint8_t {
    M_NO = 0,
    M_DISP = 1,  // disp,PC/SP/P2/P3
    M_AUTO = 2,  // @disp,P2/P3
    M_DIR = 3,   // direct page
    M_PUSH = 4,  // --SP
    M_POP = 5,   // SP++
    M_SSM = 6,   // SSM Pn
};

struct InstIns8070 final {
    InstIns8070() : opc(0), seq(0) {}
    bool get(uint8_t data);

    uint8_t opc;

    uint8_t len() const;
    uint8_t busCycles() const;
    uint8_t externalCycles() const;
    AddrMode addrMode() const;
    // Its bus cycles, in match_legend.md's tokens.
    const char *sequence() const;

    static constexpr uint8_t RET = 0x5C;
    static constexpr uint8_t BRA = 0x74;
    static constexpr uint8_t BRA_HERE = 0xFE;
    static constexpr uint8_t CALL15 = 0x1F;
    static constexpr uint16_t VEC_CALL15 = 0x003E;

private:
    uint8_t seq;
};

// The INS8070 as MatchWalker sees it, its code in |mems|.
struct ArchIns8070 : MatchWalker::Arch {
    explicit ArchIns8070(const MatchMemory &mems)
        : Arch(MatchWalker::Traits{.targetBias = 1}), _mems(mems) {}

    MatchWalker::Kind cycleKind(const SignalsImpl *s) const override;
    bool decode(uint32_t pc, MatchWalker::Decoded &inst) const override;
    bool isVectorTable(uint32_t addr) const override;
    const char *interruptSequence() const override;
    void markCycle(SignalsImpl *s, MatchWalker::Role role,
            uint_fast8_t span) const override;

private:
    const MatchMemory &_mems;
};

}  // namespace ins8070
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
