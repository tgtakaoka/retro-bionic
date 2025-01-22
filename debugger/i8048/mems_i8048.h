#ifndef __MEMS_I8048_H__
#define __MEMS_I8048_H__

#include "devs.h"
#include "mems.h"

namespace debugger {
namespace i8048 {

struct RegsI8048;

struct ProgI8048 final : DmaMemory {
    ProgI8048(RegsI8048 *regs, Mems *data);

    uint32_t maxAddr() const override { return 0xFFF; }
    uint16_t get(uint32_t addr, const char *space = nullptr) const override;
    void put(uint32_t addr, uint16_t data,
            const char *space = nullptr) const override;
private:
    RegsI8048 *const _regs;
    Mems *const _data;
};

struct DataI8048 final : DmaMemory {
    DataI8048(Devs *devs);

    uint32_t maxAddr() const override { return 0xFF; }
    uint16_t read_byte(uint32_t addr) const override {
        return DmaMemory::read_byte(OFFSET + addr);
    }
    void write_byte(uint32_t addr, uint16_t data) const override {
        DmaMemory::write_byte(OFFSET + addr, data);
    }
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

private:
    Devs *const _devs;

    static constexpr uint16_t OFFSET = 0x1000;
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
