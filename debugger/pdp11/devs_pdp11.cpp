#include "devs_pdp11.h"
#include <strings.h>

namespace debugger {
namespace pdp11 {

DevsPdp11::DevsPdp11(const Mems *dma)
    : _dl11(new Dl11()),
      _kw11l(new Kw11l()),
      _rk11(new Rk11(dma)),
      _swr(new Swr()),
      _system(dma != nullptr) {
    _dl11->setBaseAddr(DL11_BASE);
    _kw11l->setBaseAddr(KW11L_BASE);
    _rk11->setBaseAddr(RK11_BASE);
}

DevsPdp11::~DevsPdp11() {
    delete _dl11;
    delete _kw11l;
    delete _rk11;
    delete _swr;
}

void DevsPdp11::reset() {
    _dl11->reset();
    _kw11l->reset();
    _rk11->reset();
}

void DevsPdp11::begin() {
    enableDevice(_dl11);
    _kw11l->enable(_system);
    _rk11->enable(_system);
    _swr->enable(_system);
}

Disk *DevsPdp11::disk() const {
    return _system ? _rk11 : nullptr;
}

bool DevsPdp11::loopDevices() {
    _kw11l->loop();
    _rk11->loop();
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

uint16_t DevsPdp11::vector(uint_fast8_t level) const {
    if (level == 5 && _rk11->requesting()) {
        _rk11->acknowledge();
        return Rk11::VECTOR;
    }
    return _dl11->acknowledge();
}

Device *DevsPdp11::parseDevice(const char *name) const {
    if (strcasecmp(name, _dl11->name()) == 0)
        return _dl11;
    if (_system && strcasecmp(name, _kw11l->name()) == 0)
        return _kw11l;
    if (_system && strcasecmp(name, _rk11->name()) == 0)
        return _rk11;
    if (_system && strcasecmp(name, _swr->name()) == 0)
        return _swr;
    return Devs::nullDevice();
}

// The line clock and the disk go on and off by themselves; the console
// stays as it is.
void DevsPdp11::enableDevice(Device *dev) {
    if (dev == _kw11l || dev == _rk11 || dev == _swr) {
        dev->enable(!dev->isEnabled());
        return;
    }
    _dl11->enable(dev == _dl11);
}

void DevsPdp11::printDevices() const {
    printDevice(_dl11);
    if (_system) {
        printDevice(_kw11l);
        printDevice(_rk11);
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
