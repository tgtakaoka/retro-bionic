#ifndef __MEMS_I8048_H__
#define __MEMS_I8048_H__

#include "devs.h"
#include "mems.h"

namespace debugger {
namespace i8048 {

struct RegsI8048;

/**
   Program Memory:       000-FFF
   Resident Data Memory: 00-FF
   External Data Memory: 00-FF (100-1FF to dump)
 */

struct ProgI8048 final : DmaMemory {
    ProgI8048(Mems *data);

    uint32_t maxAddr() const override { return 0xFFF; }
    uint32_t maxData() const override { return 0x1FF; }
    uint16_t get_data(uint32_t addr) const override {
        return _data->get_data(addr);
    }
    void put_data(uint32_t addr, uint16_t data) const override {
        _data->put_data(addr, data);
    }

private:
    Mems *const _data;
};

struct DataI8048 final : DmaMemory {
    DataI8048(RegsI8048 *regs, Devs *devs);

    uint32_t maxAddr() const override { return 0xFF; }
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
    RegsI8048 *const _regs;
    Devs *const _devs;

    static constexpr auto OFFSET = UINT16_C(0x1000);
};

}  // namespace i8048
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
