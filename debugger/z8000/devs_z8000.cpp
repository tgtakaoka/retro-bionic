#include "devs_z8000.h"
#include <strings.h>
#include "cpm8000_disk.h"
#include "mems_z8000.h"
#include "split_io.h"
#include "z80/devs_z80.h"

namespace debugger {
namespace z8000 {

DevsZ8000::DevsZ8000(MemsZ8000 *mems)
    : _devs(new DevsZ80(USART, new Cpm8000Disk(mems))),
      _split(new SplitIo(mems)) {
    _split->setBaseAddr(SPLIT_BASE);
}

DevsZ8000::~DevsZ8000() {
    delete _devs;
    delete _split;
}

void DevsZ8000::begin() {
    _devs->begin();
}

void DevsZ8000::reset() {
    _devs->reset();
    _split->reset();
}

bool DevsZ8000::isSelected(uint32_t addr) const {
    return _split->isSelected(addr) || _devs->isSelected(addr);
}

uint16_t DevsZ8000::read(uint32_t addr) const {
    return _split->isSelected(addr) ? _split->read(addr) : _devs->read(addr);
}

void DevsZ8000::write(uint32_t addr, uint16_t data) const {
    if (_split->isSelected(addr)) {
        _split->write(addr, data);
    } else {
        _devs->write(addr, data);
    }
}

uint16_t DevsZ8000::vector() const {
    return _devs->vector();
}

Device *DevsZ8000::parseDevice(const char *name) const {
    if (strcasecmp(name, _split->name()) == 0)
        return _split;
    return _devs->parseDevice(name);
}

// The split control is always there, like the Z380's address mask.
void DevsZ8000::enableDevice(Device *dev) {
    if (dev != _split)
        _devs->enableDevice(dev);
}

void DevsZ8000::disableDevice(Device *dev) {
    if (dev != _split)
        _devs->disableDevice(dev);
}

void DevsZ8000::printDevices() const {
    _devs->printDevices();
    printDevice(_split);
}

Disk *DevsZ8000::disk() const {
    return _devs->disk();
}

bool DevsZ8000::loopDevices() {
    return _devs->loopDevices();
}

}  // namespace z8000
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
