#ifndef __MEMS_TMS99105_H__
#define __MEMS_TMS99105_H__

#include "mems_tms9900.h"

namespace debugger {
namespace tms99105 {

struct MemsTms99105 : tms9900::MemsTms9900 {
    MemsTms99105(Devs *devs);

    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;
};

}  // namespace tms99105
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
