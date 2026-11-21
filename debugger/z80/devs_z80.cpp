#include "devs_z80.h"
#include <strings.h>
#include "i8251.h"
#include "sector_disk.h"

namespace debugger {

namespace {
// CP/M's disk formats, by the index DRIVE reads back: ibm-3740 holds any
// image up to 256256 bytes, hd8 an image of exactly 8 MB. CP/M-80 and
// CP/M-86 share them, and hd8's DPB with them.
const DiskGeometry CPM_FORMATS[] = {
        {"ibm-3740", 77, 1, 26, 128, 1, 0xE5},  // 8" SSSD, sectors from 1
        {"hd8", 256, 1, 256, 128, 0, 0xE5},     // 8 MB, sectors from 0
};
}  // namespace

DevsZ80::DevsZ80(uint16_t addr, const Mems *mems)
    : _usart(new I8251()), _disk(mems ? new SectorDisk(mems, CPM_FORMATS, 2) : nullptr) {
    _usart->setBaseAddr(addr);
    if (_disk)
        _disk->setBaseAddr(addr + 0x10);
}

DevsZ80::~DevsZ80() {
    delete _usart;
    delete _disk;
}

void DevsZ80::reset() {
    _usart->reset();
    if (_disk)
        _disk->reset();
}

Disk *DevsZ80::disk() const {
    return _disk;
}

void DevsZ80::begin() {
    enableDevice(_usart);
}

bool DevsZ80::loopDevices() {
    return _usart->loop();
}

bool DevsZ80::isSelected(uint32_t addr) const {
    return _usart->isSelected(addr) || (_disk && _disk->isSelected(addr));
}

uint16_t DevsZ80::read(uint32_t addr) const {
    if (_disk && _disk->isSelected(addr))
        return _disk->read(addr);
    return _usart->read(addr);
}

void DevsZ80::write(uint32_t addr, uint16_t data) const {
    if (_disk && _disk->isSelected(addr)) {
        _disk->write(addr, data);
    } else {
        _usart->write(addr, data);
    }
}

uint16_t DevsZ80::vector() const {
    return _usart->vector();
}

Device *DevsZ80::parseDevice(const char *name) const {
    if (strcasecmp(name, _usart->name()) == 0)
        return _usart;
    if (_disk && strcasecmp(name, _disk->name()) == 0)
        return _disk;
    return Devs::nullDevice();
}

// The disk turns on as its images mount, and I DISK turns it off and on;
// the console stays as it is.
void DevsZ80::enableDevice(Device *dev) {
    if (_disk && dev == _disk) {
        _disk->enable(!_disk->isEnabled());
        return;
    }
    _usart->enable(dev == _usart);
}

void DevsZ80::printDevices() const {
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
