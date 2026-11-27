#include "split_io.h"
#include "debugger.h"
#include "mems_z8000.h"

namespace debugger {
namespace z8000 {

namespace {
constexpr uint8_t CONTROL = 0;
constexpr uint8_t SRCSEG = 1;
constexpr uint8_t SRCL = 2;
constexpr uint8_t SRCH = 3;
constexpr uint8_t DSTSEG = 4;
constexpr uint8_t DSTL = 5;
constexpr uint8_t DSTH = 6;
constexpr uint8_t COUNTL = 7;
constexpr uint8_t COUNTH = 8;

constexpr uint8_t SPLIT_ON = 0x01;
}  // namespace

void SplitIo::print() const {
    cli.print("  routing ");
    cli.println(_mems->split() ? "split" : "merged");
}

void SplitIo::reset() {
    _mems->setSplit(false);
    _srcSeg = _dstSeg = 0;
    _srcOff = _dstOff = 0;
}

uint16_t SplitIo::peek(uint32_t addr) const {
    switch (addr - _base_addr) {
    case CONTROL:
        return _mems->split() ? SPLIT_ON : 0;
    case SRCSEG:
        return _srcSeg;
    case SRCL:
        return _srcOff & 0xFF;
    case SRCH:
        return _srcOff >> 8;
    case DSTSEG:
        return _dstSeg;
    case DSTL:
        return _dstOff & 0xFF;
    case DSTH:
        return _dstOff >> 8;
    case COUNTL:
        return _count & 0xFF;
    case COUNTH:
        return _count >> 8;
    default:
        return 0;
    }
}

void SplitIo::write(uint32_t addr, uint16_t data) {
    switch (addr - _base_addr) {
    case CONTROL:
        _mems->setSplit(data & SPLIT_ON);
        break;
    case SRCSEG:
        _srcSeg = data;
        break;
    case SRCL:
        _srcOff = (_srcOff & 0xFF00) | (data & 0xFF);
        break;
    case SRCH:
        _srcOff = (_srcOff & 0x00FF) | (data << 8);
        break;
    case DSTSEG:
        _dstSeg = data;
        break;
    case DSTL:
        _dstOff = (_dstOff & 0xFF00) | (data & 0xFF);
        break;
    case DSTH:
        _dstOff = (_dstOff & 0x00FF) | (data << 8);
        break;
    case COUNTL:
        _count = (_count & 0xFF00) | (data & 0xFF);
        break;
    case COUNTH:
        _count = (_count & 0x00FF) | (data << 8);
        copy(_count ? _count : 0x10000);
        break;
    }
}

// The bytes stay within each 64K segment: the offset wraps, the segment
// does not. The copy is written back before the next bus cycle.
void SplitIo::copy(uint32_t count) const {
    for (uint32_t i = 0; i < count; ++i) {
        const auto b = _mems->readByteSeg((_srcOff + i) & 0xFFFF, _srcSeg);
        _mems->writeByteSeg((_dstOff + i) & 0xFFFF, b, _dstSeg);
    }
    const auto dst = uint32_t(_dstSeg) * MemsZ8000::SEG_SIZE;
    const auto head = MemsZ8000::SEG_SIZE - _dstOff;
    _mems->writeBack(dst + _dstOff, count < head ? count : head);
    if (count > head)
        _mems->writeBack(dst, count - head);
}

}  // namespace z8000
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
