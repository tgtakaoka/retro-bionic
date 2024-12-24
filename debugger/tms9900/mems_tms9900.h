#ifndef __MEMS_TMS9900_H__
#define __MEMS_TMS9900_H__

#include "devs.h"
#include "mems.h"

namespace debugger {
namespace tms9900 {

struct MemsTms9900 : DmaMemory {
    MemsTms9900(uint8_t addr_bits, Devs *devs);

    uint32_t maxAddr() const override { return _max_addr; }
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;
    uint16_t get(uint32_t addr, const char *space = nullptr) const override;
    void put(uint32_t addr, uint16_t data,
            const char *space = nullptr) const override;
    uint16_t get_inst(uint32_t addr) const override { return raw_read16(addr); }
    void put_inst(uint32_t addr, uint16_t data) const override {
        raw_write16(addr, data);
    }

    uint16_t vec_nmi() const { return maxAddr() - 3; }

protected:
    const uint16_t _max_addr;
    Devs *_devs;
};

}  // namespace tms9900
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
