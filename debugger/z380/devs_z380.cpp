#include "devs_z380.h"
#include <strings.h>
#include "debugger.h"
#include "mems_z380.h"

namespace debugger {
namespace z380 {

void AddressMask::print() const {
    cli.print("  ");
    cli.printDec(_bits);
    cli.print(" bits ");
    cli.printHex(_mems->busMask(), 8);
    cli.println();
}

void AddressMask::reset() {
    write(ADDR_MASK_BASE, ALL_BITS);
}

uint16_t AddressMask::peek(uint32_t) const {
    return _bits;
}

// A count that is none or past 32 is all of them.
void AddressMask::write(uint32_t, uint16_t data) {
    _bits = (data == 0 || data > ALL_BITS) ? ALL_BITS : data;
    _mems->setBusMask(
            _bits == ALL_BITS ? ~UINT32_C(0) : (UINT32_C(1) << _bits) - 1);
}

DevsZ380::DevsZ380(MemsZ380 *mems)
    : _devs(new DevsZ80(USART_BASE, mems)), _mask(new AddressMask(mems)) {}

DevsZ380::~DevsZ380() {
    delete _devs;
    delete _mask;
}

void DevsZ380::reset() {
    _devs->reset();
    _mask->reset();
}

bool DevsZ380::isSelected(uint32_t addr) const {
    return _mask->isSelected(addr) || _devs->isSelected(addr);
}

uint16_t DevsZ380::read(uint32_t addr) const {
    return _mask->isSelected(addr) ? _mask->read(addr) : _devs->read(addr);
}

void DevsZ380::write(uint32_t addr, uint16_t data) const {
    if (_mask->isSelected(addr)) {
        _mask->write(addr, data);
    } else {
        _devs->write(addr, data);
    }
}

Device *DevsZ380::parseDevice(const char *name) const {
    if (strcasecmp(name, _mask->name()) == 0)
        return _mask;
    return _devs->parseDevice(name);
}

// The address mask register is always there.
void DevsZ380::enableDevice(Device *dev) {
    if (dev != _mask)
        _devs->enableDevice(dev);
}

void DevsZ380::disableDevice(Device *dev) {
    if (dev != _mask)
        _devs->disableDevice(dev);
}

void DevsZ380::printDevices() const {
    _devs->printDevices();
    printDevice(_mask);
}

}  // namespace z380
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
