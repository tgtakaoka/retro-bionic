#include "devs_i8086.h"
#include <strings.h>
#include "pc_disk.h"
#include "timer.h"

namespace debugger {
namespace i8086 {

DevsI8086::DevsI8086(const Mems *mems)
    : DevsI8080(new PcDisk(mems)), _timer(new Timer()) {
    _timer->setBaseAddr(TIMER_BASE);
}

DevsI8086::~DevsI8086() {
    delete _timer;
}

void DevsI8086::begin() {
    DevsI8080::begin();
    _timer->enable(true);
}

void DevsI8086::reset() {
    _timer->reset();
    DevsI8080::reset();
}

bool DevsI8086::loopDevices() {
    _timer->loop();
    return DevsI8080::loopDevices();
}

bool DevsI8086::isSelected(uint32_t addr) const {
    return _timer->isSelected(addr) || DevsI8080::isSelected(addr);
}

uint16_t DevsI8086::read(uint32_t addr) const {
    if (_timer->isSelected(addr))
        return _timer->read(addr);
    return DevsI8080::read(addr);
}

void DevsI8086::write(uint32_t addr, uint16_t data) const {
    if (_timer->isSelected(addr)) {
        _timer->write(addr, data);
    } else {
        DevsI8080::write(addr, data);
    }
}

// The tick comes before the console, as IRQ0 does.
uint16_t DevsI8086::vector() const {
    const auto type = _timer->vector();
    return type ? type : DevsI8080::vector();
}

Device *DevsI8086::parseDevice(const char *name) const {
    if (strcasecmp(name, _timer->name()) == 0)
        return _timer;
    return DevsI8080::parseDevice(name);
}

// I TIMER toggles the timer; the others as the 8080 has them.
void DevsI8086::enableDevice(Device *dev) {
    if (dev == _timer) {
        _timer->enable(!_timer->isEnabled());
        return;
    }
    DevsI8080::enableDevice(dev);
}

void DevsI8086::printDevices() const {
    DevsI8080::printDevices();
    printDevice(_timer);
}

}  // namespace i8086
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
