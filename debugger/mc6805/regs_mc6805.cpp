#include "regs_mc6805.h"
#include "debugger.h"
#include "mems_mc6805.h"
#include "pins_mc6805.h"

namespace debugger {
namespace mc6805 {
namespace {
//                   0123456789012345678901234567890123
const char line[] = "PC=xxxx SP=xxxx X=xx A=xx CC=HINZC";
}  // namespace

RegsMc6805::RegsMc6805(const char *cpu, PinsMc6805 *pins)
    : _cpu(cpu), _pins(pins), _buffer(line) {}

void RegsMc6805::print() const {
    _buffer.hex16(3, _pc);
    _buffer.hex16(11, _sp);
    _buffer.hex8(18, _x);
    _buffer.hex8(23, _a);
    _buffer.bits(29, _cc, 0x10, line + 29);
    cli.println(_buffer);
}

void RegsMc6805::save() {
    const uint8_t SWI = 0x83;  // 1:N:w:W:W:W:W:V:v:A
    _pins->injectReads(&SWI, sizeof(SWI));
    uint8_t context[5];
    _pins->captureWrites(context, sizeof(context), &_sp);
    // Capturing writes to stack in little endian order.
    _pc = le16(context) - 1;  //  offset SWI instruction.
    _x = context[2];
    _a = context[3];
    _cc = context[4];
    context[0] = hi(0x1000);
    context[1] = lo(0x1000);
    _pins->injectReads(context, 2);
    _pins->suspend();
}

void RegsMc6805::restore() {
    // Store registers into stack on internal RAM.
    _sp -= 4;
    internal_write(_sp++, _cc);
    internal_write(_sp++, _a);
    internal_write(_sp++, _x);
    internal_write(_sp++, hi(_pc));
    internal_write(_sp, lo(_pc));
    // Restore registers
    const uint8_t RTI = 0x80;  // 1:N:n:n:n:n:n:n:N
    _pins->injectReads(&RTI, sizeof(RTI));
    _pins->suspend();
}

bool RegsMc6805::saveContext(const Signals *frame) {
    // Machine context were pushed in the following order; PCL, PCH, X, A, CC
    const auto pcl = frame;
    const auto pch = frame->next();
    if (pcl->write() && pcl->write()) {
        _pc = uint16(pch->data, pcl->data);
        _sp = frame->addr;
        const auto x = frame->next(2);
        const auto a = frame->next(3);
        const auto cc = frame->next(4);
        if (x->write() && a->write() && cc->write()) {
            _x = x->data;
            _a = a->data;
            _cc = cc->data;
            return true;
        }
    }
    return false;
}

void RegsMc6805::helpRegisters() const {
    cli.println("?Reg: PC X A CC");
}

constexpr const char *REGS8[] = {
        "A",   // 1
        "X",   // 2
        "CC",  // 3
};

constexpr const char *REGS16[] = {
        "PC",  // 4
};

const Regs::RegList *RegsMc6805::listRegisters(uint_fast8_t n) const {
    static constexpr RegList REG_8{REGS8, 3, 1, UINT8_MAX};
    static RegList REG_16{REGS16, 1, 4, UINT16_MAX};
    return n == 0 ? &REG_8 : (n == 1 ? &REG_16 : nullptr);
}

bool RegsMc6805::setRegister(uint_fast8_t reg, uint32_t value) {
    switch (reg) {
    case 4:
        _pc = value;
        return true;
    case 2:
        _x = value;
        break;
    case 1:
        _a = value;
        break;
    case 3:
        _cc = value;
        break;
    }
    return false;
}

uint8_t RegsMc6805::internal_read(uint8_t addr) const {
    uint8_t LDA[] = {
            0xB6, uint8(addr),  // LDA addr ; 1:2:D
    };
    _pins->injectReads(LDA, sizeof(LDA), sizeof(LDA));
    _pins->suspend();
    static constexpr uint8_t STA_0F00[] = {
            0xC7, 0x0F, 0x00,  // STA $0F00 ; 1:2:3:n:W (MC146805)
                               //           ; 1:2:3:R:W (MC68HC05)
    };
    _pins->injectReads(STA_0F00, sizeof(STA_0F00));
    uint8_t data;
    _pins->captureWrites(&data, sizeof(data));
    _pins->suspend();
    return data;
}

void RegsMc6805::internal_write(uint8_t addr, uint8_t data) const {
    uint8_t LDA_STA[] = {
            0xA6, data,  // LDA #val ; 1:2
            0xB7, addr,  // STA addr ; 1:2:n:E (MC146805)
    };  //          ; 1:2:D:E (MC68HC05)
    _pins->injectReads(LDA_STA, sizeof(LDA_STA));
    _pins->suspend();
}

}  // namespace mc6805
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
