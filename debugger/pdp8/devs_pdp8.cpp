#include "devs_pdp8.h"
#include <string.h>
#include "debugger.h"
#include "mc6850.h"

namespace debugger {
namespace pdp8 {

DevsPdp8::DevsPdp8() : _acia(new Mc6850()) {}

DevsPdp8::~DevsPdp8() {
    delete _acia;
}

void DevsPdp8::reset() {
    _acia->reset();
    _acia->setBaseAddr(ACIA_BASE);
}

void DevsPdp8::begin() {
    enableDevice(_acia);
}

void DevsPdp8::loop() {
    _acia->loop();
}

bool DevsPdp8::isSelected(uint32_t addr) const {
    return _acia->isSelected(addr);
}

uint16_t DevsPdp8::read(uint32_t addr) const {
    return _acia->read(addr);
}

void DevsPdp8::write(uint32_t addr, uint16_t data) const {
    _acia->write(addr, data);
}

Device *DevsPdp8::parseDevice(const char *name) const {
    if (strcasecmp(name, _acia->name()) == 0)
        return _acia;
    return Devs::nullDevice();
}

void DevsPdp8::enableDevice(Device *dev) {
    _acia->enable(dev == _acia);
}

void DevsPdp8::printDevices() const {
    printDevice(_acia);
}

}  // namespace pdp8
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
