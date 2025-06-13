#include "regs_z88.h"
#include "char_buffer.h"
#include "debugger.h"
#include "inst_z88.h"
#include "pins_z88.h"

namespace debugger {
namespace z88 {
namespace {
// clang-format off
//                    01234567890123456789012345678901234567890123456
const char line1[] = "PC=xxxx SP=xxxx IP=xxxx RP=xx:xx FLAGS=CZSVDH12";
// clang-format on
}  // namespace

RegsZ88::RegsZ88(PinsZ88 *pins) : RegsZ8(pins), _buffer1(line1) {}

const char *RegsZ88::cpu() const {
    return "Z88";
}

void RegsZ88::print() const {
    _buffer1.hex16(3, _pc);
    _buffer1.hex16(11, _sp);
    _buffer1.hex16(19, _ip);
    _buffer1.hex8(27, _rp0);
    _buffer1.hex8(30, _rp1);
    _buffer1.bits(39, _flags, 0x80, line1 + 39);
    _buffer1[46] = (_flags & 0x01) ? '1' : '0';  // bank address
    cli.println(_buffer1);
    RegsZ8::print();
}

void RegsZ88::reset() {
    constexpr auto EMT = 254;  // External Memory Timing
    write_reg(EMT, (EXTERNAL_STACK << 1));
    constexpr auto P0M = 240;  // Port 0 Mode
    write_reg(P0M, 0xFF);      // Port 0 = A15-A8
    constexpr auto PM = 241;   // Port 1 Mode
    write_reg(PM, 2 << 4);
    static constexpr uint8_t JP_RESET[] = {
            0x8D,  // JP ORG_RESET
            hi(InstZ88::ORG_RESET),
            lo(InstZ88::ORG_RESET),
    };
    _pins->execInst(JP_RESET, sizeof(JP_RESET));
}

void RegsZ88::switchBank(RegSpace space) {
    if (space == SET_BANK1) {
        static constexpr uint8_t BANK1[] = {
                0x46, FLAGS, 0x01,  // OR FLAGS, #F_BANK
        };
        _pins->execInst(BANK1, sizeof(BANK1));
    } else {
        static constexpr uint8_t BANK0[] = {
                0x56, FLAGS, 0xFE,  // AND FLAGS, #LNOT F_BANK
        };
        _pins->execInst(BANK0, sizeof(BANK0));
    }
}

uint_fast8_t RegsZ88::save_rp0() const {
    static constexpr uint8_t SAVE_RP0[] = {
            0xA0, SPH,  // INCW SPH
            0x70, RP0,  // PUSH RP0
    };
    uint8_t rp0;
    _pins->captureWrites(
            SAVE_RP0, sizeof(SAVE_RP0), nullptr, &rp0, sizeof(rp0));
    return rp0;
}

void RegsZ88::restore_rp0(uint_fast8_t rp0) const {
    const uint8_t RESTORE_RP0[] = {
            0x31, InstZ88::SRP0(rp0),  // SRP0 #rp0
    };
    _pins->execInst(RESTORE_RP0, sizeof(RESTORE_RP0));
}

void RegsZ88::save_sfrs() {
    _rp0 = save_rp0();
    restore_rp0(R(0));
    const auto r0 = save_r(0);
    _flags = raw_read_reg(FLAGS);
    _sp = uint16(raw_read_reg(SPH), raw_read_reg(SPL));
    _rp1 = raw_read_reg(RP1);
    _ip = uint16(raw_read_reg(IPH), raw_read_reg(IPL));
    restore_r(0, r0);
    restore_rp0(_rp0);
}

void RegsZ88::restore_sfrs() {
    restore_rp0(R(0));
    write_reg(FLAGS, _flags);
    write_reg(SPH, hi(_sp));
    write_reg(SPL, lo(_sp));
    write_reg(RP1, _rp1);
    write_reg(IPL, lo(_ip));
    write_reg(IPH, hi(_ip));
    restore_rp0(_rp0);
}

uint_fast8_t RegsZ88::save_r(uint_fast8_t num) const {
    const uint8_t SAVE_R[] = {
            0xD3, uint8((num << 4) | 0 | 1),  // LDE @RR0,Rn
    };
    uint8_t val;
    _pins->captureWrites(SAVE_R, sizeof(SAVE_R), nullptr, &val, sizeof(val));
    return val;
}

void RegsZ88::restore_r(uint_fast8_t num, uint8_t val) const {
    if (InstZ88::writeOnly(_rp0, _rp1, num))
        return;
    const uint8_t LOAD_R[] = {
            uint8(num, 0x0C), val,  // LD rn,#val
    };
    _pins->execInst(LOAD_R, sizeof(LOAD_R));
}

uint_fast8_t RegsZ88::raw_read_reg(uint8_t addr) const {
    const uint8_t READ_REG[] = {
            0x08, addr,  // LD R0,addr
            0xD3, 0x01,  // LDE @RR0,R0
    };
    uint8_t val;
    _pins->captureWrites(
            READ_REG, sizeof(READ_REG), nullptr, &val, sizeof(val));
    return val;
}

uint_fast8_t RegsZ88::read_reg(uint8_t addr, RegSpace space) {
    const auto rp0 = save_rp0();
    restore_rp0(R(0));
    const auto r0 = save_r(0);
    uint8_t val;
    if (space == SET_TWO && addr >= 0xC0) {
        // Accessed by Indirect register, Indexed regiser, and stack operation
        const uint8_t READ_TWO[] = {
                0x0C, addr,  // LD R0,#addr
                0xC7, 0x00,  // LD R0,@R0
                0xD3, 0x01,  // LDE @RR0,R0
        };
        _pins->captureWrites(
                READ_TWO, sizeof(READ_TWO), nullptr, &val, sizeof(val));
    } else {
        if (addr >= 0xE0)
            switchBank(space);
        val = raw_read_reg(addr);
    }
    restore_r(0, r0);
    restore_rp0(rp0);
    return val;
}

void RegsZ88::write_reg(uint8_t addr, uint8_t val, RegSpace space) {
    const uint8_t WRITE_REG[] = {
            0xE6, addr, val,  // LD addr,#val
    };
    _pins->execInst(WRITE_REG, sizeof(WRITE_REG));
    update_r(addr, val);
    if (addr == RP0 || addr == RP1)
        save_all_r();
}

void RegsZ88::update_r(uint_fast8_t addr, uint_fast8_t val) {
    const uint8_t rp0 = _rp0 & 0xF8;
    if ((addr & 0xF8) == rp0)
        _r[addr & 7] = val;
    const uint8_t rp1 = _rp1 & 0xF8;
    if ((addr & 0xF8) == rp1)
        _r[(addr & 7) + 8] = val;
}

void RegsZ88::set_r(uint_fast8_t num, uint_fast8_t val) {
    write_reg(R(num), _r[num] = val);
}

void RegsZ88::set_flags(uint_fast8_t val) {
    write_reg(FLAGS, _flags = val);
}

void RegsZ88::set_sp(uint16_t val) {
    _sp = val;
    write_reg(SPH, hi(val));
    write_reg(SPL, lo(val));
}

void RegsZ88::set_ip(uint16_t val) {
    _ip = val;
    write_reg(IPH, hi(val));
    write_reg(IPL, lo(val));
}

void RegsZ88::set_rp(uint_fast8_t val) {
    set_rp0(val);
    set_rp1(val + 8);
}

void RegsZ88::set_rp0(uint_fast8_t val) {
    write_reg(RP0, _rp0 = val);
}

void RegsZ88::set_rp1(uint_fast8_t val) {
    write_reg(RP1, _rp1 = val);
}

void RegsZ88::helpRegisters() const {
    cli.println("?Reg: PC SP IP RP/0/1 FLAGS R0~R15 RR0~14");
}

constexpr const char *Z88_REGS8[] = {
        "RP0",  // 29
        "RP1",  // 30
};
constexpr const char *Z88_REGS16[] = {
        "IP",  // 31
};

const Regs::RegList *RegsZ88::listRegisters(uint_fast8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {Z88_REGS8, 2, 29, UINT8_MAX},
            {Z88_REGS16, 1, 31, UINT16_MAX},
    };
    if (n < 2)
        return RegsZ8::listRegisters(n);
    return n < 4 ? &REG_LIST[n - 2] : nullptr;
}

bool RegsZ88::setRegister(uint_fast8_t reg, uint32_t value) {
    switch (reg) {
    case 31:
        set_ip(value);
        break;
    case 29:
        set_rp0(value);
        break;
    case 30:
        set_rp1(value);
        break;
    default:
        return RegsZ8::setRegister(reg, value);
    }
    return false;
}

}  // namespace z88
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
