#ifndef __MEMS_CP1610_H__
#define __MEMS_CP1610_H__

#include "devs.h"
#include "mems.h"

namespace debugger {
namespace cp1600 {

struct MemsCp1610 final : DmaMemory {
    MemsCp1610(Devs *devs);

    uint32_t maxAddr() const override { return UINT16_MAX; }

    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

private:
    Devs *_devs;

    void isPrint(const uint8_t *data, char buf[2]) const override;
};

}  // namespace cp1600
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
