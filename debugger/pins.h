#ifndef __DEBUGGER_PINS_H__
#define __DEBUGGER_PINS_H__

#include <stdint.h>

namespace debugger {

struct Regs;
struct Mems;
struct Devs;

struct Pins {
    virtual ~Pins();

    virtual void idle() = 0;
    virtual bool step(bool show) = 0;
    virtual void run() = 0;
    virtual void printCycles() = 0;
    virtual void assertInt(uint8_t name = 0) = 0;
    virtual void negateInt(uint8_t name = 0) = 0;
    virtual void setBreakInst(uint32_t addr) const = 0;

    void reset();

    // Control USER LED
    void setRun() const;
    void setHalt() const;

    static void assert_debug();
    static void negate_debug();
    static void toggle_debug();

protected:
    friend struct Target;
    Regs *_regs;
    Mems *_mems;
    Devs *_devs;

    template <typename REGS>
    REGS *regs() const { return static_cast<REGS *>(_regs); }
    template <typename MEMS>
    MEMS *mems() const { return static_cast<MEMS *>(_mems); }
    template <typename DEVS>
    DEVS *devs() const { return static_cast<DEVS *>(_devs); }

    virtual void resetPins() = 0;

    bool isBreakPoint(uint32_t addr) const;
    void saveBreakInsts() const;
    void restoreBreakInsts() const;

    static bool haltSwitch();

    static void pinsMode(const uint8_t *pins, uint8_t size, uint8_t mode);
    static void pinsMode(
            const uint8_t *pins, uint8_t size, uint8_t mode, uint8_t val);

    static constexpr uint8_t hi(uint16_t v) {
        return static_cast<uint8_t>(v >> 8);
    }
    static constexpr uint8_t lo(uint16_t v) {
        return static_cast<uint8_t>(v >> 0);
    }
    static constexpr uint16_t uint16(uint8_t hi, uint8_t lo) {
        return static_cast<uint16_t>(hi) << 8 | lo;
    }
};
}  // namespace debugger

#endif /* __PINS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
