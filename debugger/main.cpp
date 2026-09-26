/**
 *  Bionic controller
 */

#include "debugger.h"

using namespace debugger;

// A byte on the halt port asks a running CPU to stop. The core re-calls this
// from every yield() while the port still holds data, so the byte has to be
// taken here; left unread it re-raises the flag right after setRun() clears
// it, and the next run stops before it has begun.
void serialEventUSB1() {
    while (SerialUSB1.available())
        SerialUSB1.read();
    Pins::isrHaltSwitch();
}

void setup() {
    Console.begin(115200);
    while (!Console)
        yield();
    cli.begin(Console);
#if defined(ENABLE_LOGGER)
    Logger.begin(115200);
    logger.begin(Logger);
#endif
    Pins::initDebug();
    auto id = Identity::readIdentity();
    Debugger.begin(id.instance());
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
