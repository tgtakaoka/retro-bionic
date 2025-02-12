#include "identity.h"
#include <string.h>
#include "config_debugger.h"
#include "debugger.h"
#include "pins.h"
#include "target.h"
#include "unio_bus.h"
#include "unio_eeprom.h"

namespace debugger {

unio::UnioBus unioBus{PIN_ID, 16};

const struct Identity *Identity::_head = nullptr;

Identity::Identity(const char *name, Pins *(*instance)())
    : _name(name), _instance(instance), _next(_head) {
    _head = this;
}

Target *Identity::instance() const {
    return new Target(this, _instance());
}

const Identity &Identity::searchIdentity(const char *name) {
    for (auto *id = _head; id; id = id->_next) {
        if (strcasecmp(name, id->name()) == 0)
            return *id;
    }
    return IdentityNull;
}

const Identity &Identity::readIdentity() {
    Pins::initDebug();
    unioBus.standby();
    unio::Eeprom rom{unioBus};
    uint8_t buffer[16];
    if (rom.read(0, sizeof(buffer), buffer)) {
        const auto *name = reinterpret_cast<const char *>(buffer);
        const auto len = strnlen(name, sizeof(buffer));
        if (len < sizeof(buffer) && !Pins::haltSwitch())
            return searchIdentity(name);
    }
    return IdentityNull;
}

void Identity::writeIdentity(const char *name) {
    const auto len = strlen(name);
    unioBus.standby();
    unio::Eeprom rom{unioBus};
    const auto *buffer = reinterpret_cast<const uint8_t *>(name);
    const auto valid = rom.write(0, len + 1, buffer);
    if (valid) {
        cli.println("DONE");
    } else {
        cli.println("FAILED");
    }
}

void Identity::printIdentity() {
    unioBus.standby();
    unio::Eeprom rom{unioBus};
    uint8_t buffer[16];
    const auto valid = rom.read(0, sizeof(buffer), buffer);
    const auto *name = reinterpret_cast<const char *>(buffer);
    if (valid) {
        cli.print(name);
    } else {
        cli.print("?????");
    }
    if (strcasecmp(name, Debugger.target().cpuName()) != 0) {
        cli.print(" (CPU: ");
        cli.print(Debugger.target().cpuName());
        cli.print(')');
    }
}

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
