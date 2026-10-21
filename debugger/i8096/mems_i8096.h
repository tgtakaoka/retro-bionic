#ifndef __MEMS_I8096_H__
#define __MEMS_I8096_H__

#include "devs.h"
#include "inst_i8096.h"
#include "mems.h"

namespace debugger {
namespace i8096 {

struct RegsI8096;

struct MemsI8096 : DmaMemory {
    MemsI8096(Devs *devs, RegsI8096 *regs, CpuType cpu);
    void setCpu(CpuType cpu);

    uint32_t maxAddr() const override { return UINT16_MAX; }

    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;
    // The word at the even |addr|: a device's range is aligned, so its
    // check covers both bytes.
    uint16_t readWord(uint32_t addr) const;
    void writeWord(uint32_t addr, uint16_t data) const;
    // The configuration byte the CPU reads at |addr|, CCB or CCB1, after
    // a reset.
    uint8_t ccb(uint16_t addr) const;

    uint16_t get_data(uint32_t addr) const override;
    void put_data(uint32_t addr, uint16_t data) const override;
    uint16_t get_prog(uint32_t addr) const override;
    void put_prog(uint32_t addr, uint16_t data) const override;

    static constexpr uint16_t CCB = 0x2018; // Chip Configuration Byte Address
    // The 80C196NP and 80296SA's second one
    static constexpr uint16_t CCB1 = 0x201A;

private:
    Devs *const _devs;
    RegsI8096 *const _regs;
    CpuType _cpu;
    // The register file ends here: data below it is the CPU's.
    uint16_t _regsEnd;

    // Data the CPU holds: the register file, and the 80C196NP and
    // 80296SA's peripheral SFRs at 1F00H-1FFFH.
    bool onChip(uint32_t addr) const {
        return addr < _regsEnd ||
               (_cpu >= CPU_80C196NP && addr >= 0x1F00 && addr < 0x2000);
    }
};

// The bytes as the CPU reads them, the devices included.
struct CpuMemory final : MatchMemory {
    explicit CpuMemory(const MemsI8096 *mems) : _mems(mems) {}
    uint16_t read_byte(uint32_t addr) const override {
        return _mems->read(addr);
    }

private:
    const MemsI8096 *const _mems;
};

}  // namespace i8096
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
