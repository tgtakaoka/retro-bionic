#include "regs_hd6120.h"
#include "debugger.h"
#include "inst_pdp8.h"
#include "pins_hd6120.h"
#include "signals_hd6120.h"

namespace debugger {
namespace hd6120 {
namespace {
// clang-format off
//                             1         2         3         4         5         6
//                   0123456789012345678901234567890123456789012345678901234567890123456
const char line[] = "AC=XXXX MQ=XXXX L=X PC=XXXXX DF=X F=LGRPI SP1=XXXX SP2=XXXX SW=XXXX";
// clang-format on
}  // namespace

RegsHd6120::RegsHd6120(PinsHd6120 *pins) : RegsPdp8(pins), _buffer(line) {}

const char *RegsHd6120::cpu() const {
    return "6120";
}

void RegsHd6120::print() const {
    _buffer.oct12(3, _ac);
    _buffer.oct12(11, _mq);
    _buffer.hex1(18, _link());
    _buffer.oct3(23, _if());
    _buffer.oct12(24, _pc);
    _buffer.oct3(32, _df());
    _buffer.bits(36, _flags, link, "LGRPI");
    _buffer.oct12(46, _sp1);
    _buffer.oct12(55, _sp2);
    _buffer.oct12(63, _sw);
    cli.println(_buffer);
    _pins->idle();
}

void RegsHd6120::save() {
    static const uint16_t SAVE[] = {
            04322,  // JMS 7722; capture PC
            03001,  // DCA 0001; capture AC
            07701,  // ACL     ; MQ->AC
            03002,  // DCA 0002; capture MQ
            06256,  // GCF     ; FLAGS->AC
            03003,  // DCA 0003; capture FLAGS
            06207,  // RSP1    ; SP1->AC
            03004,  // DCA 0004; capture SP1
            06227,  // RSP2    ; SP2->AC
            03005,  // DCA 0005; capture SP2
    };
    uint16_t buffer[6];
    _pins->captureWrites(SAVE, length(SAVE), buffer, length(buffer));
    _pc = (buffer[0] - 1) & 07777;  // offset JMS instruction
    _ac = buffer[1];
    _mq = buffer[2];
    _flags = buffer[3];
    _sp1 = buffer[4];
    _sp2 = buffer[5];
}

void RegsHd6120::restore() {
    const uint16_t RESTORE[] = {
            07200,          // CLA
            01004, _sp1,    // TAD 0004  ; Load _sp1
            06217,          // LSP1      ; AC->SP1, AC=0
            01005, _sp2,    // TAD 0005  ; Load _sp2
            06237,          // LSP2      ; AC->SP2, AC=0
            01002, _mq,     // TAD 0002  ; Load _mq2
            07421,          // MQL       ; AC->MQ
            01003, _flags,  // TAD 0003  ; Load _flags
            06005,          // RTF       ; AC->FLAGS
            01001, _ac,     // TAD 0001  ; Load _ac
            05400, _pc,     // JMP I 0000; Jump to _pc
    };
    _pins->injectReads(RESTORE, length(RESTORE));
}

void RegsHd6120::breakPoint() {
    static const uint16_t EXIT_CP[] = {
            05323,  // JMP 7723  ; Control panel vector
            06003,  // PG0       ; Reset HLTFLG
            06004,  // PEX       ; Exit from Panel mode after next instruction
            05323,  // JMP 7723
    };
    uint16_t buffer[1];
    _pins->captureWrites(EXIT_CP, length(EXIT_CP), buffer, length(buffer));
    save();
    _pc = buffer[0] - 1;  // offset break point HLT
}

void RegsHd6120::helpRegisters() const {
    cli.println("?Reg: AC PC MQ L IF DF GT IEFF SP1 SP2");
}

constexpr const char *REGS12[] = {
        "SP1",  // REG_SP1
        "SP2",  // REG_SP2
};
constexpr const char *REGS1[] = {
        "GT",  // REG_GR
};
constexpr const char *REGS3[] = {
        "IF",  // REG_IF
        "DF",  // REG_DF
};

const Regs::RegList *RegsHd6120::listRegisters(uint_fast8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS12, 2, REG_SP1, 07777},
            {REGS1, 1, REG_GT, 1},
            {REGS3, 2, REG_IF, 7},
    };
    if (n < 2)
        return RegsPdp8::listRegisters(n);
    return n < 5 ? &REG_LIST[n - 2] : nullptr;
}

bool RegsHd6120::setRegister(uint_fast8_t reg, uint32_t value) {
    switch (reg) {
    case REG_GT:
        set_gt(value);
        break;
    case REG_IF:
        set_if(value);
        break;
    case REG_DF:
        set_df(value);
        break;
    case REG_SP1:
        _sp1 = value;
        break;
    case REG_SP2:
        _sp2 = value;
        break;
    default:
        return RegsPdp8::setRegister(reg, value);
    }
    return false;
}

}  // namespace hd6120
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
