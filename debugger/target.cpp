#include "target.h"
#include <string.h>
#include "config_debugger.h"
#include "debugger.h"
#include "unio_bus.h"
#include "unio_eeprom.h"

namespace debugger {

unio::UnioBus unioBus{PIN_ID, 16};

namespace nulldev {
struct NullDevs final : Devs {
    void begin() override {}
    void reset() override {}
    void loop() override {}
} Devs;

struct NullMems final : Mems {
    NullMems() : Mems(Endian::ENDIAN_BIG) {}
    uint32_t maxAddr() const override { return 0; }
    uint16_t raw_read(uint32_t addr) const override {
        (void)addr;
        return 0;
    }
    void raw_write(uint32_t addr, uint16_t data) const override {
        (void)addr;
        (void)data;
    }

protected:
    libasm::Assembler *assembler() const override { return nullptr; }
    libasm::Disassembler *disassembler() const override { return nullptr; }
} Mems;

struct NullPins final : Pins {
    void resetPins() override {}
    void idle() override {}
    bool step(bool show) override { return false; }
    void run() override {}
    void assertInt(uint8_t name) override { (void)name; }
    void negateInt(uint8_t name) override { (void)name; }
    void printCycles() override {}
    void setBreakInst(uint32_t addr) const override { (void)addr; }
} Pins;

struct NullRegs final : Regs {
    const char *cpu() const override { return "none"; }
    const char *cpuName() const override { return cpu(); }
    void print() const override {}
    void save() override {}
    void restore() override {}
    uint32_t nextIp() const override { return 0; }
    void helpRegisters() const override {}
    const RegList *listRegisters(uint8_t n) const override {
        (void)n;
        return nullptr;
    }
} Regs;

struct Target TargetNull {
    "", Pins, Regs, Mems, Devs
};

}  // namespace nulldev

Target *Target::_head = nullptr;

Target &Target::searchIdentity(const char *identity) {
    for (auto *target = _head; target; target = target->_next) {
        if (strcasecmp(identity, target->_identity) == 0)
            return *target;
    }
    return nulldev::TargetNull;
}

Target &Target::readIdentity() {
    unioBus.standby();
    unio::Eeprom rom{unioBus};
    uint8_t buffer[16];
    if (rom.read(0, sizeof(buffer), buffer)) {
        const auto *identity = reinterpret_cast<const char *>(buffer);
        const auto len = strnlen(identity, sizeof(buffer));
        if (len < sizeof(buffer))
            return searchIdentity(identity);
    }
    return nulldev::TargetNull;
}

void Target::writeIdentity(const char *identity) {
    const auto len = strlen(identity);
    unioBus.standby();
    unio::Eeprom rom{unioBus};
    const auto *buffer = reinterpret_cast<const uint8_t *>(identity);
    const auto valid = rom.write(0, len + 1, buffer);
    if (valid) {
        cli.println("DONE");
    } else {
        cli.println("FAILED");
    }
}

void Target::printIdentity() {
    unioBus.standby();
    unio::Eeprom rom{unioBus};
    uint8_t buffer[16];
    const auto valid = rom.read(0, sizeof(buffer), buffer);
    const auto *identity = reinterpret_cast<const char *>(buffer);
    if (valid) {
        cli.print(identity);
    } else {
        cli.print("?????");
    }
    if (strcasecmp(identity, Debugger.target()._regs.cpuName()) != 0) {
        cli.print(" (CPU: ");
        cli.print(Debugger.target()._regs.cpuName());
        cli.print(')');
    }
}

Target::Target(const char *id, Pins &pins, Regs &regs, Mems &mems, Devs &devs)
    : _identity(id),
      _pins(pins),
      _regs(regs),
      _mems(mems),
      _devs(devs),
      _next(_head) {
    _head = this;
}

void Target::begin() const {
    reset();
    _devs.begin();
}

void Target::reset() const {
    _pins.reset();
    _devs.reset();
}

void Target::run() const {
    _pins.setRun();
    _devs.setIdle(false);
    _pins.run();
    _devs.setIdle(true);
    _pins.setHalt();
}

bool Target::step(bool show) const {
    return _pins.step(show);
}

void Target::idle() const {
    _pins.idle();
}

void Target::assertInt(uint8_t name) const {
    _pins.assertInt(name);
}

void Target::negateInt(uint8_t name) const {
    _pins.negateInt(name);
}

void Target::printCycles() const {
    _pins.printCycles();
}

bool Target::printBreakPoints() const {
    return _pins.printBreakPoints();
}

bool Target::setBreakPoint(uint32_t addr) const {
    return _pins.setBreakPoint(addr);
}

bool Target::clearBreakPoint(uint8_t index) const {
    return _pins.clearBreakPoint(index);
}

bool Target::isOnBreakPoint() const {
    return _pins.isBreakPoint(_regs.nextIp());
}

void Target::printRegisters() const {
    _regs.print();
}

void Target::printStatus() const {
    printRegisters();
    disassemble(_regs.nextIp(), 1);
}

uint8_t Target::validRegister(const char *word, uint32_t &max) const {
    return _regs.validRegister(word, max);
}

void Target::setRegister(uint8_t reg, uint32_t value) const {
    _regs.setRegister(reg, value);
}

void Target::helpRegisters() const {
    _regs.helpRegisters();
}

uint32_t Target::maxAddr() const {
    return _mems.maxAddr();
}

uint32_t Target::assemble(uint32_t addr, const char *line) const {
    return _mems.assemble(addr, line);
}

uint32_t Target::disassemble(uint32_t addr, uint8_t numInsn) const {
    return _mems.disassemble(addr, numInsn);
}

uint16_t Target::read_memory(uint32_t addr, const char *space) const {
    return _mems.get(addr, space);
}

uint16_t Target::get_inst(uint32_t addr) const {
    return _mems.get_inst(addr);
}

void Target::put_inst(uint32_t addr, uint16_t data) const {
    _mems.put_inst(addr, data);
}

void Target::write_memory(
        uint32_t addr, const uint8_t *buffer, uint8_t len) const {
    _mems.put(addr, buffer, len);
}

void Target::write_code(
        uint32_t addr, const uint8_t *buffer, uint8_t len) const {
    _mems.put(addr, buffer, len);
}

bool Target::printRomArea() const {
    auto *rom = _mems.romArea();
    if (rom == nullptr)
        return false;
    rom->print();
    return true;
}

void Target::setRomArea(uint32_t begin, uint32_t end) const {
    auto *rom = _mems.romArea();
    if (rom)
        rom->set(begin, end);
}

void Target::printDevices() const {
    _devs.printDevices();
}

Device &Target::parseDevice(const char *word) const {
    return _devs.parseDevice(word);
}

void Target::enableDevice(Device &dev) const {
    _devs.enableDevice(dev);
}

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
