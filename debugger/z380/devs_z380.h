#ifndef __DEVS_Z380_H__
#define __DEVS_Z380_H__

#include "z80/devs_z80.h"

// The Z80's USART, above FFH where only a 16- or 32-bit I/O address
// reaches it.
#undef USART_BASE
#define USART_BASE 0x0140
#define ADDR_MASK_BASE 0x0148

namespace debugger {
namespace z380 {

struct MemsZ380;

/**
 * The address mask register, one byte-wide I/O port: the number of
 * address lines memory decodes, from A0. It is 32 after a reset; a 64K
 * system writes 16, so memory mirrors as on a board that decodes only
 * A0-A15, wherever above it the CPU reaches.
 */
struct AddressMask final : Device {
    explicit AddressMask(MemsZ380 *mems) : _mems(mems) { _enabled = true; }

    const char *name() const override { return "AMASK"; }
    const char *description() const override { return "Addr mask"; }
    void print() const override;
    void reset() override;
    bool loop() override { return false; }
    uint16_t read(uint32_t addr) override { return peek(addr); }
    uint16_t peek(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) override;
    uint32_t baseAddr() const override { return ADDR_MASK_BASE; }

private:
    static constexpr uint8_t ALL_BITS = 32;

    MemsZ380 *const _mems;
    uint8_t _bits = ALL_BITS;
};

// The Z80's devices and the address mask register.
struct DevsZ380 final : Devs {
    explicit DevsZ380(MemsZ380 *mems);
    ~DevsZ380();

    void begin() override { _devs->begin(); }
    void reset() override;
    bool isSelected(uint32_t addr) const override;
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;
    uint16_t vector() const override { return _devs->vector(); }

    Device *parseDevice(const char *name) const override;
    void enableDevice(Device *dev) override;
    void printDevices() const override;
    Disk *disk() const override { return _devs->disk(); }

protected:
    bool loopDevices() override { return _devs->loopDevices(); }

private:
    DevsZ80 *const _devs;
    AddressMask *const _mask;
};

}  // namespace z380
}  // namespace debugger

#endif /* __DEVS_Z380_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
