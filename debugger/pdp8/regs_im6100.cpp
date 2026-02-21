#include "regs_im6100.h"
#include "debugger.h"
#include "pins_im6100.h"

namespace debugger {
namespace im6100 {
namespace {
//                             1         2         3         4
//                   0123456789012345678901234567890123456789012
const char line[] = "AC=XXXX MQ=XXXX L=X PC=XXXX F=L_R_I SW=XXXX";
}  // namespace

RegsIm6100::RegsIm6100(PinsIm6100 *pins) : RegsPdp8(pins), _buffer(line) {}

const char *RegsIm6100::cpu() const {
    return "6100";
}

void RegsIm6100::print() const {
    _buffer.oct12(3, _ac);
    _buffer.oct12(11, _mq);
    _buffer.hex1(18, _link());
    _buffer.oct12(23, _pc);
    _buffer.bits(30, _flags, 04000, "L_R_I");
    _buffer.oct12(39, _sw);
    cli.println(_buffer);
    _pins->idle();
}

void RegsIm6100::save() {
    static const uint16_t SAVE[] = {
            04322, 07000,  // JMS 7722; dummy read, capture PC
            03001, 07000,  // DCA 0001; dummy read, capture AC
            07701,         // ACL     ; MQ->AC
            03002, 07000,  // DCA 0002; dummy read, capture MQ
            06004, 07000,  // GTF     ; FLAGS->AC, dummy read, dummy write
            03003, 07000,  // DCA 0003; dummy read capture FLAGS
    };
    uint16_t buffer[5];
    _pins->captureWrites(SAVE, length(SAVE), buffer, length(buffer));
    _pc = (buffer[0] - 1) & 07777;  // offset JMS instruction
    _ac = buffer[1];
    _mq = buffer[2];
    _flags = buffer[4];
}

void RegsIm6100::restore() {
    const uint16_t RESTORE[] = {
            07200,          // CLA
            01002, _mq,     // TAD 0002  ; Load MQ
            07421,          // MQL       ; AC->MQ
            01003, _flags,  // TAD 0003  ; Load FLAGS
            06005, 07000,   // RTF       ; AC->FLAGS, dummy read, dummy write
            07200,          // CLA
            01001, _ac,     // TAD 0001  ; Load AC
            05400, _pc,     // JMP I 0000; Jump to PC
    };
    _pins->injectReads(RESTORE, length(RESTORE));
}

void RegsIm6100::breakPoint() {
    static const uint16_t EXIT_CP[] = {
            05323,  // JMP 7723  ; Control panel vector
            06001,  // ION       ; Exit from Panel mode after next instruction
            05323,  // JMP 7723
    };
    uint16_t buffer[1];
    _pins->captureWrites(EXIT_CP, length(EXIT_CP), buffer, length(buffer));
    save();
    _pc = buffer[0] - 1;  // offset break point HLT
}

void RegsIm6100::helpRegisters() const {
    cli.println("?Reg: AC PC MQ LINK");
}

}  // namespace im6100
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
