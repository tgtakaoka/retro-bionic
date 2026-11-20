#include "devs_pdp11.h"
#include <strings.h>

namespace debugger {
namespace pdp11 {

DevsPdp11::DevsPdp11() : _dl11(new Dl11()) {
    _dl11->setBaseAddr(DL11_BASE);
}

DevsPdp11::~DevsPdp11() {
    delete _dl11;
}

void DevsPdp11::reset() {
    _dl11->reset();
}

void DevsPdp11::begin() {
    enableDevice(_dl11);
}

bool DevsPdp11::loopDevices() {
    return _dl11->loop();
}


uint16_t DevsPdp11::read(uint32_t addr) const {
    return _dl11->read(addr);
}

uint16_t DevsPdp11::peek(uint32_t addr) const {
    return _dl11->peek(addr);
}

void DevsPdp11::write(uint32_t addr, uint16_t data) const {
    _dl11->write(addr, data);
}

uint16_t DevsPdp11::vector() const {
    return _dl11->acknowledge();
}

Device *DevsPdp11::parseDevice(const char *name) const {
    if (strcasecmp(name, _dl11->name()) == 0)
        return _dl11;
    return Devs::nullDevice();
}

void DevsPdp11::enableDevice(Device *dev) {
    _dl11->enable(dev == _dl11);
}

void DevsPdp11::printDevices() const {
    printDevice(_dl11);
}

}  // namespace pdp11
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
