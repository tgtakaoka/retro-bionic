#include "devs_pdp11.h"
#include <strings.h>
#include "debugger.h"

namespace debugger {
namespace pdp11 {

Disk::Mounted DisksPdp11::mount(const char *name, const char *path) {
    const auto mounted = _rk11->mount(name, path);
    return mounted == MOUNT_NO_UNIT ? _rq->mount(name, path) : mounted;
}

bool DisksPdp11::unmount(const char *name) {
    return _rk11->unmount(name) || _rq->unmount(name);
}

// An RK05 pack or a slotted image goes on the RK11, any other on the
// MSCP controller, and the other's units are cleared.
void DisksPdp11::mountBeside(const char *path) {
    _rk11->mountBeside(path);
    if (_rk11->anyMounted()) {
        _rq->unmountAll();
    } else {
        _rq->mountBeside(path);
    }
}

void DisksPdp11::printDrives() const {
    if (!_rk11->anyMounted() && !_rq->anyMounted()) {
        cli.println("  no disk mounted");
        return;
    }
    if (_rk11->anyMounted())
        _rk11->printDrives();
    if (_rq->anyMounted())
        _rq->printDrives();
}

DevsPdp11::DevsPdp11(const Mems *dma)
    : _dl11(new Dl11()),
      _kw11l(new Kw11l()),
      _rk11(new Rk11(dma)),
      _rq(new Rq(dma)),
      _swr(new Swr()),
      _disks(new DisksPdp11(_rk11, _rq)),
      _system(dma != nullptr) {
    _dl11->setBaseAddr(DL11_BASE);
    _kw11l->setBaseAddr(KW11L_BASE);
    _rk11->setBaseAddr(RK11_BASE);
    _rq->setBaseAddr(RQ_BASE);
}

DevsPdp11::~DevsPdp11() {
    delete _disks;
    delete _dl11;
    delete _kw11l;
    delete _rk11;
    delete _rq;
    delete _swr;
}

void DevsPdp11::reset() {
    _dl11->reset();
    _kw11l->reset();
    _rk11->reset();
    _rq->reset();
}

void DevsPdp11::begin() {
    enableDevice(_dl11);
    _kw11l->enable(_system);
    _rk11->enable(_system);
    _rq->enable(_system);
    _swr->enable(_system);
}

Disk *DevsPdp11::disk() const {
    return _system ? _disks : nullptr;
}

bool DevsPdp11::loopDevices() {
    _kw11l->loop();
    _rk11->loop();
    _rq->loop();
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

void DevsPdp11::writeByte(uint32_t addr, uint8_t data) const {
    const auto dev = select(addr);
    if (dev != _rk11) {
        dev->write(addr, data);
        return;
    }
    const uint32_t word = addr & ~1;
    const uint16_t old = _rk11->peek(word);
    _rk11->write(word, (addr & 1) ? (old & 0377) | (data << 8)
                                  : (old & ~0377) | data);
}

uint16_t DevsPdp11::vector() const {
    return _dl11->acknowledge();
}

uint16_t DevsPdp11::vector(uint_fast8_t level) const {
    if (level == 5) {
        if (_rk11->requesting()) {
            _rk11->acknowledge();
            return Rk11::VECTOR;
        }
        if (_rq->requesting()) {
            _rq->acknowledge();
            return Rq::VECTOR;
        }
    }
    return _dl11->acknowledge();
}

bool DevsPdp11::requesting(uint_fast8_t level) const {
    return level == 5 && (_rk11->requesting() || _rq->requesting());
}

Device *DevsPdp11::parseDevice(const char *name) const {
    if (strcasecmp(name, _dl11->name()) == 0)
        return _dl11;
    if (_system && strcasecmp(name, _kw11l->name()) == 0)
        return _kw11l;
    if (_system && strcasecmp(name, _rk11->name()) == 0)
        return _rk11;
    if (_system && strcasecmp(name, _rq->name()) == 0)
        return _rq;
    if (_system && strcasecmp(name, _swr->name()) == 0)
        return _swr;
    return Devs::nullDevice();
}

// I dev turns the line clock, a disk or the switches on, and the console
// stays as it is; the DL11 is the only console.
void DevsPdp11::enableDevice(Device *dev) {
    if (dev == _kw11l || dev == _rk11 || dev == _rq || dev == _swr) {
        dev->enable(true);
        return;
    }
    _dl11->enable(dev == _dl11);
}

void DevsPdp11::printDevices() const {
    printDevice(_dl11);
    if (_system) {
        printDevice(_kw11l);
        printDevice(_rk11);
        printDevice(_rq);
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
