#ifndef __PINS_I8086_BASE_H__
#define __PINS_I8086_BASE_H__

#include "cycles.h"
#include "npx_i8086.h"
#include "pins.h"
#include "qs_i8086.h"
#include "signals_i8086.h"

namespace debugger {
namespace i8086 {

// What every 8086-family board shares: the debugger's park and traps,
// injected sequences, and the backtrace from the queue status. A board
// brings its bus loop and clock.
struct PinsI8086Base : Pins {
    bool bus8() const { return _bus8; }
    bool vSeries() const { return _v_series; }
    // Whether the D8087 board has its 8087 seated, as reset found.
    bool npx() const { return _npx; }
    // An 80C186, rather than an 8086, 8088, V20 or V30.
    bool i80186() const { return _qs_board == QsTracker::N80C186; }
    // The address bus: 20 bits, or the 80286's 24.
    uint32_t addrMask() const { return _addrMask; }
    uint32_t physical(uint16_t seg, uint16_t off) const {
        return ((uint32_t(seg) << 4) + off) & _addrMask;
    }

    bool step(bool show) override;
    void run() override;

    void printCycles() override;
    void setBreakInst(uint32_t addr) const override;

    // Bytes the CPU reads from, or writes to, the debugger instead of
    // memory while an injected sequence runs.
    struct Overlay {
        Overlay(uint32_t mask) : _mask(mask) {}
        void clear() { _num = 0; }
        void put(uint32_t addr, uint8_t data);
        // Whether |addr| has a byte; it goes to |data|.
        bool get(uint32_t addr, uint8_t &data) const;
        uint16_t wordAt(uint32_t addr) const;
        // Whether the CPU read every byte put since clear().
        bool allRead() const;
        void markRead(uint32_t addr);

    private:
        static constexpr uint_fast8_t MAX = 40;
        const uint32_t _mask;
        uint32_t _addr[MAX];
        uint8_t _data[MAX];
        bool _read[MAX];
        uint_fast8_t _num = 0;
        int find(uint32_t addr) const;
    };
    Overlay &overlay() { return _overlay; }

    // Runs |inst| on the CPU parked at |org|, its reads answered from
    // |inst| by address, then from the overlay, then from memory; every
    // write goes to the overlay. Parks at the read of |exit| once |inst|
    // was read through, or, with EXIT_OVERLAY, once the CPU read every
    // byte the overlay held before the run. |org| becomes the park.
    static constexpr uint32_t EXIT_OVERLAY = UINT32_MAX;
    void execInst(const uint8_t *inst, uint_fast8_t len, uint32_t &org,
            uint32_t exit);
    // Cycles from |base| on, while set, take |image|'s bytes, as the
    // CPU's dummy read for the 8087 does, and the 8087's writes leave
    // theirs there, not in memory; the bytes each way are counted.
    void setNpxImage(NpxImage *image, uint32_t base) {
        _npxImage = image;
        _npxBase = base;
        _npxRead = _npxWritten = 0;
    }
    uint_fast8_t npxRead() const { return _npxRead; }
    uint_fast8_t npxWritten() const { return _npxWritten; }
    virtual bool npxBusy() const { return false; }

protected:
    PinsI8086Base(bool fpu, QsTracker::Board board = QsTracker::P8086,
            uint32_t addrMask = 0xFFFFF)
        : _fpu(fpu),
          _qs_board(board),
          _qs(board),
          _addrMask(addrMask),
          _overlay(addrMask) {}

    const bool _fpu;
    const QsTracker::Board _qs_board;
    QsTracker _qs;
    const uint32_t _addrMask;
    bool _bus8 = false;
    bool _v_series = false;
    bool _npx = false;
    // When each cycle in the ring showed its status, in CPU cycles: where
    // its queue status samples are.
    uint32_t _seenAt[Cycles::MAX_CYCLES];
    // DIAG: K 94 picks the backtrace, by QS pairs, by QS0 alone, or raw.
    enum QsMode : uint8_t { QS_PAIR, QS_ZERO, QS_RAW };
    QsMode _qsMode = QS_PAIR;
    // When the last run or step took over from the restore; the fetch it
    // resumed with was seen just before.
    uint32_t _resumed = 0;

    // Waits for the next bus cycle and holds it, its address and status
    // read. Null when the CPU shows none: halted.
    virtual Signals *prepareCycle() = 0;
    // Serves |s| and lets it end.
    virtual Signals *completeCycle(Signals *s) = 0;
    virtual void assertNmi() = 0;
    virtual void negateNmi() = 0;
    // Stops the queue status ring at a known time.
    virtual void freezeQs() = 0;
    // CPU cycles per bus clock: the ring's sample spacing.
    virtual uint32_t clockCycles() const = 0;

    Signals *resumeCycle(uint32_t addr);
    void retrieveData(Signals *s);
    void storeData(Signals *s);
    void dumpQueue();
    bool printExecuted(bool stats);

private:
    Overlay _overlay;
    // The cycle a restore left the CPU in, for step() and run() to go on
    // with.
    Signals *_next = nullptr;
    // A trap's vector, and the frame it pushed, lowest byte first.
    uint8_t _vector = 0;
    uint32_t _frameAt[6];
    uint8_t _frame[6];
    uint_fast8_t _frameBytes = 0;
    NpxImage *_npxImage = nullptr;
    uint32_t _npxBase = 0;
    uint_fast8_t _npxRead = 0;
    uint_fast8_t _npxWritten = 0;
    bool npxAt(uint32_t addr) const {
        return _npxImage && ((addr - _npxBase) & 0xFFFFF) < NpxImage::SIZE;
    }
    void injectNpx(Signals *s);
    void captureNpx(const Signals *s);

    const SignalsImpl *findBacktraceStart() override;
    void printBacktrace() override;

    // A read's bytes from the window, the overlay or memory.
    void injectRead(
            Signals *s, const uint8_t *inst, uint_fast8_t len, uint32_t org);
    void recordFrame(const Signals *s);
    enum Mode : uint8_t {
        STEP,
        RUN,
    };
    bool untilTrap(Signals *s, Mode mode);
    bool parkInTrap(Signals *s, bool interrupted);
    friend struct RegsI8086;
    void restoreNext(Signals *s) { _next = s; }
};

}  // namespace i8086
}  // namespace debugger
#endif /* __PINS_I8086_BASE_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
