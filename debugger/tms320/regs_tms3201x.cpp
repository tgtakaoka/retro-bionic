#include "regs_tms3201x.h"
#include "debugger.h"
#include "inst_tms3201x.h"
#include "pins_tms320.h"

namespace debugger {
namespace tms3201x {

using tms3201x::InstTms3201X;

namespace {
// clang-format off
//                    01234567890123456789012345678901234567
const char line1[] = "PC=xxx  ACC=xxxxxxxx P=xxxxxxxx T=xxxx";
const char line2[] = "AR0=xxxx AR1=xxxx  ST=VMI ARP=x DP=x";
// clang-format on
}  // namespace

RegsTms3201X::RegsTms3201X(tms320::PinsTms320 *pins)
    : _pins(pins), _buffer1(line1), _buffer2(line2) {}

const char *RegsTms3201X::cpu() const {
    return "TMS32010";
}

void RegsTms3201X::print() const {
    _buffer1.hex12(3, _pc);
    _buffer1.hex32(12, _acc);
    _buffer1.hex32(23, _p);
    _buffer1.hex16(34, _t);
    _pins->idle();
    cli.println(_buffer1);
    _buffer2.hex16(4, _ar[0]);
    _buffer2.hex16(13, _ar[1]);
    _buffer2.bits(22, _st, 0x8000, "VMI");
    _buffer2.hex1(30, (_st >> 8) & 1);
    _buffer2.hex1(35, _st & 1);
    _pins->idle();
    cli.println(_buffer2);
    _pins->idle();
}

void RegsTms3201X::reset() {
    ;
}

uint32_t RegsTms3201X::saveAcc() const {
    _pins->injectRead(InstTms3201X::SACL(DIR_DMA));
    _pins->injectRead(InstTms3201X::OUT(0, DIR_DMA));
    const auto lo16 = _pins->captureWrite();
    _pins->injectRead(InstTms3201X::SACH(DIR_DMA));
    _pins->injectRead(InstTms3201X::OUT(0, DIR_DMA));
    const auto hi16 = _pins->captureWrite();
    return uint32(hi16, lo16);
}

uint16_t RegsTms3201X::saveAr(uint_fast8_t n) const {
    _pins->injectRead(InstTms3201X::SAR(n, DIR_DMA));
    _pins->injectRead(InstTms3201X::OUT(0, DIR_DMA));
    return _pins->captureWrite();
}

void RegsTms3201X::save() {
    // save PC
    _pc = _pins->injectRead(InstTms3201X::OUT(0, DIR_DMA));
    // save working memory (DP is yet unknown)
    const auto tmp = _pins->captureWrite();
    // save ACC
    _acc = saveAcc();
    // save P
    _pins->injectRead(InstTms3201X::PAC);
    _p = saveAcc();
    // save T
    _pins->injectRead(InstTms3201X::MPYK(1));
    _pins->injectRead(InstTms3201X::PAC);
    _pins->injectRead(InstTms3201X::SACL(DIR_DMA));
    _pins->injectRead(InstTms3201X::OUT(0, DIR_DMA));
    _t = _pins->captureWrite();
    // save AR0
    _ar[0] = saveAr(0);
    // save AR1
    _ar[1] = saveAr(1);
    // restore working memory
    _pins->injectRead(InstTms3201X::IN(0, DIR_DMA));
    _pins->injectRead(tmp);
    // save working memory for ST using AR (ARP is unknown);
    _pins->injectRead(InstTms3201X::LARK(0, DMA));
    _pins->injectRead(InstTms3201X::LARK(1, DMA));
    _pins->injectRead(InstTms3201X::OUT(0, INDIR_AR));
    const auto save = _pins->captureWrite();
    // save ST into page 1
    _pins->injectRead(InstTms3201X::SST(INDIR_AR));
    _pins->injectRead(InstTms3201X::OUT(0, INDIR_AR));
    _st = _pins->captureWrite();
    // restore working memory for ST
    _pins->injectRead(InstTms3201X::IN(0, INDIR_AR));
    _pins->injectRead(save);  // restore work memory
}

void RegsTms3201X::loadReg(
        uint_fast8_t dma, uint16_t val, uint16_t inst) const {
    _pins->injectRead(InstTms3201X::IN(0, dma));
    _pins->injectRead(val);
    _pins->injectRead(inst);
}

void RegsTms3201X::restore() {
    // save page1 working memory for ST (ARP is unknown)
    _pins->injectRead(InstTms3201X::LARK(0, DMA));
    _pins->injectRead(InstTms3201X::LARK(1, DMA));
    _pins->injectRead(InstTms3201X::OUT(0, INDIR_AR));
    const auto work = _pins->captureWrite();
    // restore ST from page 1
    loadReg(DMA, _st, InstTms3201X::LST(INDIR_AR));
    // restore page1 working memory for ST
    _pins->injectRead(InstTms3201X::IN(0, INDIR_AR));
    _pins->injectRead(work);
    // save working memory
    _pins->injectRead(InstTms3201X::OUT(0, DIR_DMA));
    const auto tmp = _pins->captureWrite();
    // restore T
    loadReg(DIR_DMA, _t, InstTms3201X::LT(DIR_DMA));
    // restore ACC
    loadReg(DIR_DMA, lo16(_acc), InstTms3201X::ZALS(DIR_DMA));
    loadReg(DIR_DMA, hi16(_acc), InstTms3201X::ADDH(DIR_DMA));
    // restore AR0
    loadReg(DIR_DMA, _ar[0], InstTms3201X::LAR(0, DIR_DMA));
    // restore AR1
    loadReg(DIR_DMA, _ar[1], InstTms3201X::LAR(1, DIR_DMA));
    // restore working memory
    _pins->injectRead(InstTms3201X::IN(0, DIR_DMA));
    _pins->injectRead(tmp);
    // restore PC
    _pins->injectRead(InstTms3201X::B);
    _pins->injectRead(_pc);
}

uint16_t RegsTms3201X::read_data(uint_fast8_t addr) const {
    _pins->injectRead(InstTms3201X::LARP(0));
    _pins->injectRead(InstTms3201X::LARK(0, addr));
    _pins->injectRead(InstTms3201X::OUT(0, INDIR_AR));
    return _pins->captureWrite();
}

void RegsTms3201X::write_data(uint_fast8_t addr, uint16_t data) const {
    _pins->injectRead(InstTms3201X::LARP(0));
    _pins->injectRead(InstTms3201X::LARK(0, addr));
    _pins->injectRead(InstTms3201X::IN(0, INDIR_AR));
    _pins->injectRead(data);
}

void RegsTms3201X::helpRegisters() const {
    cli.println("?Reg: PC ST ACC AR0 AR1 ARP DP T");
}

constexpr const char *REGS16[] = {
        "PC",   // 1
        "ST",   // 2
        "AR0",  // 3
        "AR1",  // 4
        "T",    // 5
};

constexpr const char *REGS32[] = {
        "ACC",  // 6
};

constexpr const char *REGS1[] = {
        "ARP",  // 7
        "DP",   // 8
};

const Regs::RegList *RegsTms3201X::listRegisters(uint_fast8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS16, 5, 1, UINT16_MAX},
            {REGS32, 1, 6, UINT32_MAX},
            {REGS1, 7, 2, 1},
    };
    return n < 3 ? &REG_LIST[n] : nullptr;
}

bool RegsTms3201X::setRegister(uint_fast8_t reg, uint32_t value) {
    switch (reg) {
    case 1:
        _pc = value;
        return true;
    case 2:
        _st = value;
        break;
    case 3:
    case 4:
        _ar[reg - 3] = value;
        break;
    case 5:
        _t = value;
        break;
    case 6:
        _acc = value;
        break;
    case 7:
        _st = (_st & ~0x100) | (value << 8);
        break;
    case 8:
        _st = (_st & ~1) | value;
        break;
    default:
        break;
    }
    return false;
}

}  // namespace tms3201x
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
