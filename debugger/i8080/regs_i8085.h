#ifndef __REGS_I8085_H__
#define __REGS_I8085_H__

#include "regs_i8080.h"

namespace debugger {
namespace i8085 {

struct RegsI8085 final : i8080::RegsI8080 {
    RegsI8085(i8080::PinsI8080Base *pins);

    const char *cpu() const override;

    void saveContext(uint16_t pc);

protected:
    bool ie() const override;
};

}  // namespace i8085
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
