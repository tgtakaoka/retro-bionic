#ifndef __MEMS_TMS99105_H__
#define __MEMS_TMS99105_H__

#include "mems_tms9900.h"

namespace debugger {
namespace tms99105 {

struct MemsTms99105 final : tms9900::MemsTms9900 {
    MemsTms99105(Devs *devs);

    void loadTms99110MacrostoreRom();
    uint16_t read_macro(uint32_t byte_addr) const;
    void write_macro(uint32_t byte_addr, uint16_t data) const;

private:
    static constexpr auto MACRO_OFFSET = UINT32_C(0x10000);
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
