#ifndef __REGS_TMS320C2X_H__
#define __REGS_TMS320C2X_H__

#include "char_buffer.h"
#include "regs.h"

namespace debugger {

namespace tms320c25 {
struct PinsTms320C25;
}

namespace tms320c2x {

using tms320c25::PinsTms320C25;

struct RegsTms320C2X final : Regs {
    RegsTms320C2X(PinsTms320C25 *pins);

    const char *cpu() const override;

    void print() const override;
    void reset() override {}
    /** Save at reset, where the pending fetch is the next instruction. */
    void save() override;
    /**
     * Save, the next instruction at |pc|. An interrupt taken while halting
     * may have pushed an address just past |ret| (-1 if none was).
     */
    void save(uint16_t pc, int32_t ret);
    void restore() override;

    uint32_t nextIp() const override { return _pc; }
    void helpRegisters() const override;
    const RegList *listRegisters(uint_fast8_t n) const override;
    bool setRegister(uint_fast8_t reg, uint32_t value) override;

    uint16_t read_data(uint16_t addr) const;
    void write_data(uint16_t addr, uint16_t data) const;

    /** NOP fetches a halt runs before it settles on the next instruction. */
    static constexpr uint_fast8_t DRAIN_FETCHES = 3;

private:
    PinsTms320C25 *const _pins;

    uint16_t _pc;
    uint16_t _st0;  // ARP OV OVM 1 INTM DP
    uint16_t _st1;  // ARB CNF TC SXM C 1 1 HM FSM XF FO TXM PM
    uint32_t _acc;
    uint32_t _p;
    uint16_t _t;
    uint16_t _ar[8];

    mutable CharBuffer _buffer1;
    mutable CharBuffer _buffer2;
    mutable CharBuffer _buffer3;

    uint16_t outSlot() const;
};

}  // namespace tms320c2x
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
