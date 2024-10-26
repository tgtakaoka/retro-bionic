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

    const Target &target() const { return *_target; }
    void setTarget(const Target &target) { _target = &target; }

private:
    const Target *_target;
    bool _verbose;
};

extern struct Debugger Debugger;
extern struct libcli::Cli cli;
extern struct unio::UnioBus unioBus;

#if defined(ENABLE_LOGGER)
extern struct libcli::Cli logger;
#endif

}  // namespace debugger

#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
