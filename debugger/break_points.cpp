#include "break_points.h"
#include "debugger.h"

namespace debugger {

bool BreakPoints::on(uint32_t addr) const {
    for (auto i = 0; i < _num; ++i) {
        if (_points[i] == addr)
            return true;
    }
    return false;
}

bool BreakPoints::set(uint32_t addr) {
    auto i = 0;
    while (i < _num) {
        if (_points[i] == addr)
            return true;
        ++i;
    }
    if (i < BREAK_POINTS) {
        _points[i] = addr;
        ++_num;
        return true;
    }
    return false;
}

bool BreakPoints::clear(uint8_t index) {
    if (--index >= _num)
        return false;
    for (auto i = index + 1; i < _num; ++i) {
        _points[index] = _points[i];
        ++index;
    }
    --_num;
    return true;
}

bool BreakPoints::print() const {
    for (uint8_t i = 0; i < _num; ++i) {
        cli.printDec(i + 1, -2);
        Debugger.target().disassemble(_points[i], 1);
    }
    return _num != 0;
}

void BreakPoints::saveInsts() {
    for (auto i = 0; i < _num; ++i) {
        const auto addr = _points[i];
        _insts[i] = Debugger.target().get_inst(addr);
        Debugger.target().setBreakPoint(addr);
    }
}

void BreakPoints::restoreInsts() {
    for (auto i = 0; i < _num; ++i) {
        Debugger.target().put_inst(_points[i], _insts[i]);
    }
}

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
