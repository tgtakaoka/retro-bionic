#include "regs_mc6802.h"
#include "pins_mc6800_base.h"


namespace debugger {
namespace mc6802 {
namespace {
const char MC6802[] = "MC6802";
const char MB8870[] = "MB8870";
}  // namespace

const char *RegsMc6802::cpuName() const {
    if (_type == SW_MC6800)
        return MC6802;
    if (_type == SW_MB8861)
        return MB8870;
    return RegsMc6800::cpuName();
}

uint8_t RegsMc6802::internal_read(uint16_t addr) const {
    const uint8_t LDAA[] = {
            0xB6, hi(addr), lo(addr),  // LDAA addr  ; 1:2:3:A
    };
    _pins->injectReads(LDAA, sizeof(LDAA), 4);
    _pins->injectReads(STAA_8000, sizeof(STAA_8000));
    uint8_t data;
    _pins->captureWrites(&data, sizeof(data));
    return data;
}

void RegsMc6802::internal_write(uint16_t addr, uint8_t data) const {
    const uint8_t LDAA_STAA[] = {
            0x86, data,                // LDAA #val ; 1:2
            0xB7, hi(addr), lo(addr),  // STAA addr ; 1:2:3:n:B
    };
    _pins->injectReads(LDAA_STAA, sizeof(LDAA_STAA), 7);
}

}  // namespace mc6802
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
