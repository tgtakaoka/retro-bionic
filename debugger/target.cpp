#include "target.h"

namespace debugger {

Target::Target(const Identity *id, Pins *pins)
    : _id(id),
      _pins(pins),
      _regs(pins->_regs),
      _mems(pins->_mems),
      _devs(pins->_devs) {}

void Target::begin() const {
    reset();
    _devs->begin();
}

void Target::reset() const {
    _pins->reset();
    _devs->reset();
}

void Target::run() const {
    _pins->setRun();
    _devs->setIdle(false);
    _pins->run();
    _devs->setIdle(true);
    _pins->setHalt();
}

bool Target::step(bool show) const {
    return _pins->step(show);
}

void Target::idle() const {
    _pins->idle();
}

void Target::assertInt(uint8_t name) const {
    _pins->assertInt(name);
}

void Target::negateInt(uint8_t name) const {
    _pins->negateInt(name);
}

void Target::printCycles() const {
    _pins->printCycles();
}

uint16_t Target::getInst(uint32_t addr) const {
    return _mems->get_prog(addr);
}

void Target::putInst(uint32_t addr, uint16_t data) const {
    _mems->put_prog(addr, data);
}

void Target::printRegisters(bool dis) const {
    _regs->print();
    if (dis)
        disassemble(_regs->nextIp(), 1);
}

uint_fast8_t Target::validRegister(const char *word, uint32_t &max) const {
    return _regs->validRegister(word, max);
}

bool Target::setRegister(uint_fast8_t reg, uint32_t value) const {
    return _regs->setRegister(reg, value);
}

void Target::helpRegisters() const {
    _regs->helpRegisters();
}

uint32_t Target::maxAddr() const {
    return _mems->maxAddr();
}

uint32_t Target::maxData() const {
    return _mems->maxData();
}

uint32_t Target::assemble(uint32_t addr, const char *line) const {
    return _mems->assemble(addr, line);
}

uint32_t Target::disassemble(uint32_t addr, uint_fast8_t numInsn) const {
    return _mems->disassemble(addr, numInsn);
}

void Target::dumpMemory(uint32_t addr, uint16_t len, bool prog) const {
    _mems->dumpMemory(addr, len, prog);
}

void Target::writeMemory(uint32_t addr, const uint16_t *buffer,
        uint_fast8_t len, bool prog) const {
    _mems->writeMemory(addr, buffer, len, prog);
}

void Target::loadCode(
        uint32_t byte_addr, const uint8_t *code, uint_fast8_t len) const {
    const auto unit = addressUnit();
    auto addr = byte_addr / unit;
    const auto wordAccess = _mems->wordAccess() && unit == 1;
    for (uint_fast8_t i = 0; i < len;) {
        uint16_t data = code[i++];
        if (unit == 2 || wordAccess) {
            const uint16_t next = (i < len) ? code[i++] : 0;
            if (endian() == ENDIAN_BIG) {
                data <<= 8;
                data |= next;
            } else {
                data |= next << 8;
            }
        }
        _mems->put_prog(addr++, data);
        if (wordAccess)
            addr++;
    }
}

bool Target::hasProtectArea() const {
    return _mems->protectArea() != nullptr;
}

bool Target::printProtectArea() const {
    auto *area = _mems->protectArea();
    if (area == nullptr)
        return false;
    area->print();
    return true;
}

void Target::setProtectArea(uint32_t begin, uint32_t end) const {
    auto *area = _mems->protectArea();
    if (area)
        area->set(begin, end);
}

void Target::printDevices() const {
    _devs->printDevices();
}

Device *Target::parseDevice(const char *word) const {
    return _devs->parseDevice(word);
}

void Target::enableDevice(Device *dev) const {
    _devs->enableDevice(dev);
}

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
