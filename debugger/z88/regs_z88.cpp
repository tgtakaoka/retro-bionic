#include "regs_z88.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "mems_z88.h"
#include "pins_z88.h"

namespace debugger {
namespace z88 {

struct RegsZ88 Regs;

const char *RegsZ88::cpu() const {
    return "Z88";
}

const char *RegsZ88::cpuName() const {
    return "Z88C00";
}

void RegsZ88::print() const {
    // clang-format off
    //                               0123456789012345678901234567890123456789012345678901234
    static constexpr char line1[] = "PC=xxxx SP=xxxx IP=xxxx RP=xx:xx FLAGS=CZSVDH21";
    static constexpr char line2[] = " R0=xx  R1=xx  R2=xx  R3=xx  R4=xx  R5=xx  R6=xx  R7=xx";
    static constexpr char line3[] = " R8=xx  R9=xx R10=xx R11=xx R12=xx R13=xx R14=xx R15=xx";
    // clang-format on
    static auto &buffer1 = *new CharBuffer(line1);
    buffer1.hex16(3, _pc);
    buffer1.hex8(11, get_sfr(sfr_sph));
    buffer1.hex8(13, get_sfr(sfr_spl));
    buffer1.hex8(19, get_sfr(sfr_iph));
    buffer1.hex8(21, get_sfr(sfr_ipl));
    buffer1.hex8(27, get_sfr(sfr_rp0));
    buffer1.hex8(30, get_sfr(sfr_rp1));
    const auto flags = get_sfr(sfr_flags);
    buffer1.bits(39, flags, 0x80, &line1[39]);
    buffer1[46] = (flags & 0x01) ? '1' : '0';  // bank address
    cli.println(buffer1);
    static auto &buffer2 = *new CharBuffer(line2);
    for (auto i = 0; i < 8; ++i)
        buffer2.hex8(4 + i * 7, _r[i]);
    cli.println(buffer2);
    static auto &buffer3 = *new CharBuffer(line3);
    for (auto i = 0; i < 8; ++i)
        buffer3.hex8(4 + i * 7, _r[i + 8]);
    cli.println(buffer3);
}

void RegsZ88::switchBank(RegSpace space) {
    if (space == SET_BANK1) {
        static constexpr uint8_t BANK1[] = {
                0x46, 0xD5, 0x01,  // OR FLAGS, #F_BANK
        };
        Pins.execInst(BANK1, sizeof(BANK1));
    } else {
        static constexpr uint8_t BANK0[] = {
                0x56, 0xD5, 0xFE,  // AND FLAGS, #LNOT F_BANK
        };
        Pins.execInst(BANK0, sizeof(BANK0));
    }
}

uint8_t RegsZ88::read_reg(uint8_t addr, RegSpace space) {
    if (space == SET_NONE)
        return 0;
    uint8_t val;
    if (space == SET_TWO && addr >= 0xC0) {
        static uint8_t READ_TWO[] = {
                0xFC, 0,     // LD r15,IM
                0xC7, 0xFF,  // LD r15,@r15
                0xD3, 0xFE,  // LDE @rr14,r15
                0xFC, 0,     // LD r15,IM
        };
        READ_TWO[1] = addr;
        READ_TWO[7] = _r[15];
        Pins.captureWrites(
                READ_TWO, sizeof(READ_TWO), nullptr, &val, sizeof(val));
        // Accessed by Indirect register, Indexed regiser, and stack operation
        return val;
    }
    if (addr >= 0xE0)
        switchBank(space);
    static uint8_t READ_REG[] = {
            0xF8, 0,     // LD r15,Rn
            0xD3, 0xFE,  // LDE @rr14,r15
            0xFC, 0,     // LD r15,IM
    };
    READ_REG[1] = addr;
    READ_REG[5] = _r[15];
    Pins.captureWrites(READ_REG, sizeof(READ_REG), nullptr, &val, sizeof(val));
    if (space == SET_ONE || space == SET_TWO)
        update_r(addr, val);
    return val;
}

void RegsZ88::write_reg(uint8_t addr, uint8_t val) {
    static uint8_t WRITE_REG[] = {
            0xE6, 0, 0,  // LD Rn,IM
    };
    WRITE_REG[1] = addr;
    WRITE_REG[2] = val;
    Pins.execInst(WRITE_REG, sizeof(WRITE_REG));
    update_r(addr, val);
}

void RegsZ88::reset() {
    constexpr auto EMT = 254;  // External Memory Timing
    write_reg(EMT, (Z88_EXTERNAL_STACK << 1));
    constexpr auto P0M = 240;  // Port 0 Mode
    write_reg(P0M, 0xFF);      // Port 0 = A15-A8
#if Z88_DATA_MEMORY
    constexpr auto P2CM = 250;  // Port 2/3 C Mode
    write_reg(P2CM, 2 << 6);    // P35=Output, Push-Pull
#endif
    constexpr auto PM = 241;   // Port 1 Mode
    write_reg(PM, (Z88_DATA_MEMORY << 3) | (2 << 4));
    static constexpr uint8_t JP_RESET[] = {0x8D, 0x00, 0x20};  // JP %0020
    Pins.execInst(JP_RESET, sizeof(JP_RESET));
}

void RegsZ88::update_r(uint8_t addr, uint8_t val) {
    const auto rp0 = get_sfr(sfr_rp0) & 0xF8;
    if ((addr & 0xF8) == rp0)
        _r[addr & 7] = val;
    const auto rp1 = get_sfr(sfr_rp1) & 0xF8;
    if ((addr & 0xF8) == rp1)
        _r[(addr & 7) + 8] = val;
}

void RegsZ88::save_r(uint8_t num) {
    static uint8_t SAVE_R[] = {
            0xD3, 0x0E,  // LDE @rr14, rn
    };
    SAVE_R[1] = (num << 4) | 14;  // LDE @rr14,ri
    Pins.captureWrites(
            SAVE_R, sizeof(SAVE_R), nullptr, &_r[num], sizeof(_r[0]));
}

void RegsZ88::save() {
    const auto *signals = Pins.cycle(0xFF);  // NOP
    _pc = signals->addr;

    for (uint8_t num = 0; num < sizeof(_r); num++)
        save_r(num);
    for (uint8_t num = 0; num < sizeof(_sfr); num++)
        _sfr[num] = read_reg(sfr_base + num);
    restore_r(15);
}

void RegsZ88::restore_r(uint8_t num) {
    static uint8_t LOAD_R[] = {
            0x0C, 0,  // LD r,IM
    };
    LOAD_R[0] = 0x0C | (num << 4);
    LOAD_R[1] = _r[num];
    Pins.execInst(LOAD_R, sizeof(LOAD_R));
}

void RegsZ88::restore() {
    static uint8_t JP[] = {
            0x8D, 0, 0,  // JP DA
    };

    for (uint8_t num = 0; num < sizeof(_sfr); num++)
        write_reg(sfr_base + num, _sfr[num]);
    for (uint8_t num = 0; num < sizeof(_r); num++)
        restore_r(num);

    JP[1] = hi(_pc);
    JP[2] = lo(_pc);
    Pins.execInst(JP, sizeof(JP));
}

void RegsZ88::set_rp0(uint8_t val) {
    set_sfr(sfr_rp0, val);
    for (auto num = 0; num < 8; num++)
        save_r(num);
}

void RegsZ88::set_rp1(uint8_t val) {
    set_sfr(sfr_rp1, val);
    for (auto num = 8; num < 16; num++)
        save_r(num);
}

void RegsZ88::set_sp(uint16_t val) {
    set_sfr(sfr_sph, hi(val));
    set_sfr(sfr_spl, lo(val));
}

void RegsZ88::set_ip(uint16_t val) {
    set_sfr(sfr_iph, hi(val));
    set_sfr(sfr_ipl, lo(val));
}

void RegsZ88::set_sfr(uint8_t name, uint8_t val) {
    const auto num = name - sfr_base;
    _sfr[num] = val;
    write_reg(name, val);
}

void RegsZ88::set_r(uint8_t num, uint8_t val) {
    _r[num] = val;
    restore_r(num);
}

void RegsZ88::helpRegisters() const {
    cli.println(F("?Reg: PC SP IP RP/0/1 FLAGS R0~R15 RR0~14"));
}

constexpr const char *REGS8[] = {
        "R0",     // 1
        "R1",     // 2
        "R2",     // 3
        "R3",     // 4
        "R4",     // 5
        "R5",     // 6
        "R6",     // 7
        "R7",     // 8
        "R8",     // 9
        "R9",     // 10
        "R10",    // 11
        "R11",    // 12
        "R12",    // 13
        "R13",    // 14
        "R14",    // 15
        "R15",    // 16
        "FLAGS",  // 17
        "RP",     // 18
        "RP0",    // 19
        "RP1",    // 20
};
constexpr const char *REGS16[] = {
        "RR0",   // 21
        "RR2",   // 22
        "RR4",   // 23
        "RR6",   // 24
        "RR8",   // 25
        "RR10",  // 26
        "RR12",  // 27
        "RR14",  // 28
        "PC",    // 29
        "SP",    // 30
        "IP",    // 31
};

const Regs::RegList *RegsZ88::listRegisters(uint8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS8, 20, 1, UINT8_MAX},
            {REGS16, 11, 21, UINT16_MAX},
    };
    return n < 2 ? &REG_LIST[n] : nullptr;
}

void RegsZ88::setRegister(uint8_t reg, uint32_t value) {
    switch (reg) {
    case 29:
        _pc = value;
        break;
    case 30:
        set_sp(value);
        break;
    case 31:
        set_ip(value);
        break;
    case 18:
        set_rp0(value);
        set_rp1(value + 8);
        break;
    case 19:
        set_rp0(value);
        break;
    case 20:
        set_rp1(value);
        break;
    case 17:
        set_flags(value);
        break;
    default:
        if (reg >= 21) {
            set_rr((reg - 21U) * 2, value);
        } else {
            set_r(reg - 1U, value);
        }
        break;
    }
}

}  // namespace z88
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
