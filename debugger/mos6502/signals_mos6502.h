#ifndef __SIGNALS_MOS6502_H__
#define __SIGNALS_MOS6502_H__

#include "signals.h"

namespace debugger {
namespace mos6502 {

enum HardwareType : uint8_t {
    HW_MOS6502 = 0,  // SW_MOS6502
    HW_W65C02S = 3,  // SW_W65C02S
    HW_W65C816 = 4,  // SW_W65C816
};

enum SoftwareType : uint8_t {
    SW_MOS6502 = 0,  // HW_MOS6502
    SW_G65SC02 = 1,  // HW_MOS6502
    SW_R65C02 = 2,   // HW_MOS6502
    SW_W65C02S = 3,  // HW_W65C02S
    SW_W65C816 = 4,  // HW_W65C816
};

struct Signals final : SignalsBase<Signals> {
    // Inline in pins_mos6502.h, for the bus engines.
    inline void getControl();
    inline void getAddr16();
    inline void getAddr24();
    inline void getData();
    // Puts data in the port while D0-D7 are still inputs.
    inline void stageData() const;
    // Drives what stageData() put.
    static inline void outputMode();
    static inline void inputMode();
    // D0-D7's port direction as resetPins() leaves it, so that a mode
    // change writes the direction register without reading it.
    static void saveDataMode();
    void print() const;

    inline bool read() const;
    inline bool write() const;
    bool fetch() const;
    bool vector() const;
    inline bool addr24() const;
    // Without the run-time part tests.
    template <HardwareType H>
    bool fetch() const;
    template <HardwareType H>
    bool vector() const;

private:
    static uint32_t _dataInput;
    static uint32_t _dataOutput;
    uint8_t cntl() const { return _signals[0]; }
    uint8_t &cntl() { return _signals[0]; }
};

}  // namespace mos6502
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
