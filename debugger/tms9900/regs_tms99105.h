#ifndef __REGS_TMS99105_H__
#define __REGS_TMS99105_H__

#include "regs_tms9900.h"

namespace debugger {
namespace tms99105 {

struct PinsTms99105;

enum MacroMode : uint_fast8_t {
    MACRO_BASELINE = 0,
    MACRO_STANDARD = 1,
    MACRO_PROTOTYPING = 2,
    MACRO_TMS99110 = 3,
};

struct RegsTms99105 final : tms9900::RegsTms9900 {
    RegsTms99105(PinsTms99105 *pins, Mems *mems);

    const char *cpu() const override;
    const char *cpuName() const override { return cpu(); }

    void print() const override;
    void reset() override;
    void save() override;
    void restore() override;

    void helpRegisters() const override;
    const RegList *listRegisters(uint_fast8_t n) const override;
    bool setRegister(uint_fast8_t reg, uint32_t value) override;

    void breakPoint() override;
    MacroMode macroMode() const { return _macroMode; }
    void setCpuType(bool tms99110) { _tms99110 = tms99110; }

private:
    MacroMode _macroMode;
    bool _modeValid;
    bool _tms99110;

    template <typename MEMS>
    MEMS *mems() const {
        return static_cast<MEMS *>(_mems);
    }

    template <typename T, uint_fast8_t SIZE>
    inline auto length(const T (&array)[SIZE]) const {
        return SIZE;
    }
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
