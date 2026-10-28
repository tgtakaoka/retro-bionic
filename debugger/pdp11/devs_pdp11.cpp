#include "devs_pdp11.h"
#include <strings.h>
#include "mc6850.h"

namespace debugger {
namespace pdp11 {

DevsPdp11::DevsPdp11() : _acia(new Mc6850(2)) {
    _acia->setBaseAddr(ACIA_BASE);
}

DevsPdp11::~DevsPdp11() {
    delete _acia;
}

void DevsPdp11::reset() {
    _acia->reset();
}

void DevsPdp11::begin() {
    enableDevice(_acia);
}

void DevsPdp11::loop() {
    _acia->loop();
}

bool DevsPdp11::isSelected(uint32_t addr) const {
    return _acia->isSelected(addr);
}

uint16_t DevsPdp11::read(uint32_t addr) const {
    return _acia->read(addr);
}

uint16_t DevsPdp11::peek(uint32_t addr) const {
    return _acia->peek(addr);
}

void DevsPdp11::write(uint32_t addr, uint16_t data) const {
    _acia->write(addr, data);
}

uint16_t DevsPdp11::vector() const {
    return _acia->vector();
}

Device *DevsPdp11::parseDevice(const char *name) const {
    if (strcasecmp(name, _acia->name()) == 0)
        return _acia;
    return Devs::nullDevice();
}

void DevsPdp11::enableDevice(Device *dev) {
    _acia->enable(dev == _acia);
}

void DevsPdp11::printDevices() const {
    printDevice(_acia);
}

}  // namespace pdp11
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
