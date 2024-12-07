#ifndef __BREAK_POINTS_H__
#define __BREAK_POINTS_H__

#include <stdint.h>

namespace debugger {

struct BreakPoints {
    BreakPoints() : _num(0) {}

    bool on(uint32_t addr) const;
    bool set(uint32_t addr);
    bool clear(uint8_t index);
    bool print() const;
    void saveInsts();
    void restoreInsts();

private:
    static constexpr auto BREAK_POINTS = 4;
    uint8_t _num;
    uint32_t _points[BREAK_POINTS];
    uint16_t _insts[BREAK_POINTS];
};

}  // namespace debugger

#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
