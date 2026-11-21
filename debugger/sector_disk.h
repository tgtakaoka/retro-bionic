#ifndef __DEBUGGER_SECTOR_DISK_H__
#define __DEBUGGER_SECTOR_DISK_H__

#include "device.h"
#include "mems.h"

namespace debugger {

/**
 * A paravirtual disk on eight I/O ports, for CP/M: four drives of 8"
 * single-sided single-density geometry, 77 tracks of 26 sectors of 128
 * bytes. Each drive is an image file on the SD card. A write to COMMAND
 * moves one sector between the image and the memory at DMA, whole,
 * while the CPU waits in that bus cycle.
 *
 *   +0 DRIVE    0-3
 *   +1 TRACK    0-76
 *   +2 SECTOR   1-26, physical
 *   +3 DMAL     DMA address, low byte
 *   +4 DMAH     high byte
 *   +5 COMMAND  write: 1 reads a sector, 2 writes one
 *               read: the last command's status, 0 when it succeeded
 *   +6 MOUNTED  read: bit n is set when drive n has its image
 */
struct SectorDisk final : Device {
    static constexpr uint8_t DRIVES = 4;
    static constexpr uint8_t TRACKS = 77;
    static constexpr uint8_t SECTORS = 26;
    static constexpr uint16_t SECTOR_BYTES = 128;

    SectorDisk(const Mems *mems) : _mems(mems) {}

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

    // Mounts the images beside |path|, a file loaded from the SD card:
    // its name with .dsk is drive A, and with b.dsk, c.dsk and d.dsk the
    // others. Turns the disk on when drive A is there.
    void mount(const char *path);

private:
    const Mems *const _mems;
    uint32_t _base_addr = 0;
    uint8_t _drive = 0;
    uint8_t _track = 0;
    uint8_t _sector = 1;
    uint16_t _dma = 0;
    uint8_t _status = 0;

    void transfer(bool toDisk);
};

}  // namespace debugger
#endif /* __DEBUGGER_SECTOR_DISK_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
