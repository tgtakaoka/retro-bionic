#ifndef __MEMS_I8051_H__
#define __MEMS_I8051_H__

#include "devs.h"
#include "mems.h"

namespace debugger {
namespace i8051 {

struct RegsI8051;

/**
   Program Memory:       0000-FFFF
   Resident Data Memory: 00-FF
   External Data Memory: 0000-FFFF (00-FF can be read from 10000-100FF)
 */

struct ProgI8051 final : DmaMemory {
    ProgI8051(Mems *data);

    uint32_t maxAddr() const override { return UINT16_MAX; }
    uint32_t maxData() const override { return UINT16_MAX + 0xFF; }
    uint16_t get_data(uint32_t addr) const override {
        return _data->get_data(addr);
    }
    void put_data(uint32_t addr, uint16_t data) const override {
        _data->put_data(addr, data);
    }

private:
    Mems *const _data;
};

struct DataI8051 final : DmaMemory {
    DataI8051(RegsI8051 *regs, Devs *devs);

    uint32_t maxAddr() const override { return UINT16_MAX; }
    uint16_t read_byte(uint32_t addr) const override {
        return DmaMemory::read_byte(OFFSET + addr);
    }
    void write_byte(uint32_t addr, uint16_t data) const override {
        DmaMemory::write_byte(OFFSET + addr, data);
    }
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;
    uint16_t get_data(uint32_t addr) const override;
    void put_data(uint32_t addr, uint16_t data) const override;

private:
    RegsI8051 *const _regs;
    Devs *const _devs;

    static constexpr auto OFFSET = UINT32_C(0x10000);
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
