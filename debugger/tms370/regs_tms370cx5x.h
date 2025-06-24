#ifndef __REGS_TMS370CX5X_H__
#define __REGS_TMS370CX5X_H__

#include "regs_tms370.h"

namespace debugger {
namespace tms370cx5x {

struct PinsTms370Cx5x;

struct RegsTms370Cx5x final : tms370::RegsTms370 {
    RegsTms370Cx5x(PinsTms370Cx5x *pins);

    void save() override;
    void restore() override;

private:
    static constexpr uint8_t INT1 = 0x17; // P017
    uint8_t _INTx[3];
    // TODO: save/restore SCI1 registers
};

}  // namespace tms370cx5x
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
