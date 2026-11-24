#ifndef __RK11_H__
#define __RK11_H__

#include "device.h"
#include "disk_drives.h"
#include "mems.h"

namespace debugger {
namespace pdp11 {

/**
 * RK11 disk controller with eight RK05 drives: RKDS, RKER, RKCS, RKWC,
 * RKBA, RKDA and RKDB, a word each from 177400. The drives are images on
 * the SD card: O mounts an RK05 pack's file on one, and L the slots of
 * the image named as the bootstrap it loaded, the drum's 1024 blocks and
 * then 6144 a drive. A transfer is done whole within the
 * write that starts it, between the file and the memory at RKBA, and
 * the next service sets READY and requests the interrupt.
 */
struct Rk11 final : Device, DiskDrives {
    static constexpr uint8_t DRIVES = 8;
    // The interrupt's name, its vector; level 5.
    static constexpr uint8_t VECTOR = 0220;

    Rk11(const Mems *mems);

    const char *name() const override;
    const char *description() const override;
    void print() const override;
    void reset() override;
    bool loop() override;
    void enable(bool enabled) override;
    bool isSelected(uint32_t addr) const override {
        return _enabled && (addr & ~017) == _base_addr;
    }
    void setBaseAddr(uint32_t addr) override { _base_addr = addr & ~017; }
    uint32_t baseAddr() const override { return _base_addr; }
    uint16_t read(uint32_t addr) override { return peek(addr); }
    uint16_t peek(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) override;

    // The slots of |path|'s .dsk that the file reaches, as rk0-rk7.
    void mountBeside(const char *path) override;
    // Whether it requests an interrupt.
    bool requesting() const { return _request; }
    // The CPU took the interrupt.
    void acknowledge();

protected:
    // rk0-rk7
    const char *unitName(uint8_t unit) const override;
    uint8_t unitOf(const char *name) const override;
    // A pack's file starts its drive; a longer one is the slotted image,
    // the drum and whole drives, and |unit|'s slot stands where that has it.
    bool place(uint8_t unit, uint32_t bytes, const DiskGeometry *&format,
            uint32_t &offset) const override;

private:
    DiskUnit _drives[DRIVES];
    const Mems *const _mems;
    uint32_t _base_addr = 0;
    uint16_t _rker = 0;
    uint16_t _rkcs = 0;
    uint16_t _rkwc = 0;
    uint16_t _rkba = 0;
    uint16_t _rkda = 0;
    bool _ready = true;
    bool _done = false;  // a function ended; READY comes on the next service
    bool _request = false;

    static constexpr uint8_t RKDS = 000;
    static constexpr uint8_t RKER = 002;
    static constexpr uint8_t RKCS = 004;
    static constexpr uint8_t RKWC = 006;
    static constexpr uint8_t RKBA = 010;
    static constexpr uint8_t RKDA = 012;

    void start();
    void transfer(bool toDisk);
};

}  // namespace pdp11
}  // namespace debugger
#endif /* __RK11_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
