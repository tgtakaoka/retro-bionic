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
const char line2[] = "AR0=xxxx AR1=xxxx ST=VMII____A_______D";
// clang-format on
}  // namespace

RegsTms3201X::RegsTms3201X(tms320::PinsTms320 *pins)
    : _pins(pins), _buffer1(line1), _buffer2(line2) {}

const char *RegsTms3201X::cpu() const {
    return "TMS32010";
}

void RegsTms3201X::print() const {
    ;
}

void RegsTms3201X::reset() {
    ;
}

uint32_t RegsTms3201X::saveAcc() const {
    static constexpr uint16_t SAVE_ACCL[] = {
            InstTms3201X::SACL(DIR_DMA),
            InstTms3201X::OUT(0, DIR_DMA),
    };
    const auto lo16 = _pins->captureWrite(SAVE_ACCL, length(SAVE_ACCL));
    static constexpr uint16_t SAVE_ACCH[] = {
            InstTms3201X::SACH(DIR_DMA),
            InstTms3201X::OUT(0, DIR_DMA),
    };
    const auto hi16 = _pins->captureWrite(SAVE_ACCH, length(SAVE_ACCH));
    return uint32(hi16, lo16);
}

uint16_t RegsTms3201X::saveAr(uint_fast8_t n) const {
    const uint16_t SAVE_AR[] = {
            InstTms3201X::SAR(n, DIR_DMA),
            InstTms3201X::OUT(0, DIR_DMA),
    };
    return _pins->captureWrite(SAVE_AR, length(SAVE_AR));
}

void RegsTms3201X::save() {
    // save PC
    _pc = _pins->injectRead(InstTms3201X::OUT(0, DIR_DMA));
    // save working memory (DP is yet unknown)
    const auto tmp = _pins->captureWrite(nullptr, 0);
    // save ACC
    _acc = saveAcc();
    // save P
    _pins->injectRead(InstTms3201X::PAC);
    _p = saveAcc();
    // save T
    static constexpr uint16_t SAVE_P[] = {
            InstTms3201X::MPYK(1),
            InstTms3201X::PAC,
            InstTms3201X::SACL(DIR_DMA),
            InstTms3201X::OUT(0, DIR_DMA),
    };
    _t = _pins->captureWrite(SAVE_P, length(SAVE_P));
    // save AR0
    _ar[0] = saveAr(0);
    // save AR1
    _ar[1] = saveAr(1);
    // restore working memory
    _pins->injectRead(InstTms3201X::IN(0, DIR_DMA));
    _pins->injectRead(tmp);
    // save working memory for ST using AR0
    static constexpr uint16_t SAVE_WORK[] = {
            InstTms3201X::LARK(0, DMA),
            InstTms3201X::OUT(0, INDIR_AR0),
    };
    const auto save = _pins->captureWrite(SAVE_WORK, length(SAVE_WORK));
    // save ST into page 1
    static constexpr uint16_t SAVE_ST[] = {
            InstTms3201X::SST(DIR_DMA),
            InstTms3201X::OUT(0, INDIR_AR0),
    };
    _st = _pins->captureWrite(SAVE_ST, length(SAVE_ST));
    // restore working memory for ST
    _pins->injectRead(InstTms3201X::IN(0, INDIR_AR0));
    _pins->injectRead(save);  // restore work memory
}

void RegsTms3201X::loadReg(
        uint_fast8_t dma, uint16_t val, uint16_t inst) const {
    _pins->injectRead(InstTms3201X::IN(0, dma));
    _pins->injectRead(val);
    _pins->injectRead(inst);
}

void RegsTms3201X::restore() {
    // save working memory for ST using AR0
    static constexpr uint16_t SAVE_WORK[] = {
            InstTms3201X::LARK(0, DMA),
            InstTms3201X::OUT(0, INDIR_AR0),
    };
    const auto work = _pins->captureWrite(SAVE_WORK, length(SAVE_WORK));
    // restore ST from page 1
    loadReg(INDIR_AR0, _st, InstTms3201X::LST(INDIR_AR0));
    // restore working memory for ST
    // save working memory (don't care DP)
    const uint16_t SAVE_TMP[] = {
            InstTms3201X::IN(0, INDIR_AR0),
            work,
            InstTms3201X::OUT(0, DIR_DMA),
    };
    const auto tmp = _pins->captureWrite(SAVE_TMP, length(SAVE_TMP));
    // restore P using MPY
    auto t = _t;
    const auto f = factor(_p, t);
    loadReg(DIR_DMA, t, InstTms3201X::LT(DIR_DMA));
    loadReg(DIR_DMA, f, InstTms3201X::MPY(DIR_DMA));
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

void RegsTms3201X::helpRegisters() const {
    cli.println("?Reg: PC ST ACC AR0 AR1 ARP DP T P");
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
        "P",    // 7
};

constexpr const char *REGS1[] = {
        "ARP",  // 8
        "DP",   // 9
};

const Regs::RegList *RegsTms3201X::listRegisters(uint_fast8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS16, 5, 1, UINT16_MAX},
            {REGS32, 2, 6, UINT32_MAX},
            {REGS1, 8, 2, 1},
    };
    return n < 1 ? &REG_LIST[n] : nullptr;
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
        _p = value;
        break;
    case 8:
        _st = (_st & ~0x100) | (value << 8);
        break;
    case 9:
        _st = (_st & ~1) | value;
        break;
    default:
        break;
    }
    return false;
}

uint16_t RegsTms3201X::factor(uint32_t p, uint16_t &t) {
    if (t) {
        const auto f = p / t;
        if (f <= UINT16_MAX && p - f * t == 0)
            return f;
    }
    auto f = (t == 0) ? UINT16_MAX : p / t;
    if (f > UINT16_MAX)
        f = UINT16_MAX;
    while (f) {
        if (p % f == 0) {
            t = p / f;
            return f;
        }
        f--;
    }
    return 0;
}

}  // namespace tms3201x
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
