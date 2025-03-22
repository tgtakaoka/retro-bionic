#include "break_points.h"
#include "debugger.h"

namespace debugger {

bool BreakPoints::on(uint32_t addr) const {
    for (auto i = from(); i <= _num; ++i) {
        if (_points[i] == addr)
            return true;
    }
    return false;
}

bool BreakPoints::set(uint32_t addr) {
    uint_fast8_t i = 1;
    while (i <= _num) {
        if (_points[i] == addr)
            return true;
        ++i;
    }
    if (i <= BREAK_POINTS) {
        _points[i] = addr;
        ++_num;
        return true;
    }
    return false;
}

void BreakPoints::setTemp(uint32_t addr) {
    _points[0] = addr;
    _has_temp = true;
}

bool BreakPoints::clear(uint_fast8_t index) {
    if (index > _num)
        return false;
    for (auto i = index; i <= _num; ++i) {
        _points[i] = _points[i + 1];
    }
    --_num;
    return true;
}

bool BreakPoints::print() const {
    for (auto i = from(); i <= _num; ++i) {
        cli.printDec(i, -2);
        Debugger.target().disassemble(_points[i], 1);
    }
    return _num != 0;
}

void BreakPoints::saveInsts() {
    for (auto i = from(); i <= _num; ++i) {
        const auto addr = _points[i];
        _insts[i] = Debugger.target().getInst(addr);
        Debugger.target().setBreakPoint(addr);
    }
}

void BreakPoints::restoreInsts() {
    for (auto i = from(); i <= _num; ++i) {
        Debugger.target().putInst(_points[i], _insts[i]);
    }
    _has_temp = false;
}

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
