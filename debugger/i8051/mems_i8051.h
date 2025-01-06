#ifndef __MEMS_I8051_H__
#define __MEMS_I8051_H__

#include "devs.h"
#include "mems.h"

namespace debugger {
namespace i8051 {

struct RegsI8051;

struct ProgI8051 final : DmaMemory {
    ProgI8051(RegsI8051 *regs, Mems *data);

    uint32_t maxAddr() const override { return UINT16_MAX; }
    uint16_t get(uint32_t addr, const char *space = nullptr) const override;
    void put(uint32_t addr, uint16_t data,
            const char *space = nullptr) const override;

private:
    RegsI8051 *const _regs;
    Mems *const _data;
};

struct DataI8051 final : DmaMemory {
    DataI8051(Devs *devs);

    uint32_t maxAddr() const override { return UINT16_MAX; }
    uint16_t raw_read(uint32_t addr) const override {
        return DmaMemory::raw_read(OFFSET + addr);
    }
    void raw_write(uint32_t addr, uint16_t data) const override {
        DmaMemory::raw_write(OFFSET + addr, data);
    }
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

private:
    Devs *const _devs;

    static constexpr uint32_t OFFSET = 0x10000U;
};

}  // namespace i8051
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
