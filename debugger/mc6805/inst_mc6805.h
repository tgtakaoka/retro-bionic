#ifndef __INST_MC6805_H__
#define __INST_MC6805_H__

#include <stdint.h>

namespace debugger {
namespace mc6805 {

struct InstMc6805 {
    virtual ~InstMc6805() {}

    static constexpr uint8_t SWI = 0x83;

    bool valid(uint8_t inst) const;
    virtual bool isStop(uint8_t inst) const { return false; }

protected:
    virtual const uint8_t *table() const = 0;
};

}  // namespace mc6805
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
