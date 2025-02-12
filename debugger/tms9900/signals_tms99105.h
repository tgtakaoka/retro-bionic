#ifndef __SIGNALS_TMS99105_H__
#define __SIGNALS_TMS99105_H__

#include "signals_tms9900.h"

namespace debugger {
namespace tms99105 {

// Bus status
// LSB - #MEM
//     - BST1
//     - BST2
// MSB - MST3
enum BST : uint8_t {
    SOPL = 0x0,   // Source operand with MPILCK
    SOP = 0x8,    // Source operand
    IOP = 0x4,    // Immediate data, second word, or symbolic address
    IAQ = 0xC,    // Instrucion acuisition
    DOP = 0x2,    // Destinaton operand
    INTA = 0xA,   // Interrupt accknowledge
    WS = 0x6,     // Workspace
    GM = 0xE,     // General memory
    AUMSL = 0x1,  // Internal arithmetic unit op or macro store with MPILCK
    AUMS = 0x9,   // Internal arithmetic unit op or macro store
    RESET = 0x5,  // Reset
    IO = 0xD,     // IO
    WP = 0x3,     // Workspace pointer update
    ST = 0xB,     // Status register update
    MID = 0x7,    // Macroinstruction detected
    HOLDA = 0xF,  // Hold acknowledge
};

struct Signals final : SignalsBase<Signals, tms9900::Signals> {
    void getAddress();
    void getControl();
    void getData();
    void outData() const;
    void inputMode() const;

    bool writeEnable() const;
    bool readEnable() const;
    bool macrostore() const;

private:
    uint8_t bst() const { return _signals[1]; }
    uint8_t &bst() { return _signals[1]; }
    uint8_t rd() const { return _signals[2]; }
    uint8_t &rd() { return _signals[2]; }
};
}  // namespace tms99105
}  // namespace debugger
#endif /* __SIGNALS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
