#ifndef __BREAK_POINTS_H__
#define __BREAK_POINTS_H__

#include <stdint.h>

namespace debugger {

struct BreakPoints {
    BreakPoints() : _num(0), _has_temp(false) {}

    bool on(uint32_t addr) const;
    bool set(uint32_t addr);
    void setTemp(uint32_t addr);
    bool clear(uint_fast8_t index);
    bool print() const;
    void saveInsts();
    void restoreInsts();

private:
    static constexpr auto BREAK_POINTS = 4;
    uint_fast8_t _num;
    bool _has_temp;
    uint32_t _points[BREAK_POINTS + 1];
    uint16_t _insts[BREAK_POINTS + 1];

    uint_fast8_t from() const { return _has_temp ? 0 : 1; }
};

}  // namespace debugger

#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
