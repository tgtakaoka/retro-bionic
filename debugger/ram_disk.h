#ifndef __DEBUGGER_RAM_DISK_H__
#define __DEBUGGER_RAM_DISK_H__

#include "device.h"
#include "disk_drives.h"

namespace debugger {

/**
 * A paravirtual disk for a CPU that cannot wait: its images sit whole in
 * the block cache, a command copies sectors by DMA while the CPU runs
 * on, and DONE raises an interrupt. Nothing reaches the card in a run;
 * written blocks go back at a halt, O sync or an unmount, so the cache
 * is write-back from the first mount on. Sixteen byte-wide registers:
 *
 *   +0 DRIVE    write: 0-3
 *               read: the drive in bits 0-3, the mounted drives in 4-7
 *   +1 LSNH     the logical sector number, bits 23-16
 *   +2 LSNM     bits 15-8
 *   +3 LSNL     bits 7-0
 *   +4 ADDRH    the buffer in memory, high byte
 *   +5 ADDRL    low byte
 *   +6 COMMAND  write: 1 reads COUNT sectors into the buffer, 2 writes
 *               them from it, 0 only acknowledges DONE
 *      STATUS   read: bit 7 DONE, from a command's end to the next
 *               write here; bit 0 BUSY; bits 1-3 the last command's
 *               error: 0 none, 1 no drive or image, 2 LSN past the end,
 *               3 buffer past memory, 4 sector not cached, 5 bad command
 *   +7 INTR     the interrupt name DONE raises; 0 raises none
 *   +8 COUNT    sectors per command, 1 after a reset
 *
 * The DMA writes the buffer behind the CPU's back, so a buffer should
 * not share a 32-byte cache line with memory the CPU writes meanwhile;
 * a page-aligned one never does. The target gives the formats, and
 * `L <stem>.s19` mounts <stem>d0.dsk-<stem>d3.dsk beside it.
 */
struct RamDisk final : Device, DiskDrives {
    static constexpr uint8_t DRIVES = 4;

    // |memoryBytes| is the target's memory, which a buffer stays within.
    RamDisk(const DiskGeometry *formats, uint8_t count, uint32_t memoryBytes)
        : DiskDrives(_drives, DRIVES, formats, count),
          _memoryBytes(memoryBytes) {}

    const char *name() const override;
    const char *description() const override;
    void print() const override;
    void reset() override;
    bool loop() override;
    bool isSelected(uint32_t addr) const override {
        return _enabled && (addr & ~15) == _base_addr;
    }
    void setBaseAddr(uint32_t addr) override { _base_addr = addr & ~15; }
    uint32_t baseAddr() const override { return _base_addr; }
    uint16_t read(uint32_t addr) override { return peek(addr); }
    uint16_t peek(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) override;

    // Turns the disk on. An image the cache cannot hold whole is
    // unmounted again, with a line saying so.
    Mounted mount(const char *name, const char *path) override;
    void mountBeside(const char *path) override;
    // Whether DONE waits with interrupt |name|.
    bool requesting(uint8_t name) const { return _done && _intr == name; }
    // Finishes a command in flight, for a halt: the cache then holds
    // what it wrote.
    void settle();

protected:
    // D0-D3
    const char *unitName(uint8_t unit) const override;
    uint8_t unitOf(const char *name) const override;
    bool reserveOnMount() const override { return true; }

private:
    DiskUnit _drives[DRIVES];
    const uint32_t _memoryBytes;
    uint32_t _base_addr = 0;
    uint8_t _drive = 0;
    uint32_t _lsn = 0;
    uint16_t _addr = 0;
    uint8_t _intr = 0;
    uint8_t _count = 1;
    uint8_t _error = 0;
    bool _done = false;
    // A command in flight: |_left| sectors from |_lsn| at |_addr|, and
    // the bytes the running DMA covers.
    bool _busy = false;
    bool _toDisk = false;
    uint16_t _left = 0;
    uint8_t *_dst = nullptr;
    uint16_t _len = 0;

    bool admit(uint8_t unit);
    void command(uint8_t cmd);
    void step();
    void finish(uint8_t error);
    void acknowledge();
    bool dmaDone() const;
    void dmaStart(const uint8_t *src, uint8_t *dst, uint16_t len);
};

}  // namespace debugger
#endif /* __DEBUGGER_RAM_DISK_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
