#ifndef __DEBUGGER_H__
#define __DEBUGGER_H__

#include "config_debugger.h"

#include <libcli.h>
#include "break_points.h"
#include "target.h"
#include "unio_bus.h"

namespace debugger {

struct Debugger {
    void begin(Target &target);
    void exec(char c);
    void loop();

    const Target &target() const { return *_target; }
    void setTarget(const Target &target) { _target = &target; }

    bool isBreakPoint(uint32_t addr) { return _breakPoints.on(addr); }
    bool setBreakPoint(uint32_t addr) { return _breakPoints.set(addr); }
    bool clearBreakPoint(uint8_t index) { return _breakPoints.clear(index); }
    bool printBreakPoints() const { return _breakPoints.print(); }
    void saveBreakInsts() { _breakPoints.saveInsts(); }
    void restoreBreakInsts() { _breakPoints.restoreInsts(); }

private:
    const Target *_target;
    bool _verbose;
    BreakPoints _breakPoints;
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
