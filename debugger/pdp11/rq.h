#ifndef __RQ_H__
#define __RQ_H__

#include "device.h"
#include "disk_drives.h"
#include "mems.h"
#include "mscp.h"

namespace debugger {
namespace pdp11 {

/**
 * An RQDX3 MSCP disk controller: IP and SA, a word each from 172150,
 * vector 154 at level 5, with four units, ra0-ra3. Their media are
 * image files on the SD card, each unit as large as its file: O mounts
 * one, and L mounts x.dsk as ra0 and x1.dsk-x3.dsk as ra1-ra3 beside the
 * x.hex it loaded. A command is taken whole, its transfer with it, in
 * the service after the host's poll, and the end packet's interrupt
 * follows.
 */
struct Rq final : Device, DiskDrives, MscpHost {
    static constexpr uint8_t UNITS = Mscp::UNITS;
    // The interrupt's name, its vector; level 5.
    static constexpr uint8_t VECTOR = 0154;

    Rq(const Mems *mems);

    const char *name() const override;
    const char *description() const override;
    void print() const override;
    void reset() override;
    bool loop() override;
    void enable(bool enabled) override;
    bool isSelected(uint32_t addr) const override {
        return _enabled && (addr & ~3) == _base_addr;
    }
    void setBaseAddr(uint32_t addr) override { _base_addr = addr & ~3; }
    uint32_t baseAddr() const override { return _base_addr; }
    uint16_t read(uint32_t addr) override;
    uint16_t peek(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) override;

    void mountBeside(const char *path) override;
    void unmountAll();
    // Whether it requests an interrupt.
    bool requesting() const { return _request; }
    // The CPU took the interrupt.
    void acknowledge();

    // MscpHost
    uint16_t readWord(uint32_t addr) const override {
        return _mems->read16(addr);
    }
    void writeWord(uint32_t addr, uint16_t data) const override {
        _mems->write16(addr, data);
    }
    uint32_t unitBlocks(uint8_t unit) const override;
    bool transfer(uint8_t unit, uint32_t lbn, uint32_t addr, uint32_t bytes,
            bool toDisk) override;
    void interrupt() override;

protected:
    // ra0-ra3
    const char *unitName(uint8_t unit) const override;
    uint8_t unitOf(const char *name) const override;
    // The file's size, in cylinders of 256 blocks, the last one short:
    // the controller bounds a transfer by the blocks the file holds.
    bool place(uint8_t unit, uint32_t bytes, const DiskGeometry *&format,
            uint32_t &offset) const override;

private:
    DiskUnit _units[UNITS];
    // Each unit's geometry and blocks come from its file as it mounts.
    mutable DiskGeometry _geometry[UNITS];
    mutable uint32_t _blocks[UNITS];
    const Mems *const _mems;
    Mscp _mscp;
    uint32_t _base_addr = 0;
    bool _request = false;

    static constexpr uint8_t IP = 0;
    static constexpr uint8_t SA = 2;
};

}  // namespace pdp11
}  // namespace debugger
#endif /* __RQ_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
