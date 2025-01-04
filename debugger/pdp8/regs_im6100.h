#ifndef __REGS_IM6100_H__
#define __REGS_IM6100_H__

#include "char_buffer.h"
#include "regs_pdp8.h"

namespace debugger {
namespace im6100 {

struct PinsIm6100;

struct RegsIm6100 final : pdp8::RegsPdp8 {
    RegsIm6100(PinsIm6100 *pins);

    const char *cpu() const override;

    void print() const override;
    void save() override;
    void restore() override;
    void breakPoint() override;

    void helpRegisters() const override;

private:
    mutable CharBuffer _buffer;
};

}  // namespace im6100
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
