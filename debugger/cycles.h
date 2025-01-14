#ifndef __DEBUGGER_CYCLES_H__
#define __DEBUGGER_CYCLES_H__

#include <stdint.h>
#include "mems.h"

namespace debugger {

template <typename SIGNALS>
struct Cycles {
    Cycles() : _ring(new SIGNALS[MAX_CYCLES]) {}
    ~Cycles() { delete[] _ring; }

    int cycles() const { return _cycles; }
    SIGNALS *put() { return item(_put); }
    SIGNALS *get() { return item(_get); }
    const SIGNALS *put() const { return item(_put); }
    const SIGNALS *get() const { return item(_get); }

    void reset() {
        _cycles = _put = _get = 0;
        put()->clear();
    }

    void next() {
        _put = (_put + 1) % MAX_CYCLES;
        if (_cycles < MAX_CYCLES) {
            _cycles++;
        } else {
            _get = (_put + 1) % MAX_CYCLES;
        }
        put()->clear();
    }

    void discard(const SIGNALS *s) {
        const auto drop = diff(s, put());
        if (cycles() < drop) {
            _cycles = 0;
            _put = _get;
        } else {
            _cycles -= drop;
            _put = pos(s);
        }
        put()->clear();
    }

    uint_fast8_t pos(const SIGNALS *s) const { return s - _ring; }

    SIGNALS *prev(SIGNALS *s, uint_fast8_t backward = 1) {
        return item((pos(s) + MAX_CYCLES - backward) % MAX_CYCLES);
    }

    SIGNALS *next(SIGNALS *s, uint_fast8_t forward = 1) {
        return item((pos(s) + forward) % MAX_CYCLES);
    }

    const SIGNALS *prev(const SIGNALS *s, uint_fast8_t backward = 1) const {
        return item((pos(s) + MAX_CYCLES - backward) % MAX_CYCLES);
    }

    const SIGNALS *next(const SIGNALS *s, uint_fast8_t forward = 1) const {
        return item((pos(s) + forward) % MAX_CYCLES);
    }

    int diff(const SIGNALS *s, const SIGNALS *from) const {
        return ((s + MAX_CYCLES) - from) % MAX_CYCLES;
    }

    void print(const SIGNALS *end = nullptr) const {
        const auto g = get();
        const auto n = diff(end ? end : put(), g);
        for (auto i = 0; i < n; ++i) {
            next(g, i)->print();
        }
    }

    void disassemble(const Mems *mems) const {
        const auto g = get();
        for (auto i = 0; i < cycles();) {
            const auto s = next(g, i);
            if (s->fetch()) {
                const auto len = mems->disassemble(s->addr, 1) - s->addr;
                for (uint_fast8_t j = 1; j < len; ++j) {
                    const auto t = next(s, j);
                    if (t->addr < s->addr || t->addr >= s->addr + len)
                        t->print();
                }
                i += len;
            } else {
                s->print();
                i += 1;
            }
        }
    }

private:
    static constexpr auto MAX_CYCLES = 128;
    uint_fast8_t _cycles;
    uint_fast8_t _put;
    uint_fast8_t _get;
    SIGNALS *_ring;

    SIGNALS *item(uint_fast8_t pos) { return &_ring[pos]; }

    const SIGNALS *item(uint_fast8_t pos) const { return &_ring[pos]; }
};

}  // namespace debugger
#endif /* __CYCLES_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
