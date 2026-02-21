#ifndef __DEBUGGER_H__
#define __DEBUGGER_H__

#include <Arduino.h>
#include <libcli.h>
#include "break_points.h"
#include "config_debugger.h"
#include "target.h"
#include "unio_bus.h"

namespace debugger {

struct Debugger {
    void begin(Target *target);
    void exec(char c);
    void loop();

    Target &target() const { return *_target; }
    bool verbose() const { return _verbose; }

    void go();
    BreakPoints &breakPoints() { return _breakPoints; }

    static constexpr uint_fast8_t numDigits(
            const uint_fast8_t bits, const uint_fast8_t radix) {
        const auto digit = (radix == 8) ? 3 : 4;
        return bits / digit + ((bits % digit) == 0 ? 0 : 1);
    }

private:
    Target *_target;
    bool _verbose;
    BreakPoints _breakPoints;
};

extern struct Debugger Debugger;
extern libcli::Cli cli;
extern struct unio::UnioBus unioBus;

#if defined(ENABLE_LOGGER)
extern libcli::Cli logger;
#endif

}  // namespace debugger

#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
