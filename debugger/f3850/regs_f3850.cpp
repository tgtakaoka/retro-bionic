#include "regs_f3850.h"
#include "debugger.h"
#include "pins_f3850.h"

#define LOG_ROMC(e)
// #define LOG_ROMC(e) e

namespace debugger {
namespace f3850 {

namespace {
// clang-format off
//                              1         2         3         4
//                    01234567890123456789012345678901234567890123456789
const char line1[] = "P0=xxxx P=xxxx DC=xxxx DC1=xxxx IS=xx W=IVZCS A=xx";
const char line2[] = "R0=xx R1=xx R2=xx R3=xx R4=xx R5=xx R6=xx R7=xx";
const char line3[] = "R8=xx  J=xx HU=xx HL=xx KL=xx KH=xx QU=xx QH=xx";
// clang-format on
}  // namespace

RegsF3850::RegsF3850(PinsF3850 *pins, Devs *devs)
    : _pins(pins),
      _devs(devs),
      _buffer1(line1),
      _buffer2(line2),
      _buffer3(line3) {}

const char *RegsF3850::cpu() const {
    return "F3850";
}

bool RegsF3850::romc_read(Signals *s) {
    LOG_ROMC(cli.print("@@ R ROMC="));
    LOG_ROMC(cli.printHex(s->romc, 2));
    LOG_ROMC(cli.print(" pc0="));
    LOG_ROMC(cli.printHex(_pc0, 4));
    LOG_ROMC(cli.print(" dc0="));
    LOG_ROMC(cli.printlnHex(_dc0, 4));
    switch (s->romc()) {
    case 0x00:
        s->markFetch();
        if (s->readMemory())
            s->data = _mems->read(_pc0);
        s->markRead(_pc0++);
        break;
    case 0x01:
        if (s->readMemory())
            s->data = _mems->read(_pc0);
        s->markRead(_pc0);
        _pc0 += static_cast<int8_t>(s->data);
        break;
    case 0x02:
        if (s->readMemory())
            s->data = _mems->read(_dc0);
        s->markRead(_dc0++);
        break;
    case 0x03:
        if (s->readMemory())
            s->data = _mems->read(_pc0);
        s->markRead(_pc0++);
        _ioaddr = s->data;
        break;
    case 0x06:
        s->data = hi(_dc0);
        break;
    case 0x07:
        s->data = hi(_pc1);
        break;
    case 0x09:
        s->data = lo(_dc0);
        break;
    case 0x0B:
        s->data = lo(_pc1);
        break;
    case 0x0C:
        if (s->readMemory())
            s->data = _mems->read(_pc0);
        s->markRead(_pc0);
        _pc0 = uint16(hi(_pc0), s->data);
        break;
    case 0x0E:
        if (s->readMemory())
            s->data = _mems->read(_pc0);
        s->markRead(_pc0);
        _dc0 = uint16(hi(_dc0), s->data);
        break;
    case 0x0F:  // Interruput acknowledge
        s->data = lo(_devs->vector());
        _pc1 = _pc0;
        _pc0 = uint16(hi(_pc0), s->data);
        break;
    case 0x11:
        if (s->readMemory())
            s->data = _mems->read(_pc0);
        s->markRead(_pc0);
        _dc0 = uint16(s->data, lo(_dc0));
        break;
    case 0x13:  // Interruput acknowledge
        s->data = hi(_devs->vector());
        _pc0 = uint16(s->data, lo(_pc0));
        break;
    case 0x1B:  // I/O read
        if (_ioaddr < 4)
            return false;
        if (_devs->isSelected(_ioaddr)) {
            s->data = _devs->read(_ioaddr);
        } else {
            s->data = 0;
        }
        s->markIoRead(_ioaddr);
        break;
    case 0x1E:
        s->data = lo(_pc0);
        break;
    case 0x1F:
        s->data = hi(_pc0);
        break;
    default:
        s->data = 0;
        return false;
    }
    return true;
}

bool RegsF3850::romc_write(Signals *s) {
    LOG_ROMC(cli.print("@@ W ROMC="));
    LOG_ROMC(cli.printHex(s->romc, 2));
    LOG_ROMC(cli.print(" pc0="));
    LOG_ROMC(cli.printHex(_pc0, 4));
    LOG_ROMC(cli.print(" dc0="));
    LOG_ROMC(cli.printlnHex(_dc0, 4));
    switch (s->romc()) {
    case 0x04:
        _pc0 = _pc1;
        break;
    case 0x05:
        if (s->writeMemory())
            _mems->write(_dc0, s->data);
        s->markWrite(_dc0++);
        break;
    case 0x08:
        _pc1 = _pc0;
        _pc0 = uint16(s->data, s->data);
        break;
    case 0x0A:
        _dc0 += static_cast<int8_t>(s->data);
        break;
    case 0x0D:
        _pc1 = _pc0 + 1;
        return true;
    case 0x12:
        _pc1 = _pc0;
        _pc0 = uint16(hi(_pc0), s->data);
        break;
    case 0x14:
        _pc0 = uint16(s->data, lo(_pc0));
        break;
    case 0x15:
        _pc1 = uint16(s->data, lo(_pc1));
        break;
    case 0x16:
        _dc0 = uint16(s->data, lo(_dc0));
        break;
    case 0x17:
        _pc0 = uint16(hi(_pc0), s->data);
        break;
    case 0x18:
        _pc1 = uint16(hi(_pc1), s->data);
        break;
    case 0x19:
        _dc0 = uint16(hi(_dc0), s->data);
        break;
    case 0x1A:  // I/O write
        if (_ioaddr < 4)
            break;
        if (_devs->isSelected(_ioaddr))
            _devs->write(_ioaddr, s->data);
        s->markIoWrite(_ioaddr);
        break;
    case 0x1C:
        _ioaddr = s->data;
        return true;
    case 0x1D: {
        const auto tmp = _dc0;
        _dc0 = _dc1;
        _dc1 = tmp;
        break;
    }
    default:
        return true;
    }
    return false;
}

void RegsF3850::print() const {
    _buffer1.hex16(3, _pc0);
    _buffer1.hex16(10, _pc1);
    _buffer1.hex16(18, _dc0);
    _buffer1.hex16(27, _dc1);
    _buffer1.hex4(35, (_isar >> 3) & 7);
    _buffer1.hex4(36, _isar & 7);
    _buffer1.bits(40, _w, 0x10, line1 + 40);
    _buffer1.hex8(48, _a);
    cli.println(_buffer1);
    for (auto n = 0; n < 8; ++n)
        _buffer2.hex8(n * 6 + 3, _r[n]);
    cli.println(_buffer2);
    for (auto n = 0; n < 8; ++n)
        _buffer3.hex8(n * 6 + 3, _r[n + 8]);
    cli.println(_buffer3);
}

void RegsF3850::save() {
    static constexpr uint8_t SAVE_A[] = {
            0x17,  // ST ; (DC)<-A
    };
    static constexpr uint8_t SAVE_ISAR[] = {
            0x0A,  // LR A,IS
            0x17,  // ST
    };
    static constexpr uint8_t SAVE_REGS[] = {
            0x40, 0x17,  // LR A,0; ST
            0x41, 0x17,  // LR A,1; ST
            0x42, 0x17,  // LR A,2; ST
            0x43, 0x17,  // LR A,3; ST
            0x44, 0x17,  // LR A,4; ST
            0x45, 0x17,  // LR A,5; ST
            0x46, 0x17,  // LR A,6; ST
            0x47, 0x17,  // LR A,7; ST
            0x48, 0x17,  // LR A,8; ST
            0x49, 0x17,  // LR A,J; ST
            0x4A, 0x17,  // LR A,HU; ST
            0x4B, 0x17,  // LR A,HL; ST
            0x00, 0x17,  // LR A,KU; ST
            0x01, 0x17,  // LR A,KL; ST
            0x02, 0x17,  // LR A,QU; ST
            0x03, 0x17,  // LR A,QL; ST
    };
    static constexpr uint8_t SAVE_W[] = {
            0x1E,  // LR J,W
            0x49,  // LR A,J
            0x17,  // ST
    };

    const auto pc = _pc0;  // save PC0
    const auto dc = _dc0;  // save DC0
    _pins->captureWrites(SAVE_A, sizeof(SAVE_A), &_a, sizeof(_a));
    _pins->captureWrites(SAVE_ISAR, sizeof(SAVE_ISAR), &_isar, sizeof(_isar));
    _pins->captureWrites(SAVE_REGS, sizeof(SAVE_REGS), _r, sizeof(_r));
    _pins->captureWrites(SAVE_W, sizeof(SAVE_W), &_w, sizeof(_w));
    _pc0 = pc;  // restore PC0
    _dc0 = dc;  // restore DC
}

void RegsF3850::restore() {
    const uint8_t LOAD_ALL[] = {
            0x20, _w, 0x59, 0x1D,  // LI w;  LR J,A; LR W,J
            0x20, _r[9], 0x59,     // LI j;  LR J,A
            uint8(0x60 | isu()),   // LISU isu
            uint8(0x68 | isl()),   // LISL isl
            0x20, _a,              // LI a
    };

    const auto pc = _pc0;  // save PC0
    _pins->execInst(LOAD_ALL, sizeof(LOAD_ALL));
    _pc0 = pc;  // restire PC0
}

void RegsF3850::set_isl(uint8_t val) {
    _isar &= ~07;
    _isar |= val;
}

void RegsF3850::set_isu(uint8_t val) {
    _isar &= ~070;
    _isar |= (val << 3);
}

void RegsF3850::update_r(uint8_t num, uint8_t val) {
    _r[num] = val;
    write_reg(num, val);
}

uint8_t RegsF3850::read_reg(uint8_t addr) {
    const auto isu = (addr >> 3) & 7;
    const auto isl = addr & 7;
    const uint8_t READ_REG[] = {
            uint8(0x60 | isu),  // LISU isu
            uint8(0x68 | isl),  // LISL isl
            0x4C,               // LD A,S
            0x17,               // ST
    };

    uint8_t val;
    const auto pc = _pc0;  // save PC0
    const auto dc = _dc0;  // save DC0
    _pins->captureWrites(READ_REG, sizeof(READ_REG), &val, sizeof(val));
    _pc0 = pc;
    _dc0 = dc;
    return val;
}

void RegsF3850::write_reg(uint8_t addr, uint8_t val) {
    const auto isu = (addr >> 3) & 7;
    const auto isl = addr & 7;
    const uint8_t WRITE_REG[] = {
            uint8(0x60 | isu),  // LISU isu
            uint8(0x68 | isl),  // LISL isl
            0x20, val,          // LI val
            0x5C,               // LD S,A
    };
    const auto pc = _pc0;  // save PC0
    _pins->execInst(WRITE_REG, sizeof(WRITE_REG));
    _pc0 = pc;
}

uint8_t RegsF3850::read_io(uint8_t addr) {
    if (addr < 2) {
        const uint8_t READ_IO[] = {
                uint8(0xA0 | addr), 0x17,  // INS p; ST
        };
        const auto pc = _pc0;  // save PC0
        const auto dc = _dc0;  // save DC0
        uint8_t val;
        _pins->captureWrites(READ_IO, sizeof(READ_IO), &val, sizeof(val));
        _pc0 = pc;
        _dc0 = dc;
        return val;
    }
    if (_devs->isSelected(addr))
        return _devs->read(addr);
    return 0;
}

void RegsF3850::write_io(uint8_t addr, uint8_t val) {
    if (addr < 2) {
        const uint8_t WRITE_IO[] = {
                0x20, val,           // LI d
                uint8(0xB0 | addr),  // OUTS addr
        };
        const auto pc = _pc0;  // save PC0
        _pins->execInst(WRITE_IO, sizeof(WRITE_IO));
        _pc0 = pc;
        return;
    }
    if (_devs->isSelected(addr))
        _devs->write(addr, val);
}

void RegsF3850::helpRegisters() const {
    cli.println("?Reg: P0/PC P1/P DC DC1 H K Q IS W A R0-R8 J {H,K,Q,IS}{U,L}");
}

constexpr const char *REGS3[] = {
        "ISU",  // 1
        "ISL",  // 2
};
constexpr const char *REGS6[] = {
        "IS",  // 3
        "W",   // 4
};
constexpr const char *REGS8[] = {
        "A",   // 5
        "R0",  // 6
        "R1",  // 7
        "R2",  // 8
        "R3",  // 9
        "R4",  // 10
        "R5",  // 11
        "R6",  // 12
        "R7",  // 13
        "R8",  // 14
        "J",   // 15, R9
        "HU",  // 16, R10
        "HL",  // 17, R11
        "KU",  // 18, R12
        "KL",  // 19, R13
        "QU",  // 10, R14
        "QL",  // 21, R15
};
constexpr const char *REGS16[] = {
        "P0",   // 22
        "PC",   // 23
        "P1",   // 24
        "P",    // 25
        "DC",   // 26
        "DC1",  // 27
        "H",    // 28
        "K",    // 29
        "Q",    // 30
};

const Regs::RegList *RegsF3850::listRegisters(uint8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS3, 2, 1, 0x7},
            {REGS6, 2, 3, 0x1F},
            {REGS8, 17, 5, UINT8_MAX},
            {REGS16, 9, 22, UINT16_MAX},
    };
    return n < 4 ? &REG_LIST[n] : nullptr;
}

void RegsF3850::setRegister(uint8_t reg, uint32_t value) {
    switch (reg) {
    case 22:
    case 23:
        _pc0 = value;
        break;
    case 24:
    case 25:
        _pc1 = value;
        break;
    case 26:
        _dc0 = value;
        break;
    case 27:
        _dc1 = value;
        break;
    case 28:
        update_r(HU, hi(value));
        update_r(HL, lo(value));
        break;
    case 29:
        update_r(KU, hi(value));
        update_r(KL, lo(value));
        break;
    case 30:
        update_r(QU, hi(value));
        update_r(QL, lo(value));
        break;
    case 5:
        _a = value;
        break;
    case 4:
        _w = value;
        break;
    case 3:
        _isar = value;
        break;
    case 1:
        set_isu(value);
        break;
    case 2:
        set_isl(value);
        break;
    default:
        update_r(reg - 6U, value);
        break;
    }
}

}  // namespace f3850
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
