#ifndef __SPLIT_IO_H__
#define __SPLIT_IO_H__

#include "device.h"

namespace debugger {
namespace z8000 {

struct MemsZ8000;

/**
 * The Z8002's split instruction/data control, on eight byte-wide I/O
 * ports. The Z8002 drives no segment lines, so the debugger synthesizes
 * three 64K segments (system, code, data) and routes a cycle to one by
 * its mode and status; this device is how the BIOS, in system mode,
 * turns that routing on for a split program and loads code and data into
 * the program's segments.
 *
 *   +0 CONTROL  write: bit 0 turns the routing on; read: its state
 *   +1 SRCSEG   the copy's source segment
 *   +2 SRCL     source offset, low byte
 *   +3 SRCH     high byte
 *   +4 DSTSEG   the copy's destination segment
 *   +5 DSTL     destination offset, low byte
 *   +6 DSTH     high byte
 *   +7 COUNT    write: copy that many bytes (0 means 64K) across the
 *               segments, in EXTMEM, while the CPU waits
 *
 * A Z8001's cycles carry a real segment and its BIOS copies with long
 * addresses, so it needs only CONTROL: on, a normal-mode program fetch
 * in segment 8 reads segment 0BH, the Olivetti M20's instruction space.
 */
struct SplitIo final : Device {
    explicit SplitIo(MemsZ8000 *mems) : _mems(mems) { _enabled = true; }

    const char *name() const override { return "SPLIT"; }
    const char *description() const override { return "I/D split"; }
    void print() const override;
    void reset() override;
    bool loop() override { return false; }
    bool isSelected(uint32_t addr) const override {
        return (addr & ~0xF) == _base_addr;
    }
    void setBaseAddr(uint32_t addr) override { _base_addr = addr & ~0xF; }
    uint32_t baseAddr() const override { return _base_addr; }
    uint16_t read(uint32_t addr) override { return peek(addr); }
    uint16_t peek(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) override;

private:
    MemsZ8000 *const _mems;
    uint32_t _base_addr = 0;
    uint8_t _srcSeg = 0;
    uint16_t _srcOff = 0;
    uint8_t _dstSeg = 0;
    uint16_t _dstOff = 0;
    uint16_t _count = 0;

    void copy(uint32_t count) const;
};

}  // namespace z8000
}  // namespace debugger
#endif /* __SPLIT_IO_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
