
/**
 *  Bionic controller
 */

#include "debugger.h"

using namespace debugger;

void setup() {
    Console.begin(115200);
    while (!Console)
        yield();
    cli.begin(Console);
    auto &target = Target::readIdentity();
    Debugger.begin(target);
}

void loop() {
    Debugger.loop();
}

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
