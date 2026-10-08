#include "inst_mc6800.h"

namespace debugger {
namespace mc6800 {

MatchWalker::Kind ArchMc6800::cycleKind(const SignalsImpl *impl) const {
    const auto s = static_cast<const Signals *>(impl);
    if (!s->valid())
        return MatchWalker::K_NONE;
    return s->read() ? MatchWalker::K_READ : MatchWalker::K_WRITE;
}

// The operand bytes, big endian, are the effective address, and the
// target but for a relative branch's: the next + disp.
bool ArchMc6800::decode(uint32_t pc, MatchWalker::Decoded &inst) const {
    pc &= 0xFFFF;
    const auto opc = _mems->read_byte(pc);
    const auto seq = instSequence(opc);
    if (seq == nullptr || *seq == 0)
        return false;
    inst.pc = pc;
    inst.seq = seq;
    inst.length = MatchWalker::instructionBytes(seq);
    uint16_t operand = 0;
    for (auto i = 1u; i < inst.length; ++i)
        operand = (operand << 8) | _mems->read_byte((pc + i) & 0xFFFF);
    inst.hasEa = inst.hasTarget = true;
    inst.ea = operand;
    const auto next = pc + inst.length;
    const auto branch = (opc & 0xF0) == 0x20 || opc == 0x8D;  // Bcc, BSR
    inst.target =
            branch ? (next + static_cast<int8_t>(operand)) & 0xFFFF : operand;
    return true;
}

// A fetch marked with the cycles its instruction took.
void ArchMc6800::markCycle(
        SignalsImpl *impl, MatchWalker::Role role, uint_fast8_t span) const {
    const auto s = static_cast<Signals *>(impl);
    if (role == MatchWalker::R_FETCH) {
        s->markFetch(span);
    } else {
        s->clearFetch();
    }
}

}  // namespace mc6800
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
