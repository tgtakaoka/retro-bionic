#include "devs_mc6809.h"
#include <strings.h>

namespace debugger {
namespace mc6809 {

namespace {
// The RAM disk's formats, NitrOS-9's RBF disks of 256-byte sectors: a
// 35-track CoCo .dsk, which any image up to its size mounts as, and hard
// disks of 16384 and 32768 sectors, images of exactly that size.
const DiskGeometry RBF_FORMATS[] = {
        {"coco35", 35, 1, 18, 256, 0, 0},
        {"os9-4m", 64, 1, 256, 256, 0, 0},
        {"os9-8m", 128, 1, 256, 256, 0, 0},
};
}  // namespace

DevsMc6809::DevsMc6809()
    : _acia(new Mc6850()),
      _tick(new Tick()),
      _disk(new RamDisk(RBF_FORMATS, 3, 0x10000)) {
    _acia->setBaseAddr(ACIA_BASE);
    _tick->setBaseAddr(TICK_BASE);
    _disk->setBaseAddr(DISK_BASE);
}

DevsMc6809::~DevsMc6809() {
    delete _acia;
    delete _tick;
    delete _disk;
}

void DevsMc6809::reset() {
    _acia->reset();
    _tick->reset();
    _disk->reset();
}

// The disk turns on as its images mount.
void DevsMc6809::begin() {
    _acia->enable(true);
    _tick->enable(true);
}

bool DevsMc6809::loopDevices() {
    _tick->loop();
    _disk->loop();
    return _acia->loop();
}

// A halt lets a transfer in flight end, so the cache is whole for the
// sync that follows.
void DevsMc6809::setIdle(bool idle) {
    if (idle)
        _disk->settle();
}

uint16_t DevsMc6809::read(uint32_t addr) const {
    if (_disk->isSelected(addr))
        return _disk->read(addr);
    if (_tick->isSelected(addr))
        return _tick->read(addr);
    return _acia->read(addr);
}

void DevsMc6809::write(uint32_t addr, uint16_t data) const {
    if (_disk->isSelected(addr)) {
        _disk->write(addr, data);
    } else if (_tick->isSelected(addr)) {
        _tick->write(addr, data);
    } else {
        _acia->write(addr, data);
    }
}

Device *DevsMc6809::parseDevice(const char *name) const {
    if (strcasecmp(name, _acia->name()) == 0)
        return _acia;
    if (strcasecmp(name, _tick->name()) == 0)
        return _tick;
    if (strcasecmp(name, _disk->name()) == 0)
        return _disk;
    return Devs::nullDevice();
}

void DevsMc6809::enableDevice(Device *dev) {
    dev->enable(true);
}

void DevsMc6809::printDevices() const {
    printDevice(_acia);
    printDevice(_tick);
    printDevice(_disk);
}

}  // namespace mc6809
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
