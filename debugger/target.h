#ifndef __DEBUGGER_TARGET_H__
#define __DEBUGGER_TARGET_H__

#include "devs.h"
#include "mems.h"
#include "pins.h"
#include "regs.h"

namespace debugger {
struct Target {
    static Target &readIdentity();
    static void writeIdentity(const char *identity);
    static void printIdentity();

    Target(const char *id, Pins &pins, Regs &regs, Mems &mems, Devs &devs);

    void begin() {
        _pins->resetPins();
        _devs->begin();
    }
    const char *identity() const { return _identity; }

    void reset() { _pins->resetPins(); }
    void run();
    bool step(bool show) { return _pins->step(show); }
    void idle() { _pins->idle(); }

    void printCycles() { _pins->printCycles(); }
    void printRegisters() const { _regs->print(); }
    void printStatus() const;
    void printDevices() const { _devs->printDevices(); }
    bool printRomArea() const;
    bool printBreakPoints() const { return _pins->printBreakPoints(); }
    bool isOnBreakPoint() const { return _pins->isBreakPoint(_regs->nextIp()); }

protected:
    friend class Debugger;
    const char *const _identity;
    Pins *_pins;
    Regs *_regs;
    Mems *_mems;
    Devs *_devs;
    Target *const _next;

    static Target *_head;
    static Target &searchIdentity(const char *identity);
};
}  // namespace debugger

#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
