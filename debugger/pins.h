#ifndef __DEBUGGER_PINS_H__
#define __DEBUGGER_PINS_H__

#include <stdint.h>

#include "cycles.h"

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

    // Wall-clock time the last run() actually spent free-running the
    // target -- between startRunTimer() and stopRunTimer(), so it
    // excludes register save/restore and any post-run disassembly. Reads
    // and clears in one step, so a value is reported once: 0 if this
    // target has not called them yet (most don't; see z280 for the first
    // one), or if already retrieved.
    uint32_t retrieveRunMicros() {
        const auto us = _runMicros;
        _runMicros = 0;
        return us;
    }

    // How many instructions the next run()'s post-stop backtrace may
    // print -- set before run(), read by disassembleCycles(). 0: none.
    void setRunLineLimit(uint32_t n) { _lineLimit = n; }

    // Post-stop backtrace: disposes cycles before findBacktraceStart(),
    // then prints what's left via printBacktrace(). Most targets' run()
    // still calls their own, differently-named method instead of this
    // one, and so don't support a limit yet -- see z280 for the first
    // target that does.
    void disassembleCycles();

    static void initDebug();
    static bool haltSwitch();
    static void assert_debug();
    static void negate_debug();
    static void toggle_debug();
    static void isrHaltSwitch();

protected:
    friend struct Target;
    Regs *_regs;
    Mems *_mems;
    Devs *_devs;
    /* The bus cycle ring and its print buffer, owned by this target. */
    Cycles _cycles;
    uint32_t _startMicros = 0;
    uint32_t _runMicros = 0;
    uint32_t _lineLimit = UINT32_MAX;

    template <typename REGS>
    REGS *regs() const { return static_cast<REGS *>(_regs); }
    template <typename MEMS>
    MEMS *mems() const { return static_cast<MEMS *>(_mems); }
    template <typename DEVS>
    DEVS *devs() const { return static_cast<DEVS *>(_devs); }

    virtual void resetPins() = 0;

    // Where the kept window of the post-stop backtrace begins -- default
    // keeps everything captured (no limit support). Override with
    // signals.h's backtraceStartByFetchCount<Signals>(_lineLimit) on a
    // target whose fetch() flag reliably marks every instruction with no
    // setup; one that needs its own pattern matching to establish fetch()
    // at all runs that first, then hands its result to backtraceStartFrom()
    // instead. Non-const: several targets' pattern matching calls idle()
    // (a non-const virtual) along the way.
    virtual const SignalsImpl *findBacktraceStart() {
        return Cycles::tail();
    }

    // The post-stop backtrace's actual printing, run only on what
    // findBacktraceStart() (via disassembleCycles()) left in the ring.
    // Empty default so targets not yet converted from their own
    // disassembleCycles() still compile -- once every target overrides
    // this, it should become pure virtual so a new one that forgets it
    // fails to build instead of printing nothing.
    virtual void printBacktrace() {}

    bool isBreakPoint(uint32_t addr) const;
    void saveBreakInsts() const;
    void restoreBreakInsts() const;

    // Bracket a run()/step()'s actual free-running loop() call, the one
    // that watches the halt switch, a breakpoint or the RST-38H exit
    // convention: micros() around just that call, not the bookkeeping
    // before or the disassembly after.
    void startRunTimer();
    void stopRunTimer();

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

private:
    static volatile bool _halted;
};
}  // namespace debugger

#endif /* __PINS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
