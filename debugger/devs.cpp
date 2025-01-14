#include "devs.h"
#include <string.h>
#include "debugger.h"
#include "device.h"

namespace debugger {

namespace {
struct NullDevice final : Device {
    NullDevice() : Device() {}

    const char *name() const override { return "null"; }
    const char *description() const override { return "null device"; }
    void reset() override {}
    void loop() override {}
} NullDevice;
}  // namespace

Device *Devs::nullDevice() {
    return &NullDevice;
}

void Devs::printDevice(const Device *dev) const {
    const auto radix = Debugger.target().inputRadix();
    const auto digits =
            Debugger::numDigits(Debugger.target().addressWidth(), radix);
    cli.printStr(dev->name(), -6);
    cli.print(' ');
    cli.printStr(dev->description(), -10);
    cli.print(" at ");
    cli.printNum(dev->baseAddr(), radix, digits);
    if (dev->isEnabled()) {
        cli.println();
        dev->print();
    } else {
        cli.println(" DISABLED");
    }
}

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
