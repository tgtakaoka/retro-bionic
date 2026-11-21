#ifndef __DEBUGGER_SECTOR_DISK_H__
#define __DEBUGGER_SECTOR_DISK_H__

#include "device.h"
#include "disk_drives.h"
#include "mems.h"

namespace debugger {

/**
 * A paravirtual CP/M disk on eight byte-wide I/O ports, with four drives
 * of SD card images. A COMMAND write moves one sector to or from DMA
 * while the CPU waits.
 *
 *   +0 DRIVE    write: 0-3
 *               read: the drive in bits 0-3, its format's index in 4-7
 *   +1 TRACK    cylinder x heads + head, from 0
 *   +2 SECTOR   from the format's first sector
 *   +3 DMAL     DMA address, low byte
 *   +4 DMAH     high byte
 *   +5 COMMAND  write: 1 reads a sector, 2 writes one
 *               read: the last command's status, 0 when it succeeded
 *   +6 MOUNTED  read: bit n is set when drive n has its image
 *   +7 DMAX     DMA address bits 16-23, past a 64K space
 *
 * The target gives the formats; see DiskDrives::place().
 */
struct SectorDisk final : Device, DiskDrives {
    static constexpr uint8_t DRIVES = 4;

    SectorDisk(const Mems *mems, const DiskGeometry *formats, uint8_t count)
        : DiskDrives(_drives, DRIVES, formats, count), _mems(mems) {}

    const char *name() const override;
    const char *description() const override;
    void print() const override;
    void reset() override;
    bool loop() override { return false; }
    bool isSelected(uint32_t addr) const override {
        return _enabled && (addr & ~7) == _base_addr;
    }
    void setBaseAddr(uint32_t addr) override { _base_addr = addr & ~7; }
    uint32_t baseAddr() const override { return _base_addr; }
    uint16_t read(uint32_t addr) override { return peek(addr); }
    uint16_t peek(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) override;

    // Turns the disk on.
    Mounted mount(const char *name, const char *path) override;
    // |path|'s .dsk, b.dsk, c.dsk and d.dsk as A-D.
    void mountBeside(const char *path) override;

protected:
    // A-D
    const char *unitName(uint8_t unit) const override;
    uint8_t unitOf(const char *name) const override;

private:
    DiskUnit _drives[DRIVES];
    const Mems *const _mems;
    uint32_t _base_addr = 0;
    uint8_t _drive = 0;
    uint8_t _track = 0;
    uint8_t _sector = 1;
    uint32_t _dma = 0;
    uint8_t _status = 0;

    void transfer(bool toDisk);
    uint32_t dmaAt(uint16_t i) const { return (_dma + i) & _mems->maxAddr(); }
};

}  // namespace debugger
#endif /* __DEBUGGER_SECTOR_DISK_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
