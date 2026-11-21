#include "devs_pdp11.h"
#include <strings.h>

namespace debugger {
namespace pdp11 {

DevsPdp11::DevsPdp11(bool clock)
    : _dl11(new Dl11()), _kw11l(new Kw11l()), _swr(new Swr()), _clock(clock) {
    _dl11->setBaseAddr(DL11_BASE);
    _kw11l->setBaseAddr(KW11L_BASE);
}

DevsPdp11::~DevsPdp11() {
    delete _dl11;
    delete _kw11l;
    delete _swr;
}

void DevsPdp11::reset() {
    _dl11->reset();
    _kw11l->reset();
}

void DevsPdp11::begin() {
    enableDevice(_dl11);
    _kw11l->enable(_clock);
    _swr->enable(_clock);
}

bool DevsPdp11::loopDevices() {
    _kw11l->loop();
    return _dl11->loop();
}

uint16_t DevsPdp11::read(uint32_t addr) const {
    return select(addr)->read(addr);
}

uint16_t DevsPdp11::peek(uint32_t addr) const {
    return select(addr)->peek(addr);
}

void DevsPdp11::write(uint32_t addr, uint16_t data) const {
    select(addr)->write(addr, data);
}

uint16_t DevsPdp11::vector() const {
    return _dl11->acknowledge();
}

Device *DevsPdp11::parseDevice(const char *name) const {
    if (strcasecmp(name, _dl11->name()) == 0)
        return _dl11;
    if (_clock && strcasecmp(name, _kw11l->name()) == 0)
        return _kw11l;
    if (_clock && strcasecmp(name, _swr->name()) == 0)
        return _swr;
    return Devs::nullDevice();
}

// The line clock and the switches go on and off by themselves; the
// console stays as it is.
void DevsPdp11::enableDevice(Device *dev) {
    if (dev == _kw11l || dev == _swr) {
        dev->enable(!dev->isEnabled());
        return;
    }
    _dl11->enable(dev == _dl11);
}

void DevsPdp11::printDevices() const {
    printDevice(_dl11);
    if (_clock) {
        printDevice(_kw11l);
        printDevice(_swr);
    }
}

}  // namespace pdp11
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
