#ifndef __MEMS_MN1613_H__
#define __MEMS_MN1613_H__

#include "devs.h"
#include "mems.h"

namespace debugger {
namespace mn1613 {

struct MemsMn1613 final : ExtMemory {
    MemsMn1613(Devs *devs);

    uint32_t maxAddr() const override { return _max_addr; }
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;
    uint16_t get(uint32_t addr, const char *space = nullptr) const override;
    void put(uint32_t addr, uint16_t data,
            const char *space = nullptr) const override;
    uint16_t get_inst(uint32_t addr) const override;
    void put_inst(uint32_t addr, uint16_t data) const override;

    void setMaxAddr(uint32_t max_addr) { _max_addr = max_addr; }

private:
    Devs *const _devs;
    uint32_t _max_addr;

    void isPrint(const uint8_t *data, char buf[2]) const override;
};

}  // namespace mn1613
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
