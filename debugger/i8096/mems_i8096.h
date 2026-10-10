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

    uint32_t maxAddr() const override { return UINT16_MAX; }

    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

    uint16_t get_data(uint32_t addr) const override;
    void put_data(uint32_t addr, uint16_t data) const override;
    uint16_t get_prog(uint32_t addr) const override;
    void put_prog(uint32_t addr, uint16_t data) const override;

    static constexpr uint16_t CCB = 0x2018; // Chip Configuration Byte Address

private:
    Devs *const _devs;
    RegsI8096 *const _regs;
    // The register file ends here: data below it is the CPU's.
    const uint16_t _regsEnd;
};

// The bytes as the CPU reads them, the CCB and the devices included.
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
