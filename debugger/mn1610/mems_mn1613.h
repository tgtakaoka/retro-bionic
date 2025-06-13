#ifndef __MEMS_MN1613_H__
#define __MEMS_MN1613_H__

#include "mems.h"

namespace debugger {
namespace mn1613 {

struct MemsMn1613 final : ExtMemory {
    MemsMn1613();

    uint32_t maxAddr() const override { return _max_addr; }
    bool wordAccess() const override { return true; }

    uint16_t read(uint32_t addr) const override { return read_word(addr); }
    void write(uint32_t addr, uint16_t data) const override {
        write_word(addr, data);
    }

    void setMaxAddr(uint32_t max_addr) { _max_addr = max_addr; }

private:
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
