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
    void reset() override {}
    void idle() override {}
    bool step(bool show) override { return false; }
    void run() override {}
    void assertInt(uint8_t name) override { (void)name; }
    void negateInt(uint8_t name) override { (void)name; }
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

Target::Target(const char *id, Pins &pins, Regs &regs, Mems &mems, Devs &devs)
    : _identity(id),
      _pins(&pins),
      _regs(&regs),
      _mems(&mems),
      _devs(&devs),
      _next(_head) {
    _head = this;
}

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
    if (strcasecmp(identity, Debugger.regs().cpuName()) != 0) {
        cli.print(" (CPU: ");
        cli.print(Debugger.regs().cpuName());
        cli.print(')');
    }
}

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
