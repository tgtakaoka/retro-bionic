#include "identity.h"
#include "target.h"

namespace debugger {

struct DevsNull final : Devs {
    void begin() override {}
    void reset() override {}
    void loop() override {}
};

struct MemsNull final : Mems {
    MemsNull() : Mems(Endian::ENDIAN_BIG) {}
    uint32_t maxAddr() const override { return 0; }
    uint16_t raw_read(uint32_t) const override { return 0; }
    void raw_write(uint32_t, uint16_t) const override {}
};

struct RegsNull final : Regs {
    const char *cpu() const override { return "none"; }
    const char *cpuName() const override { return cpu(); }
    void print() const override {}
    void save() override {}
    void restore() override {}
    uint32_t nextIp() const override { return 0; }
    void helpRegisters() const override {}
    const RegList *listRegisters(uint8_t) const override { return nullptr; }
};

struct PinsNull final : Pins {
    PinsNull() {
        _regs = new RegsNull();
        _mems = new MemsNull();
        _devs = new DevsNull();
    }
    void resetPins() override {}
    void idle() override {}
    bool step(bool show) override { return false; }
    void run() override {}
    void assertInt(uint8_t name) override { (void)name; }
    void negateInt(uint8_t name) override { (void)name; }
    void setBreakInst(uint32_t addr) const override { (void)addr; }
    void printCycles() override {}
};

Pins *instanceNull() {
    return new PinsNull();
}

const struct Identity IdentityNull("", instanceNull);

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
