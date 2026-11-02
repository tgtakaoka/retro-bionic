#ifndef __QS_DECODE_I8086_H__
#define __QS_DECODE_I8086_H__

#include <stdint.h>

namespace debugger {
namespace i8086 {

/**
 * The instructions an 8086 executed, from its bus cycles and its queue
 * status sampled once per CLK. Fetched bytes form streams, a new one where
 * the fetch address jumps or, with QS1, after the queue empties (QS 10).
 * Each byte QS0 shows taken comes, in order, from the current stream; with
 * QS1 the next stream takes over at the flush that began it, so a byte
 * taken soon after a fetch, as a V30 does, still counts against its own
 * stream. Without QS1, or for a stream no flush began, the stream whose
 * first fetch was last before the byte takes over. Instructions decode
 * one after another from a stream's start, as far as its bytes were taken;
 * other cycles go in between, in time order. The first stream, begun
 * before the samples, is left out, unless it is the only one: then, with
 * QS1, the stop places it (see below).
 *
 * Source |S| tells, for cycles 0..cycles()-1 oldest first and samples
 * 0..samples()-1 oldest first:
 *   int cycles(), uint32_t cycleTime(int i),
 *   bool fetch(int i, uint32_t &first, uint32_t &next): a code fetch, its
 *     first byte and the address after it;
 *   int samples(), uint32_t sampleTime(int k), uint8_t qs(int k): QS1:QS0;
 *   uint32_t insn(uint32_t addr): shows the instruction, returns the next;
 *   uint8_t length(uint32_t addr): the instruction's length, quietly;
 *   void cycle(int i): shows a cycle that isn't a fetch.
 * Times are counts that may wrap.
 *
 * An instruction at |stopAt| started last is left out: its first byte was
 * taken just as the CPU stopped, before it ran. The stop's flush begins no
 * stream; bytes taken after it are the stop's own.
 *
 * A stream begun before the samples, alone in them, as on an 8-bit bus in
 * a long run of code, ends where the CPU stopped: its last byte taken is
 * just before |stopAt|, or, a QS 01, at it. The one whose QS 01s fall where
 * the instructions' lengths say, up to |stopAt|, places the stream.
 */
template <typename S, int MAX_STREAMS>
struct QsDecoder {
    QsDecoder(S &src, bool pairs, uint32_t stopAt = UINT32_MAX)
        : _src(src), _pairs(pairs), _stopAt(stopAt) {}

    /**
     * Shows the last |limit| instructions, and the cycles from the first of
     * them on. False when the samples hold no stream but the first.
     */
    bool decode(uint32_t limit = UINT32_MAX) {
        if (!findStreams() && !placeFirst())
            return false;
        // A quiet pass finds whether the last start is at |stopAt|: the
        // bytes taken, not the instructions' lengths, place it.
        const auto hide = run(FIND, -1, 0);
        int skip = 0;
        if (limit != UINT32_MAX) {
            restart();
            const uint32_t starts = run(COUNT, hide, 0);
            skip = starts > limit ? starts - limit : 0;
        }
        restart();
        run(SHOW, hide, skip);
        return true;
    }

    /** Taken bytes where QS 01 and the decode agree on a start, or not. */
    uint32_t good = 0;
    uint32_t bad = 0;

private:
    S &_src;
    const bool _pairs;
    const uint32_t _stopAt;
    int _streams = 0;
    // Each stream's first address, its first fetch's time, the flush that
    // began it (-1 for none), and its bytes taken so far.
    uint32_t _start[MAX_STREAMS];
    uint32_t _since[MAX_STREAMS];
    int _flushedAt[MAX_STREAMS];
    uint32_t _taken[MAX_STREAMS];
    // The first stream placed by the stop, and its first instruction.
    bool _placed = false;
    uint32_t _firstInsn = 0;

    static bool before(uint32_t a, uint32_t b) { return int32_t(a - b) < 0; }

    enum Mode : uint8_t { FIND, COUNT, SHOW };

    void restart() {
        for (int s = 0; s < _streams; ++s)
            _taken[s] = 0;
    }

    // Goes through the samples, leaving out the instruction at |_stopAt|
    // the |hide|th byte taken starts. FIND returns how many bytes were
    // taken before the last start, if at |_stopAt|, else -1; COUNT the
    // instructions there are to show; SHOW shows all but the first |skip|
    // of them, and the cycles from the first shown on.
    int run(Mode mode, int hide, int skip) {
        const bool show = mode == SHOW;
        const auto n = _src.cycles();
        int cur = 0;        // the stream taken from now
        int cycle = 0;      // the next cycle to show, if not a fetch
        uint32_t next = _placed ? _firstInsn : 0;  // the next instruction
        bool lost = false;  // past a flush that began no stream
        uint32_t fetched;
        int taken = 0;         // bytes taken so far
        int lastStart = -1;    // the last start's byte
        bool atStop = false;   // the last start is at |_stopAt|
        bool stopped = false;  // past the instruction the CPU stopped at
        int starts = 0;        // instructions found
        bool showing = skip == 0;
        for (int k = 0; k < _src.samples(); ++k) {
            const auto t = _src.sampleTime(k);
            const auto qs = _src.qs(k);
            bool began = false;
            while (cur + 1 < _streams &&
                    (_flushedAt[cur + 1] < 0 ? before(_since[cur + 1], t)
                                             : _flushedAt[cur + 1] == k)) {
                ++cur;
                next = _start[cur];
                began = true;
                lost = false;
            }
            if (_pairs && qs == 2 && !began)
                lost = true;
            const auto known = cur > 0 || _placed;
            while (show && cycle < n && !before(t, _src.cycleTime(cycle))) {
                const auto i = cycle++;
                if (known && showing && !_src.fetch(i, fetched, fetched))
                    _src.cycle(i);
            }
            if (!known || lost || (qs & 1) == 0)
                continue;
            const auto a = (_start[cur] + _taken[cur]++) & 0xFFFFF;
            if (qs == 1) {
                lastStart = taken;
                atStop = a == _stopAt;
            }
            if (mode == FIND) {
                ++taken;
                continue;
            }
            if (show) {
                if ((qs == 1) == (a == next)) {
                    ++good;
                } else {
                    ++bad;
                }
            }
            if (taken == hide)
                stopped = true;
            if (a == next && !stopped) {
                if (show && starts >= skip) {
                    showing = true;
                    next = _src.insn(a);
                } else {
                    next = (a + _src.length(a)) & 0xFFFFF;
                }
                ++starts;
            }
            ++taken;
        }
        while (show && showing && cycle < n) {
            const auto i = cycle++;
            if (!_src.fetch(i, fetched, fetched))
                _src.cycle(i);
        }
        if (mode == FIND)
            return atStop ? lastStart : -1;
        return starts;
    }

    // How many QS 01s of the first stream, if it starts at |base|, fall
    // where the lengths don't say; the first's address in |first|.
    int misplaced(uint32_t base, uint32_t &first, int &starts) {
        int j = 0, bad = 0;
        uint32_t prev = UINT32_MAX;
        starts = 0;
        for (int k = 0; k < _src.samples(); ++k) {
            const auto qs = _src.qs(k);
            if (qs == 2)
                break;
            if ((qs & 1) == 0)
                continue;
            const auto a = (base + j++) & 0xFFFFF;
            if (qs != 1)
                continue;
            if (prev == UINT32_MAX) {
                first = a;
            } else if (prev + _src.length(prev) != a) {
                ++bad;
            }
            prev = a;
            ++starts;
        }
        if (prev != UINT32_MAX && prev != _stopAt &&
                prev + _src.length(prev) != _stopAt)
            ++bad;
        return bad;
    }

    bool placeFirst() {
        if (!_pairs || _streams != 1 || _stopAt == UINT32_MAX)
            return false;
        int taken = 0;
        uint8_t last = 0;
        for (int k = 0; k < _src.samples(); ++k) {
            const auto qs = _src.qs(k);
            if (qs == 2)
                break;
            if (qs & 1) {
                ++taken;
                last = qs;
            }
        }
        // Too few starts can fall right by chance.
        constexpr int MIN_STARTS = 3;
        for (int at = 0; at < (last == 1 ? 2 : 1); ++at) {
            const auto base = (_stopAt - taken + at) & 0xFFFFF;
            uint32_t first;
            int starts;
            if (misplaced(base, first, starts) == 0 && starts >= MIN_STARTS) {
                _start[0] = base;
                _firstInsn = first;
                _placed = true;
                return true;
            }
        }
        return false;
    }

    bool findStreams() {
        uint32_t expect = UINT32_MAX;
        int k = 0;  // the next sample to look for a flush in
        for (int i = 0; i < _src.cycles() && _streams < MAX_STREAMS; ++i) {
            uint32_t first, next;
            if (!_src.fetch(i, first, next))
                continue;
            const auto t = _src.cycleTime(i);
            int flushed = -1;
            for (; k < _src.samples() && before(_src.sampleTime(k), t); ++k) {
                if (_pairs && _src.qs(k) == 2)
                    flushed = k;
            }
            if (first != expect || flushed >= 0) {
                _start[_streams] = first;
                _since[_streams] = t;
                _flushedAt[_streams] = flushed;
                _taken[_streams] = 0;
                ++_streams;
            }
            expect = next;
        }
        return _streams >= 2;
    }
};

}  // namespace i8086
}  // namespace debugger
#endif /* __QS_DECODE_I8086_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
