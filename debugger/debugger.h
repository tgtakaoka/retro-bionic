#ifndef __DEBUGGER_H__
#define __DEBUGGER_H__

#include "config_debugger.h"

#include <libcli.h>
#include "target.h"
#include "unio_bus.h"

namespace debugger {

struct Debugger {
    void begin(Target &target);
    void exec(char c);
    void loop();

    Target &target() const { return *_target; }
    Pins &pins() const { return target().pins(); }
    Regs &regs() const { return target().regs(); }
    Mems &mems() const { return target().mems(); }
    Devs &devs() const { return target().devs(); }

private:
    Target *_target;
};

extern struct Debugger Debugger;
extern struct libcli::Cli cli;
extern struct unio::UnioBus unioBus;

}  // namespace debugger

#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
