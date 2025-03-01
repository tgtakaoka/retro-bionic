#ifndef __MC68HC11_INIT_H__
#define __MC68HC11_INIT_H__

#include "device.h"

namespace debugger {
namespace mc68hc11 {

struct RegsMc68hc11;

struct Mc68hc11Init final : Device {
    Mc68hc11Init(uint8_t init, uint8_t ram_offset, uint16_t ram_size,
            uint8_t dev_size)
        : Device(),
          _init(init),
          _ram_offset(ram_offset),
          _ram_size(ram_size),
          _dev_size(dev_size) {
        enable(true);
        set(init);
    }

    const char *name() const override { return "INIT"; }
    const char *description() const override;
    void print() const override;

    void reset() override {}
    void loop() override {}
    void setBaseAddr(uint32_t addr) override { set(addr); }
    uint32_t baseAddr() const override { return _dev_base + INIT; }

    void configSystem(RegsMc68hc11 *regs);

    bool is_internal(uint16_t addr) const;
    uint16_t dev_base() const { return _dev_base; }

private:
    const uint8_t _init;
    const uint8_t _ram_offset;
    const uint16_t _ram_size;
    const uint8_t _dev_size;
    uint16_t _ram_base;
    uint16_t _dev_base;

    uint8_t init() const { return (_ram_base >> 8) | (_dev_base >> 12); }
    void set(uint8_t init) {
        _ram_base = ((init & 0xF0) << 8) + _ram_offset;
        _dev_base = (init & 0x0F) << 12;
    }

    static constexpr uint16_t OPTION = 0x39;
    static constexpr uint16_t INIT = 0x3D;
    static constexpr uint16_t CONFIG = 0x3F;
};

}  // namespace mc68hc11
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
