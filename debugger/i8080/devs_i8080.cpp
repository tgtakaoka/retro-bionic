#include "devs_i8080.h"
#include <strings.h>
#include "i8251.h"
#include "sector_disk.h"

namespace debugger {

DevsI8080::DevsI8080(const Mems *mems)
    : _usart(new I8251()), _disk(mems ? new SectorDisk(mems) : nullptr) {
    _usart->setBaseAddr(USART_BASE);
    if (_disk)
        _disk->setBaseAddr(DISK_BASE);
}

DevsI8080::~DevsI8080() {
    delete _usart;
    delete _disk;
}

void DevsI8080::reset() {
    _usart->reset();
    if (_disk)
        _disk->reset();
}

void DevsI8080::fileLoaded(const char *path) {
    if (_disk)
        _disk->mount(path);
}

void DevsI8080::begin() {
    enableDevice(_usart);
}

bool DevsI8080::loopDevices() {
    return _usart->loop();
}

bool DevsI8080::isSelected(uint32_t addr) const {
    return _usart->isSelected(addr) || (_disk && _disk->isSelected(addr));
}

uint16_t DevsI8080::read(uint32_t addr) const {
    if (_disk && _disk->isSelected(addr))
        return _disk->read(addr);
    return _usart->read(addr);
}

void DevsI8080::write(uint32_t addr, uint16_t data) const {
    if (_disk && _disk->isSelected(addr)) {
        _disk->write(addr, data);
    } else {
        _usart->write(addr, data);
    }
}

uint16_t DevsI8080::vector() const {
    return _usart->vector();
}

Device *DevsI8080::parseDevice(const char *name) const {
    if (strcasecmp(name, _usart->name()) == 0)
        return _usart;
    if (_disk && strcasecmp(name, _disk->name()) == 0)
        return _disk;
    return Devs::nullDevice();
}

// The disk turns on as its images mount, and I DISK turns it off and on;
// the console stays as it is.
void DevsI8080::enableDevice(Device *dev) {
    if (_disk && dev == _disk) {
        _disk->enable(!_disk->isEnabled());
        return;
    }
    _usart->enable(dev == _usart);
}

void DevsI8080::printDevices() const {
    printDevice(_usart);
    if (_disk)
        printDevice(_disk);
}

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
