#include "pins_i8096.h"
#include "debugger.h"
#include "devs_i8096.h"
#include "inst_i8096.h"
#include "mems_i8096.h"
#include "regs_i8096.h"
#include "watchdog.h"

namespace debugger {
namespace i8096 {

using Signals = SignalsI8096;

// clang-format off
/**
 * P8095BH and N8097BH bus cycle, the default PinsI8096 clocks
 *          __    __    __    __    __    __    __    __    __    __    __    __    __
 * XTAL1 __|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |
 *        __                                  __                                 __
 *   ALE /  \________________________________/  \_______________________________/  \____
 *       _________                  ___________________________________________________
 *   #RD          \________________/                                         
 *       _____________________________________________                  _______________
 *   #WR                                              \________________/     
 *       ___ ____________________________ ______ ____________________________ ______ __
 *    AD ___X_addr_______________________X_addr_X_________data_______________X_addr_X__
 *
 * The CCB selects ALE, as the 80C196NP and 80296SA always have it: a
 * cycle starts where ALE falls and ends where #RD or #WR rises.
 *
 * The CCB selects the 16-bit bus: a read is a word's, the even byte on
 * AD0-AD7; a write moves the bytes A0 and #BHE pick.
 */

// clang-format on

namespace {

// delayNanoseconds() takes about 25 ns however short; the comments give the
// phases measured.
constexpr auto xtal1_lo_ns = 20;  // 40 ns
constexpr auto xtal1_hi_ns = 20;  // 45 ns
// The bus signals move 40-60 ns after the rising XTAL1 edge making them.
constexpr auto settle_ns = 100;
// ALE sampled right after a rising edge, 60 ns or more after the one
// before.
constexpr auto ale_lo_ns = 25;  // 40 ns
// With getAddr(), XTAL1's minimum low time of 30 ns after the edge that
// ends ALE.
constexpr auto addr_lo_ns = 5;  // 65 ns
// Bus cycles between polls of the halt switch in a run: its yield()
// services the USB while XTAL1 is stopped, which would otherwise stretch
// every cycle. The devices ration their own polling.
constexpr auto halt_poll_cycles = 16;

const uint8_t PINS_OPENDRAIN[] = {
        PIN_RESET,
};

const uint8_t PINS_HIGH[] = {
        PIN_READY,
        PIN_RXD,
};

const uint8_t PINS_LOW[] = {
        PIN_EXTINT,
        PIN_XTAL1,
};

const uint8_t PINS_INPUT[] = {
        PIN_ALE,
        PIN_RD,
        PIN_WR,
        PIN_BHE,
        PIN_AD0,
        PIN_AD1,
        PIN_AD2,
        PIN_AD3,
        PIN_AD4,
        PIN_AD5,
        PIN_AD6,
        PIN_AD7,
        PIN_AD8,
        PIN_AD9,
        PIN_AD10,
        PIN_AD11,
        PIN_AD12,
        PIN_AD13,
        PIN_AD14,
        PIN_AD15,
        PIN_HSO0,
        PIN_HSO1,
        PIN_HSO2,
        PIN_HSO3,
        PIN_HSI0,
        PIN_HSI1,
        PIN_HSI2,
        PIN_HSI3,
        PIN_TXD,
};

inline void xtal1_lo() {
    digitalWriteFast(PIN_XTAL1, LOW);
}

inline void xtal1_hi() {
    digitalWriteFast(PIN_XTAL1, HIGH);
}

inline auto ale_asserted() {
    return digitalReadFast(PIN_ALE) != LOW;
}

constexpr uint32_t settle_cycles = F_CPU / 1000000 * settle_ns / 1000;

void xtal1_cycle_lo() {
    xtal1_hi();
    delayNanoseconds(xtal1_hi_ns);
    xtal1_lo();
}

void xtal1_cycle() {
    xtal1_cycle_lo();
    delayNanoseconds(xtal1_lo_ns);
}

void negate_reset() {
    pinMode(PIN_RESET, INPUT_PULLUP);
}

}  // namespace

PinsI8096::PinsI8096(CpuType cpu) : _board(cpu), _cpu(cpu) {
    _devs = new DevsI8096();
    auto regs = new RegsI8096(this);
    _regs = regs;
    auto mems = new MemsI8096(_devs, regs, cpu);
    _mems = mems;
}

void PinsI8096::resetPins() {
    pinsMode(PINS_OPENDRAIN, sizeof(PINS_OPENDRAIN), OUTPUT_OPENDRAIN, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
    for (auto i = 0; i < 10 * 4; i++)
        xtal1_cycle();
    Cycles::reset();
    _ringFirst = MatchWalker::NO_STOP;
    negate_reset();
    _idle = false;
    _held = false;
    _halt = Halt();
    auto fetch = startAfterReset(prepareCycle());
    if (fetch->addr != InstI8096::ORG_RESET) {
        cli.print("?reset fetch at ");
        cli.printlnHex(fetch->addr, 4);
    }
    hold(fetch, fetch->addr);
    _cpu = probeCpu();
    mems<MemsI8096>()->setCpu(_cpu);
    _regs->reset();
    _regs->save();
}

// |ccb| reads the CCB at 2018H, before the CCR selects the 16-bit bus:
// AD0-AD7 carry it in every bus mode. Returns the first fetch.
Signals *PinsI8096::startAfterReset(Signals *ccb) {
    ccb->addr = MemsI8096::CCB;  // AD held nothing while ALE was high
    completeCycle(
            ccb->inject(mems<MemsI8096>()->ccb(MemsI8096::CCB)), true);
    auto fetch = prepareCycle();
    // The S80C196KB and KC hold ALE high about eight periods before the
    // address is on AD.
    if (_board == CPU_80C196KC)
        fetch->addr = InstI8096::ORG_RESET;
    // The 80C196NP and 80296SA read CCB1 next, on the bus CCB0 set up.
    if (fetch->addr == MemsI8096::CCB1) {
        completeCycle(
                fetch->inject(mems<MemsI8096>()->ccb(MemsI8096::CCB1)),
                true);
        fetch = prepareCycle();
    }
    return fetch;
}

// ATTENTION: a program's own read of the CCB is taken for its RST.
bool PinsI8096::programReset(Signals *s) {
    if (s->addr != MemsI8096::CCB || !s->read())
        return false;
    if (_cpu < CPU_80C196NP) {
        completeCycle(s->inject(mems<MemsI8096>()->ccb(MemsI8096::CCB)), true);
        return true;
    }
    const auto fetch = startAfterReset(s);
    hold(fetch, fetch->addr);
    regs<RegsI8096>()->selectBus16();
    return true;
}

// The S80C196KC's register RAM reaches 1FFH, the S80C196KB's 0FFH: a
// store to 0100H shows on the bus only on the KB, before the one to 5678H
// that ends the capture. DPTS, the PTS's, is an unimplemented opcode on
// the S80296SA: its trap pushes the PC before that store. Its vector at
// 2012H points at the closing SJMP, so the sequence's last byte is read
// and the CPU parks either way.
CpuType PinsI8096::probeCpu() {
    constexpr uint16_t ext = 0x5678;
    uint8_t byte;
    if (_board == CPU_80C196KC) {
        const uint8_t STB_0100[] = {
                0xC7, 0x01, 0x00, 0x01, 0,        // STB ZERO, 0100H[0]
                0xC7, 0x01, lo(ext), hi(ext), 0,  // STB ZERO, 5678H[0]
                SJMP(-12),                        // SJMP $+2-12
        };
        const auto first = execute(_park, STB_0100, sizeof(STB_0100), &byte,
                1, EXIT_PARK, true);
        return first == 0x0100 ? CPU_80C196KB : CPU_80C196KC;
    }
    if (_board != CPU_80296SA)
        return _board;
    constexpr uint16_t VEC_UNIMPLEMENTED = 0x2012;
    const uint8_t DPTS[] = {
            0xA1, lo(ext), hi(ext), 0x18,    // LD SP, #5678H
            0xEC,                            // DPTS
            0xC7, 0x01, lo(ext), hi(ext), 0,  // STB ZERO, 5678H[0]
            SJMP(-12),                       // SJMP $+2-12
    };
    const uint16_t sjmp = _park + sizeof(DPTS) - 2;
    const uint8_t vec[] = {lo(sjmp), hi(sjmp)};
    const auto first = execute(_park, DPTS, sizeof(DPTS), &byte, 1, EXIT_PARK,
            true, VEC_UNIMPLEMENTED, vec, sizeof(vec));
    return first != ext ? CPU_80296SA : CPU_80C196NP;
}

// #RD or #WR active within |settle_ns|.
bool PinsI8096::awaitControl(Signals *s) const {
    const uint32_t start = ARM_DWT_CYCCNT;
    do {
        if (getControl(s))
            return true;
    } while (ARM_DWT_CYCCNT - start < settle_cycles);
    return false;
}


Signals *PinsI8096::prepareCycle() {
    auto s = _idle ? &_idleSignals : Signals::put();
    if (_held) {
        _held = false;
        *s = _heldSignals;
        return s;
    }
    return startCycle(s);
}

Signals *PinsI8096::startCycle(Signals *s) {
    noInterrupts();
    // assert_debug();
    // completeCycle() leaves ALE low and unrisen. Sampled right after a
    // rising XTAL1 edge, ALE shows how the edge before left it; seen high,
    // the edge just made ends the pulse. The address is settled on AD
    // before the next edge, and a write's data and #RD or #WR follow only
    // after it.
    for (auto n = 0;; ++n) {
        if (n >= no_bus_cycles)
            return noBusCycle(s);
        delayNanoseconds(ale_lo_ns);
        xtal1_hi();
        if (ale_asserted())
            break;
        xtal1_lo();
    }
    xtal1_lo();
    delayNanoseconds(addr_lo_ns);
    s->getAddr();
    // #RD or #WR falls one clock after ALE, 40-60 ns after this edge:
    // seen in its low phase, the data phase starts at the next edge,
    // never shifted by one, and the CPU no longer drives AD when
    // completeCycle() does.
    xtal1_hi();
    delayNanoseconds(xtal1_hi_ns);
    xtal1_lo();
    const auto active = awaitControl(s);
    // negate_debug();
    // assert_debug();
    for (auto n = 0; !active && !getControl(s); ++n) {
        if (n >= no_bus_cycles)
            return noBusCycle(s);
        xtal1_cycle_lo();
    }
    // negate_debug();
    interrupts();
    return s;
}

// Gives up on a cycle rather than wedge the board, leaving the halt
// switch to stop the run.
Signals *PinsI8096::noBusCycle(Signals *s) {
    interrupts();
    // A wait this long ticks too seldom to keep the watchdog off; the
    // halt switch still stops the run.
    Watchdog::feed();
    cli.println("?halt: no bus cycle");
    s->noCycle();
    return s;
}

// Keeps the CPU waiting in the read |s|, its next fetch at |park|; the
// next prepareCycle() takes it up.
void PinsI8096::hold(const Signals *s, uint16_t park) {
    _heldSignals = *s;
    _heldSignals.clear();
    _held = true;
    _park = park;
}

// A 16-bit read is always a word's, at an even address: the CPU discards
// the byte it doesn't need. An odd byte rides AD0-AD7 too, for an 8-bit
// bus an 80C196NP or 80296SA program selects with a BUSCONx.
// ATTENTION: a write there puts an odd byte on AD0-AD7, taken from
// AD8-AD15 as on the 16-bit bus.
uint16_t PinsI8096::readBus(const Signals *s) const {
    if (s->addr & 1) {
        const auto odd = _mems->read(s->addr);
        return uint16(odd, odd);
    }
    return mems<MemsI8096>()->readWord(s->addr);
}

void PinsI8096::writeBus(const Signals *s) const {
    if (s->bytes() == 2) {
        mems<MemsI8096>()->writeWord(s->addr, s->data);
    } else {
        _mems->write(s->addr, s->byteAt(s->addr));
    }
}

// |low|: drive a read's data on AD0-AD7 only.
Signals *PinsI8096::completeCycle(Signals *s, bool low) {
    xtal1_hi();
    if (s->read()) {
        if (s->readMemory()) {
            s->data = readBus(s);
        } else {
            delayNanoseconds(xtal1_hi_ns);
        }
        xtal1_lo();
        // assert_debug();
        if (low) {
            s->outLow();
        } else {
            s->outData();
        }
        // negate_debug();
        xtal1_hi();
        s->inputMode();
    } else if (s->write()) {
        // A write's data reaches AD 70 ns after the rising edge that
        // follows ALE's fall; right after the next one #WR is still low.
        xtal1_lo();
        delayNanoseconds(xtal1_lo_ns);
        xtal1_hi();
        // assert_debug();
        s->getData();
        // negate_debug();
        if (s->writeMemory())
            writeBus(s);
    }
    if (idling()) {
        delayNanoseconds(xtal1_hi_ns);
    } else {
        Cycles::next();
    }
    noInterrupts();
    // assert_debug();
    // #RD or #WR stays low three rising edges and rises after the next one;
    // the next cycle's ALE only after the edge after that, in prepareCycle().
    xtal1_lo();
    delayNanoseconds(xtal1_lo_ns);
    xtal1_hi();
    xtal1_lo();
    // negate_debug();
    interrupts();
    return s;
}

uint16_t PinsI8096::execute(uint16_t org, const uint8_t *inst, uint_fast8_t len,
        uint8_t *buf, uint_fast8_t max, uint32_t exit, bool idle, uint16_t at,
        const uint8_t *data, uint_fast8_t size) {
    constexpr uint8_t NOP = 0xFD;
    _idle = idle;
    const uint16_t leaves = exit == EXIT_PARK ? org : exit;
    // The exit counts once the window's last byte was read: a sequence
    // that jumps back to its origin reads it first.
    bool whole = len == 0;
    uint_fast8_t cap = 0;
    uint16_t first = org;
    // Bound the damage: a lost CPU never reads the exit.
    for (auto guard = 0; guard < 100; ++guard) {
        auto s = prepareCycle();
        if (s->read()) {
            const uint16_t from = s->addr;
            const uint16_t to = from + s->bytes();
            if (InstI8096::vector(from) &&
                    static_cast<uint16_t>(from - org) >= len &&
                    static_cast<uint16_t>(from - at) >= size) {
                // An interrupt took the CPU before the sequence ran: leave
                // its vector's read to the caller.
                hold(s, from);
                return first;
            }
            // ATTENTION: a PTS cycle's vector read, 2040H-205FH on the
            // 80C196KB, KC and NP, is answered with NOPs as a fetch ahead.
            if (whole && cap >= max && static_cast<uint16_t>(leaves - from) <
                                               static_cast<uint16_t>(to - from)) {
                hold(s, leaves);
                return first;
            }
            uint16_t word = 0;
            for (uint_fast8_t i = 0; i < s->bytes(); ++i) {
                const uint16_t addr = from + i;
                const uint16_t off = addr - org;
                const uint16_t pop = addr - at;
                uint8_t b = NOP;  // a fetch ahead
                if (off < len) {
                    b = inst[off];
                    if (off == len - 1)
                        whole = true;
                } else if (pop < size) {
                    b = data[pop];
                }
                word |= b << ((addr & 1) ? 8 : 0);
            }
            s->inject(word);
            completeCycle(s);
        } else if (s->write()) {
            const auto capturing = cap < max;
            if (capturing)
                s->capture();
            completeCycle(s);
            if (capturing) {
                if (cap == 0)
                    first = s->addr;
                for (uint_fast8_t i = 0; i < s->bytes() && cap < max; ++i)
                    buf[cap++] = s->byteAt(s->addr + i);
            }
        } else {
            break;
        }
    }
    if (!idle) {
        cli.print("?lost the CPU running at ");
        cli.printHex(org, 4);
        cli.print(" to ");
        cli.printlnHex(leaves, 4);
    }
    _held = false;
    return first;
}

uint16_t PinsI8096::execInst(const uint8_t *inst, uint_fast8_t len,
        uint8_t *buf, uint_fast8_t max, uint32_t exit) {
    return execute(_park, inst, len, buf, max, exit, false);
}

void PinsI8096::popInst(const uint8_t *inst, uint_fast8_t len, uint16_t at,
        const uint8_t *data, uint_fast8_t size, uint32_t exit) {
    execute(_park, inst, len, nullptr, 0, exit, false, at, data, size);
}

void PinsI8096::idle() {
    // The maximum duration of READY=L is 1us and useless for idle.
    static constexpr uint8_t SJMP_HERE[] = {
            SJMP(-2),  // SJMP $
    };
    execute(_park, SJMP_HERE, sizeof(SJMP_HERE), nullptr, 0, EXIT_PARK, true);
}

// A word in the data space: the register file is the CPU's.
uint16_t PinsI8096::readData16(uint16_t addr) const {
    return uint16(_mems->get_data(addr + 1), _mems->get_data(addr));
}

// The S80296SA on its board was seen taking an interrupt inside a call,
// after the call pushed its return address |pc| at |sp|: the cycle |push|
// before the vector read is that push, and the interrupt pushes |pc|
// again (W 0FFC=21FF, R 2006, W 0FFA=21FF). The call's target is where
// the CPU resumes; its push stays.
// ATTENTION: the S80C196NP is unchecked.
uint16_t PinsI8096::skippedCall(
        const Signals *push, uint16_t sp, uint16_t pc) const {
    if (!push->write() || push->addr != sp ||
            uint16(push->byteAt(sp + 1), push->byteAt(sp)) != pc)
        return pc;
    if (_cpu == CPU_80296SA && _mems->read(pc - 4) == 0xF1)  // ECALL
        return pc + static_cast<int16_t>(_mems->read16(pc - 3));
    if (_mems->read(pc - 3) == 0xEF)  // LCALL
        return pc + static_cast<int16_t>(_mems->read16(pc - 2));
    const auto opc = _mems->read(pc - 2);
    if ((opc & 0xF8) == 0x28) {  // SCALL
        const auto opr = ((opc & 7) << 8) | _mems->read(pc - 1);
        constexpr auto sign = 1 << 10;
        constexpr auto mask = (sign << 1) - 1;
        return pc + static_cast<int16_t>(((opr + sign) & mask) - sign);
    }
    return pc;
}

uint16_t PinsI8096::jumpTarget(uint16_t next, uint_fast8_t opc) const {
    const auto pc = _regs->nextIp();
    if (opc >= 0x20 && opc < 0x30) {  // SJMP/SCALL
        const auto opr = ((opc & 7) << 8) | _mems->read(pc + 1);
        constexpr auto sign = 1 << 10;
        constexpr auto mask = (sign << 1) - 1;
        const auto delta = static_cast<int16_t>(((opr + sign) & mask) - sign);
        return next + delta;
    } else if (opc >= 0x30 && opc < 0x40) {  // JBC/JBS
        return next + static_cast<int8_t>(_mems->read(pc + 2));
    } else if (opc >= 0xD0 && opc < 0xE0) {  // Jcc
        return next + static_cast<int8_t>(_mems->read(pc + 1));
    } else if (opc == 0xE0 || (c196() && opc == 0xE1)) {  // DJNZ/DJNZW
        return next + static_cast<int8_t>(_mems->read(pc + 2));
    } else if (_cpu >= CPU_80C196KC && opc == 0xE2) {
        // TIJMP TBASE,[INDEX],#MASK
        const auto index = _mems->get_data(readData16(_mems->read(pc + 1)));
        const auto mask = _mems->read(pc + 2);
        const auto tbase = readData16(_mems->read(pc + 3));
        return readData16(tbase + (index & mask) * 2);
    } else if (opc == 0xE3) {  // BR, the 80296SA's EBR on an odd register
        return regs<RegsI8096>()->read_data16(_mems->read(pc + 1) & ~1);
    } else if (opc == 0xE7 || opc == 0xEF) {  // LJMP/LCALL
        return next + static_cast<int16_t>(_mems->read16(pc + 1));
    } else if (_cpu == CPU_80296SA && (opc == 0xE6 || opc == 0xF1)) {
        // EJMP/ECALL: a 24-bit displacement, its low 16 bits in 64-Kbyte
        // mode
        return next + static_cast<int16_t>(_mems->read16(pc + 1));
    } else if (opc == 0xF0) {  // RET
        return readData16(regs<RegsI8096>()->sp());
    }
    return next;
}

// Feeds a TRAP to the fetches past the instruction: the next one, a
// branch's target, or an interrupt's handler. INST, where the CPU has it,
// marks every fetch from the instruction stream, prefetches and operands
// too; elsewhere the address does. The instruction's own bytes come from
// memory but at a branch back to them.
bool PinsI8096::rawStep(bool show) {
    show &= !Debugger.verbose();
    InstI8096 inst;
    const CpuMemory cpu(mems<MemsI8096>());
    if (!inst.set(_regs->nextIp(), &cpu, _cpu))
        return false;
    const uint16_t pc = _regs->nextIp();
    const auto len = inst.instLength();
    const uint16_t next = pc + len;
    const auto opc = inst.opc();
    const auto target = jumpTarget(next, opc);
    uint16_t handler = next;  // once an interrupt's vector was read
    // A block move takes two cycles a word; its count is read while the
    // register file is still the debugger's.
    uint32_t cycles = 100;
    if (c196() && (opc == InstI8096::BMOV || opc == InstI8096::BMOVI ||
                          opc == InstI8096::EBMOVI))
        cycles += 2 * readData16(_mems->read(pc + 1));
    // The first read of the TRAP vector is a stepped TRAP's own.
    auto ownTrap = opc == InstI8096::TRAP;
    _regs->restore();
    if (show)
        Cycles::reset();
    _ringFirst = MatchWalker::NO_STOP;
    _idle = false;
    // Whether a fetch of |addr| in |s| gets the TRAP, once each of the
    // instruction's bytes was fetched.
    const auto stops = [&](const Signals *s, uint16_t addr) {
        if (static_cast<uint16_t>(addr - pc) < len)
            return addr == target;  // a branch back to itself
        if (hasInst())
            return s->inst();
        return addr == next || addr == target || addr == handler;
    };
    const uint8_t all = (1 << len) - 1;
    uint8_t fetched = 0;
    for (uint32_t i = 0; i < cycles; i++) {
        if (i)
            restoreExtint();
        auto s = prepareCycle();
        if (!s->read()) {
            completeCycle(s);
            continue;
        }
        // The handler a halt waits on returns to its park.
        if (_halt.waiting && s->addr == _halt.park) {
            hold(s, s->addr);
            if (saveHalt()) {
                if (show)
                    Cycles::discard(s);
                return true;
            }
            continue;
        }
        // A stepped RST's reset pushes nothing: stop at its first fetch, as
        // after the debugger's own.
        if (opc == InstI8096::RST && s->addr == InstI8096::ORG_RESET) {
            hold(s, s->addr);
            _regs->reset();
            _regs->save();
            if (show)
                Cycles::discard(s);
            return true;
        }
        if (s->addr == InstI8096::VEC_TRAP && !ownTrap) {
            handleTrap(s, 0x2344, true);
            if (show)
                Cycles::discard(ringEnd(s));
            return true;
        }
        if (InstI8096::vector(s->addr)) {
            ownTrap = false;
            handler = _mems->read16(s->addr);
            completeCycle(s);
            continue;
        }
        const auto first = fetched == 0;
        auto word = readBus(s);
        auto trap = false;
        for (uint_fast8_t j = 0; j < s->bytes(); ++j) {
            const uint16_t addr = s->addr + j;
            const uint16_t off = addr - pc;
            if (off < len && fetched != all) {
                fetched |= 1 << off;
            } else if (stops(s, addr)) {
                const auto lane = (addr & 1) ? 8 : 0;
                word &= ~(0xFF << lane);
                word |= InstI8096::TRAP << lane;
                trap = true;
            }
        }
        if (trap)
            s->inject(word);
        completeCycle(s);
        if (first && fetched)
            s->markStart(pc);
    }
    cli.println("?step: no TRAP");
    return false;
}

bool PinsI8096::step(bool show) {
    Cycles::reset();
    _ringFirst = MatchWalker::NO_STOP;
    if (rawStep(show)) {
        if (show)
            printCycles();
        return true;
    }
    return false;
}

// Where a stop at |s| cuts the ring: an 80C196's vector read follows the
// PC's push, and so does the 8096 NMI's fetch of 0000H, which reads no
// vector; the push goes with it.
Signals *PinsI8096::ringEnd(Signals *s) const {
    if (!s->read())
        return s;
    if (InstI8096::pushesBeforeVector(_cpu) && InstI8096::vector(s->addr))
        return s->prev();
    if (s->addr == InstI8096::ORG_NMI && haltEntry() == InstI8096::ORG_NMI)
        return s->prev();
    return s;
}

// |s| reads the vector: answer |vector|, capture the PC the CPU pushes
// into |pc|, and park at |vector|, an even address. Returns the SP the
// push left.
uint16_t PinsI8096::serveVector(Signals *s, uint16_t vector, uint16_t &pc) {
    const uint8_t vec[] = {lo(vector), hi(vector)};
    if (InstI8096::pushesBeforeVector(_cpu)) {
        const auto push = s->prev();
        hold(s, s->addr);
        execute(s->addr, vec, sizeof(vec), nullptr, 0, vector, false);
        pc = uint16(push->byteAt(push->addr + 1), push->byteAt(push->addr));
        return push->addr;
    }
    uint8_t push[2];
    hold(s, s->addr);
    const auto sp = execute(
            s->addr, vec, sizeof(vec), push, sizeof(push), vector, false);
    pc = push[0] | (push[1] << 8);
    return sp;
}

void PinsI8096::handleTrap(Signals *s, uint16_t vector, bool breakTrap) {
    uint16_t pc;
    const auto sp = serveVector(s, vector, pc);
    regs<RegsI8096>()->captureContext(sp, pc, breakTrap);
}

// The cycle |s| of a halt under way: whether the CPU stopped, or the board
// was reset after the CPU never took INTR_NMI.
bool PinsI8096::haltCycle(Signals *s) {
    if (_halt.waiting) {
        if (!s->read() || s->addr != _halt.park)
            return false;
        hold(s, s->addr);
        return saveHalt();
    }
    if (!s->read() || s->addr != haltEntry()) {
        if (++_halt.cycles < 2000)
            return false;
        // A masked EXTINT can keep the halt out for long; ask again before
        // giving up on the CPU.
        if (++_halt.tries < 8) {
            negateInt(INTR_NMI);
            assertInt(INTR_NMI);
            _halt.cycles = 1;
            return false;
        }
        cli.println("?halt not taken, reset");
        resetPins();
        return true;
    }
    negateInt(INTR_NMI);
    if (s->addr == InstI8096::ORG_NMI) {
        // The 8096's NMI pushes the PC, then calls 0000H.
        const auto push = s->prev();
        _halt.sp = push->addr;
        _halt.pc =
                uint16(push->byteAt(push->addr + 1), push->byteAt(push->addr));
        hold(s, s->addr);
    } else {
        const auto before = s->prev();
        _halt.sp = serveVector(s, 0x5678, _halt.pc);
        _halt.pc = skippedCall(before, _halt.sp + 2, _halt.pc);
    }
    _halt.park = park();
    _halt.waiting = true;
    return saveHalt();
}

// A halt asked while a wait an earlier break left is pending drops the
// wait: its handler may never return to the park.
// ATTENTION: if it does, it returns to the park's address in memory.
void PinsI8096::requestHalt() {
    _halt = Halt();
    assertInt(INTR_NMI);
    _halt.cycles = 1;
}

// Saves the halt's frame, the CPU held at its park. False when an
// interrupt took the CPU first, held at its vector's read: its handler
// returns to the park, where the save runs again.
bool PinsI8096::saveHalt() {
    if (!regs<RegsI8096>()->captureContext(_halt.sp, _halt.pc, false))
        return false;
    _halt = Halt();
    return true;
}

// Whether the read |s| fetched a breakpoint's TRAP.
bool PinsI8096::fetchedBreak(const Signals *s) const {
    if (!s->read())
        return false;
    for (uint_fast8_t i = 0; i < s->bytes(); ++i) {
        const uint16_t addr = s->addr + i;
        if (isBreakPoint(addr) && s->byteAt(addr) == InstI8096::TRAP)
            return true;
    }
    return false;
}

#ifdef PROFILE_CYCLES
// For scripts/record-cycles.py: stop at any TRAP and give up well before
// the ring wraps; the debug pin frames the run, to trigger a capture on.
Signals *PinsI8096::loop() {
    constexpr auto MAX_CYCLES = 96;
    // A wait an earlier break left.
    auto carried = _halt.waiting;
    assert_debug();
    for (auto n = 0;; ++n) {
        auto s = prepareCycle();
        if (s->addr == InstI8096::VEC_TRAP && s->read()) {
            negate_debug();
            handleTrap(s, 0x4566, true);
            return s;
        }
        if (programReset(s))
            continue;
        if (_halt.cycles) {
            if (haltCycle(s))
                return s;
            if (_held)
                continue;
        }
        completeCycle(s);
        restoreExtint();
        _devs->loop();
        if ((n >= MAX_CYCLES || haltSwitch()) &&
                (_halt.cycles == 0 || carried)) {
            negate_debug();
            cli.println(n >= MAX_CYCLES ? "?cycles" : "?halt");
            requestHalt();
            carried = false;
        }
    }
}
#else
Signals *PinsI8096::loop() {
    // A wait an earlier break left.
    auto carried = _halt.waiting;
    uint_fast8_t poll = 0;
    while (true) {
        auto s = prepareCycle();
        if (s->addr == InstI8096::VEC_TRAP && s->read()) {
            // A breakpoint's TRAP was fetched a few cycles before.
            for (uint_fast8_t i = 1; i < 10; ++i) {
                if (fetchedBreak(s->prev(i))) {
                    handleTrap(s, 0x3456, true);
                    return s;
                }
            }
            // A TRAP vector pointing at itself is a program's halt to system.
            if (_mems->read16(InstI8096::VEC_TRAP) == InstI8096::VEC_TRAP) {
                handleTrap(s, 0x4566, true);
                return s;
            }
        }
        if (programReset(s))
            continue;
        if (_halt.cycles) {
            if (haltCycle(s))
                return s;
            if (_held)
                continue;
        }
        completeCycle(s);
        restoreExtint();
        _devs->loop();
        if (++poll < halt_poll_cycles)
            continue;
        poll = 0;
        if (haltSwitch() && (_halt.cycles == 0 || carried)) {
            requestHalt();
            carried = false;
        }
    }
}
#endif

void PinsI8096::run() {
    _regs->restore();
    Cycles::reset();
    _ringFirst = _regs->nextIp();
    _idle = false;
    saveBreakInsts();
    startRunTimer();
#ifdef PROFILE_CYCLES
    // Keep the TRAP's cycles, its vector and pushes (1:~:Vr:Ww, a word
    // each, the push first on an 80C196): without them the matcher can't
    // place its fetch, the end of the pattern.
    _profileEnd = loop()->next(InstI8096::pushesBeforeVector(_cpu) ? 1 : 2);
    stopRunTimer();
#else
    const auto s = loop();
    stopRunTimer();
    Cycles::discard(ringEnd(s));
#endif
    // A break that got in ahead of the halt's entry.
    // ATTENTION: the CPU may have latched NMI's edge already.
    if (_halt.cycles && !_halt.waiting) {
        negateInt(INTR_NMI);
        _halt = Halt();
    }
    restoreBreakInsts();
    disassembleCycles();
}

void PinsI8096::setBreakInst(uint32_t addr) const {
    _mems->put_prog(addr, InstI8096::TRAP);
}

void PinsI8096::assertInt(uint8_t) {
    digitalWriteFast(PIN_EXTINT, HIGH);
}

void PinsI8096::negateInt(uint8_t) {
    digitalWriteFast(PIN_EXTINT, LOW);
}

void PinsI8096::printCycles(const Signals *end) {
    const auto g = Signals::get();
    const auto cycles = g->diff(end ? end : Signals::put());
    for (auto i = 0u; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

const Signals *PinsI8096::findFetch(Signals *begin, const Signals *end) {
    const CpuMemory cpu(mems<MemsI8096>());
    ArchI8096 arch(&cpu, _cpu);
    arch.setIdle(
            [](void *pins) { static_cast<PinsI8096 *>(pins)->idle(); }, this);
    auto &walker = MatchWalker::shared();
    // The ring still begins with the run once it hasn't filled.
    const auto first = begin == Signals::get() &&
                                       Cycles::cycles() < Cycles::MAX_CYCLES - 1
                               ? _ringFirst
                               : MatchWalker::NO_STOP;
    if (!walker.walk(arch, begin, end, MatchWalker::NO_STOP, first))
        return end;
    return begin->next(walker.start());
}

// findFetch() marks the fetch() cycles as it matches instructions against
// memory, so it runs before backtraceStartFrom() counts them;
// printBacktrace() runs it again and derives the same marks.
const SignalsImpl *PinsI8096::findBacktraceStart() {
    const auto end = Signals::put();
    const auto begin = findFetch(Signals::get(), end);
    return backtraceStartFrom<Signals>(begin, _lineLimit);
}

void PinsI8096::printBacktrace() {
    const auto end = Signals::put();
#ifdef PROFILE_CYCLES
    // Every cycle; the matcher's fetches print as I.
    findFetch(Signals::get(), _profileEnd ? _profileEnd : end);
    cli.println();
    printCycles(end);
#else
    const auto begin = findFetch(Signals::get(), end);
    cli.println();
    printCycles(begin);
    const auto cycles = begin->diff(end);
    for (auto i = 0u; i < cycles;) {
        const auto s = begin->next(i);
        if (s->fetch()) {
            // The instructions starting in it: a word may bring two.
            const uint16_t pc = s->startsAt(s->addr) ? s->addr : s->addr + 1;
            uint16_t nexti = _mems->disassemble(pc, 1);
            if (nexti == s->addr + 1 && s->startsAt(nexti))
                nexti = _mems->disassemble(nexti, 1);
            const uint_fast8_t len = nexti - pc;
            idle();
            // Their fetches, a word bringing two bytes; others print. A
            // word that starts the next instruction is its fetch.
            uint_fast8_t j = 0;
            for (uint_fast8_t got = 0; got < len && i + j < cycles; j++) {
                const auto t = s->next(j);
                if (j > 0 && t->fetch())
                    break;
                const uint16_t from = t->addr;
                const uint16_t to = from + t->bytes();
                const auto inside = t->read() && to > pc && from < nexti;
                if (inside)
                    got += (to < nexti ? to : nexti) - (from > pc ? from : pc);
                if (!inside || Debugger.verbose()) {
                    t->print();
                    idle();
                }
            }
            i += j;
        } else {
            s->print();
            idle();
            i++;
        }
    }
#endif
}

}  // namespace i8096
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
