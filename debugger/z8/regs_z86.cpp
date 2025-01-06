#include "regs_z86.h"
#include "char_buffer.h"
#include "debugger.h"
#include "inst_z86.h"
#include "pins_z86.h"

namespace debugger {
namespace z86 {
namespace {
// clang-format off
//                    012345678901234567890123456789012345
const char line1[] = "PC=xxxx SP=xxxx RP=xx FLAGS=CZSVDH11";
// clang-format on
}  // namespace

RegsZ86::RegsZ86(PinsZ86 *pins) : RegsZ8(pins), _buffer1(line1) {}

const char *RegsZ86::cpu() const {
    return "Z86C";
}

const char *RegsZ86::cpuName() const {
    return "Z86C91";
}

void RegsZ86::print() const {
    _buffer1.hex16(3, _pc);
    _buffer1.hex16(11, _sp);
    _buffer1.hex8(19, _rp);
    _buffer1.bits(28, _flags, 0x80, line1 + 28);
    cli.println(_buffer1);
    RegsZ8::print();
}

void RegsZ86::reset() {
    constexpr auto P3M = 247;
    constexpr auto P01M = 248;
    write_reg(P3M, 0);
    write_reg(P01M, 0x92 | ((1 - EXTERNAL_STACK) << 2));
    static constexpr uint8_t JP_RESET[] = {
            0x8D,  // JP ORG_RESET
            hi(InstZ86::ORG_RESET),
            lo(InstZ86::ORG_RESET),
    };
    _pins->execInst(JP_RESET, sizeof(JP_RESET));
}

uint8_t RegsZ86::save_rp() const {
    static constexpr uint8_t SAVE_RP[] = {
            0xA0, SPH,  // INCW SP
            0x70, RP,   // PUSH RP
    };
    uint8_t rp;
    _pins->captureWrites(SAVE_RP, sizeof(SAVE_RP), nullptr, &rp, sizeof(rp));
    return rp;
}

void RegsZ86::restore_rp(uint8_t rp) const {
    const uint8_t RESTORE_RP[] = {
            0x31, rp,  // SRP #rp
    };
    _pins->execInst(RESTORE_RP, sizeof(RESTORE_RP));
}

void RegsZ86::save_sfrs() {
    _rp = save_rp();
    restore_rp(R(0));
    const auto r0 = save_r(0);
    _flags = raw_read_reg(FLAGS);
    _sp = uint16(raw_read_reg(SPH), raw_read_reg(SPL));
    restore_r(0, r0);
    restore_rp(_rp);
}

void RegsZ86::restore_sfrs() {
    restore_rp(R(0));
    write_reg(FLAGS, _flags);
    write_reg(SPH, hi(_sp));
    write_reg(SPL, lo(_sp));
    restore_rp(_rp);
}

uint8_t RegsZ86::save_r(uint8_t num) const {
    const uint8_t SAVE_R[] = {
            0x92, uint8(num, 0),  // LDE @RR0,Rn
    };
    uint8_t val;
    _pins->captureWrites(SAVE_R, sizeof(SAVE_R), nullptr, &val, sizeof(val));
    return val;
}

void RegsZ86::restore_r(uint8_t num, uint8_t val) const {
    if (InstZ86::writeOnly(_rp, num))
        return;
    const uint8_t LOAD_R[] = {
            uint8(num, 0x0C), val,  // LD rn,#val
    };
    _pins->execInst(LOAD_R, sizeof(LOAD_R));
}

uint8_t RegsZ86::raw_read_reg(uint8_t addr) const {
    const uint8_t READ_REG[] = {
            0x08, addr,  // LD R0,addr
            0x92, 0x00,  // LDE @RR0,R0
    };
    uint8_t val;
    _pins->captureWrites(
            READ_REG, sizeof(READ_REG), nullptr, &val, sizeof(val));
    return val;
}

uint8_t RegsZ86::read_reg(uint8_t addr, RegSpace space) {
    const auto rp = save_rp();
    restore_rp(R(0));
    const auto r0 = save_r(0);
    uint8_t val = raw_read_reg(addr);
    if (addr == RP)
        val = rp;
    restore_r(0, r0);
    restore_rp(rp);
    return val;
}

void RegsZ86::write_reg(uint8_t addr, uint8_t val, RegSpace space) {
    const uint8_t WRITE_REG[] = {
            0xE6, addr, val,  // LD addr,#val
    };
    _pins->execInst(WRITE_REG, sizeof(WRITE_REG));
    update_r(addr, val);
    if (addr == RP)
        save_all_r();
}

void RegsZ86::update_r(uint8_t addr, uint8_t val) {
    const auto rp = hi4(_rp);
    if (hi4(addr) == rp)
        _r[lo4(addr)] = val;
}

void RegsZ86::set_r(uint8_t num, uint8_t val) {
    write_reg(R(num), _r[num] = val);
}

void RegsZ86::set_flags(uint8_t val) {
    write_reg(FLAGS, _flags = val);
}

void RegsZ86::set_sp(uint16_t val) {
    _sp = val;
    write_reg(SPH, hi(val));
    write_reg(SPL, lo(val));
}

void RegsZ86::set_rp(uint8_t val) {
    write_reg(RP, _rp = val);
}

void RegsZ86::helpRegisters() const {
    cli.println("?Reg: PC SP RP FLAGS R0~R15 RR0~14");
}

}  // namespace z86
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
