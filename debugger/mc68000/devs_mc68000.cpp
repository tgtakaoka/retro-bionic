#include "devs_mc68000.h"
#include <strings.h>
#include "mc6850.h"

namespace debugger {
namespace mc68000 {

DevsMc68000::DevsMc68000(uint32_t addrMask, const Mems *mems)
    : _acia(new Mc6850()), _disk(new Cpm68kDisk(mems)) {
    _acia->setBaseAddr(ACIA_BASE & addrMask);
    _acia->setVector(ACIA_VECTOR, ACIA_VECTOR);
    _disk->setBaseAddr(DISK_BASE & addrMask);
}

DevsMc68000::~DevsMc68000() {
    delete _acia;
    delete _disk;
}

void DevsMc68000::reset() {
    _acia->reset();
    _disk->reset();
}

void DevsMc68000::begin() {
    enableDevice(_acia);
}

bool DevsMc68000::loopDevices() {
    return _acia->loop();
}

uint16_t DevsMc68000::read(uint32_t addr) const {
    if (_disk->isSelected(addr))
        return _disk->read(addr);
    return _acia->read(addr);
}

void DevsMc68000::write(uint32_t addr, uint16_t data) const {
    if (_disk->isSelected(addr)) {
        _disk->write(addr, data);
    } else {
        _acia->write(addr, data);
    }
}

uint16_t DevsMc68000::vector() const {
    return _acia->vector();
}

Device *DevsMc68000::parseDevice(const char *name) const {
    if (strcasecmp(name, _acia->name()) == 0)
        return _acia;
    if (strcasecmp(name, _disk->name()) == 0)
        return _disk;
    return Devs::nullDevice();
}

// The disk turns on as its images mount, and I DISK turns it on; the
// console stays as it is.
void DevsMc68000::enableDevice(Device *dev) {
    if (dev == _disk) {
        _disk->enable(true);
        return;
    }
    _acia->enable(dev == _acia);
}

void DevsMc68000::printDevices() const {
    printDevice(_acia);
    printDevice(_disk);
}

}  // namespace mc68000
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
