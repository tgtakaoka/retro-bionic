#include "regs_tms320c2x.h"
#include "debugger.h"
#include "inst_tms320c2x.h"
#include "pins_tms320c25.h"

namespace debugger {
namespace tms320c2x {

namespace {
// clang-format off
//                    012345678901234567890123456789012345678901234567890123456789012345678901
const char line1[] = "PC=xxxx ACC=xxxxxxxx P=xxxxxxxx T=xxxx";
const char line2[] = "AR0=xxxx AR1=xxxx AR2=xxxx AR3=xxxx AR4=xxxx AR5=xxxx AR6=xxxx AR7=xxxx";
const char line3[] = "ST0: ARP=x DP=xxx VMI  ST1: ARB=x NTSC HFXOT PM=x";
// clang-format on

using Inst = InstTms320C2X;

// The working word: X on the current data page, whatever DP is.
constexpr uint8_t X = 0x7F;
// Page 511 is off-chip, so X there is a bus cycle the Teensy sees.
constexpr uint16_t PAGE_EXT = 0x1FF;
constexpr uint16_t EXT = (PAGE_EXT << 7) | X;
// The I/O port the injected OUT and IN use; its cycles never reach devices.
constexpr uint8_t PA = 0;
}  // namespace

RegsTms320C2X::RegsTms320C2X(PinsTms320C25 *pins)
    : _pins(pins), _buffer1(line1), _buffer2(line2), _buffer3(line3) {}

const char *RegsTms320C2X::cpu() const {
    return "TMS320C25";
}

void RegsTms320C2X::print() const {
    _buffer1.hex16(3, _pc);
    _buffer1.hex32(12, _acc);
    _buffer1.hex32(23, _p);
    _buffer1.hex16(34, _t);
    _pins->idle();
    cli.println(_buffer1);
    for (auto i = 0; i < 8; i++)
        _buffer2.hex16(4 + i * 9, _ar[i]);
    _pins->idle();
    cli.println(_buffer2);
    _buffer3.hex1(9, _st0 >> 13);
    _buffer3.hex12(14, _st0 & 0x1FF);
    _buffer3.bits(18, _st0, 0x1000, "VM");
    _buffer3.bits(20, _st0, Inst::ST0_INTM, "I");
    _buffer3.hex1(32, _st1 >> 13);
    _buffer3.bits(34, _st1, 0x1000, "NTSC");
    _buffer3.bits(39, _st1, 0x0040, "HFXOT");
    _buffer3.hex1(48, _st1 & 3);
    _pins->idle();
    cli.println(_buffer3);
    _pins->idle();
}

// OUT the working word to the port and capture it.
uint16_t RegsTms320C2X::outSlot() const {
    _pins->injectRead(Inst::OUT(X, PA));
    return _pins->captureWrite();
}

void RegsTms320C2X::save() {
    save(_pins->fetchAddr(), -1);
}

void RegsTms320C2X::save(uint16_t pc, int32_t ret) {
    _pc = pc;
    // DP and ARP are unknown: keep the working word, then pass ACC and the
    // ARs through it
    const auto work = outSlot();
    for (auto n = 0; n < 8; n++) {
        _pins->injectRead(Inst::SAR(n, X));
        _ar[n] = outSlot();
    }
    _pins->injectRead(Inst::SACL(X));
    const auto acc_lo = outSlot();
    _pins->injectRead(Inst::SACH(X));
    _acc = uint32(outSlot(), acc_lo);
    _pins->injectRead(Inst::IN(X, PA));
    _pins->injectIo(work);
    // every AR points off-chip, so stores through the current one are bus
    // cycles whichever ARP is; SST and SST1 ignore DP only to use page 0
    for (auto n = 0; n < 8; n++) {
        _pins->injectRead(Inst::LRLK(n));
        _pins->injectRead(EXT);
    }
    _pins->injectRead(Inst::SST(Inst::IND));
    _st0 = _pins->captureData(EXT);
    _pins->injectRead(Inst::SST1(Inst::IND));
    _st1 = _pins->captureData(EXT);
    // P whole, through ACC with no product shift
    _pins->injectRead(Inst::SPM(0));
    _pins->injectRead(Inst::PAC);
    _pins->injectRead(Inst::SACL(Inst::IND));
    const auto p_lo = _pins->captureData(EXT);
    _pins->injectRead(Inst::SACH(Inst::IND));
    _p = uint32(_pins->captureData(EXT), p_lo);
    // T, as P = T * 1
    _pins->injectRead(Inst::MPYK(1));
    _pins->injectRead(Inst::PAC);
    _pins->injectRead(Inst::SACL(Inst::IND));
    _t = _pins->captureData(EXT);
    // An interrupt taken while halting pushed the address of a NOP the
    // halt fed; return to the instruction those NOPs stood in for.
    _pins->injectRead(Inst::POP);
    _pins->injectRead(Inst::SACL(Inst::IND));
    auto tos = _pins->captureData(EXT);
    if (ret >= 0 && static_cast<uint16_t>(tos - ret - 1) < DRAIN_FETCHES)
        tos = ret;
    _pins->injectRead(Inst::LALK);
    _pins->injectRead(tos);
    _pins->injectRead(Inst::PUSH);
    // keep interrupts off while halted; restore() sets INTM back
    // ATTENTION: the timer keeps counting through the save; a TINT the
    // program enabled can still be taken before this DINT
    _pins->injectRead(Inst::DINT);
}

void RegsTms320C2X::restore() {
    // everything loads by direct addressing from page 511, off-chip, so each
    // operand is a bus read the Teensy answers
    _pins->injectRead(Inst::LDPK(PAGE_EXT));
    // P has no low half load: T = 1 and MPY give the low half, sign
    // extended, and LPH the high half
    _pins->injectRead(Inst::LT(X));
    _pins->injectData(EXT, 1);
    _pins->injectRead(Inst::MPY(X));
    _pins->injectData(EXT, lo16(_p));
    _pins->injectRead(Inst::LPH(X));
    _pins->injectData(EXT, hi16(_p));
    _pins->injectRead(Inst::LT(X));
    _pins->injectData(EXT, _t);
    _pins->injectRead(Inst::ZALH(X));
    _pins->injectData(EXT, hi16(_acc));
    _pins->injectRead(Inst::ADDS(X));
    _pins->injectData(EXT, lo16(_acc));
    for (auto n = 0; n < 8; n++) {
        _pins->injectRead(Inst::LAR(n, X));
        _pins->injectData(EXT, _ar[n]);
    }
    // LST1 copies ARB to ARP too, so LST goes last; it also sets DP back.
    // ADDS left C and OV, which these restore.
    _pins->injectRead(Inst::LST1(X));
    _pins->injectData(EXT, _st1);
    _pins->injectRead(Inst::LST(X));
    _pins->injectData(EXT, _st0);
    // LST leaves INTM alone; an interrupt waits for the instruction after
    // EINT, the branch
    _pins->injectRead((_st0 & Inst::ST0_INTM) ? Inst::DINT : Inst::EINT);
    _pins->branchTo(_pc);
}

uint16_t RegsTms320C2X::read_data(uint16_t addr) const {
    _pins->injectRead(Inst::LRLK(0));
    _pins->injectRead(addr);
    _pins->injectRead(Inst::LARP(0));
    _pins->injectRead(Inst::OUT(Inst::IND, PA));
    return _pins->captureWrite();
}

void RegsTms320C2X::write_data(uint16_t addr, uint16_t data) const {
    _pins->injectRead(Inst::LRLK(0));
    _pins->injectRead(addr);
    _pins->injectRead(Inst::LARP(0));
    _pins->injectRead(Inst::IN(Inst::IND, PA));
    _pins->injectIo(data);
}

void RegsTms320C2X::helpRegisters() const {
    cli.println("?Reg: PC ST0 ST1 T AR0-AR7 ACC P ARP ARB DP BIO");
}

constexpr const char *REGS16[] = {
        "PC",   // 1
        "ST0",  // 2
        "ST1",  // 3
        "T",    // 4
        "AR0",  // 5
        "AR1",  // 6
        "AR2",  // 7
        "AR3",  // 8
        "AR4",  // 9
        "AR5",  // 10
        "AR6",  // 11
        "AR7",  // 12
};

constexpr const char *REGS32[] = {
        "ACC",  // 13
        "P",    // 14
};

constexpr const char *REGS3[] = {
        "ARP",  // 15
        "ARB",  // 16
};

constexpr const char *REGS9[] = {
        "DP",  // 17
};

constexpr const char *REGS1[] = {
        "BIO",  // 18: drives the #BIO pin
};

const Regs::RegList *RegsTms320C2X::listRegisters(uint_fast8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS16, 12, 1, UINT16_MAX},
            {REGS32, 2, 13, UINT32_MAX},
            {REGS3, 2, 15, 7},
            {REGS9, 1, 17, 0x1FF},
            {REGS1, 1, 18, 1},
    };
    return n < 5 ? &REG_LIST[n] : nullptr;
}

bool RegsTms320C2X::setRegister(uint_fast8_t reg, uint32_t value) {
    switch (reg) {
    case 1:
        _pc = value;
        return true;
    case 2:
        _st0 = value;
        break;
    case 3:
        _st1 = value;
        break;
    case 4:
        _t = value;
        break;
    case 13:
        _acc = value;
        break;
    case 14:
        _p = value;
        break;
    case 15:
        _st0 = (_st0 & 0x1FFF) | (value << 13);
        break;
    case 16:
        _st1 = (_st1 & 0x1FFF) | (value << 13);
        break;
    case 17:
        _st0 = (_st0 & ~0x1FF) | value;
        break;
    case 18:
        _pins->setBio(value != 0);
        break;
    default:
        if (reg >= 5 && reg <= 12)
            _ar[reg - 5] = value;
        break;
    }
    return false;
}

}  // namespace tms320c2x
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
