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

    void begin() {
        _pins->resetPins();
        _devs->begin();
    }
    const char *identity() const { return _identity; }
    Pins &pins() { return *_pins; }
    Regs &regs() { return *_regs; }
    Mems &mems() { return *_mems; }
    Devs &devs() { return *_devs; }

    Target(const char *id, Pins &pins, Regs &regs, Mems &mems, Devs &devs);

protected:
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
