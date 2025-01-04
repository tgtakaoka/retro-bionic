#include "regs_im6100.h"
#include "debugger.h"
#include "inst_pdp8.h"
#include "pins_im6100.h"
#include "signals_im6100.h"

#define NELEM(a) (sizeof(a) / sizeof(a[0]))

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
    _buffer.oct12(10, _mq);
    _buffer.hex1(18, _link());
    _buffer.oct12(23, _pc);
    _buffer.bits(30, _flags, 04000, "L_R_I");
    _buffer.oct12(39, _sw);
    cli.println(_buffer);
    _pins->idle();
}

void RegsIm6100::save() {
    const uint16_t SAVE[] = {
            05323,  // JMP 7623 at 7777
            03001,  // DCA 0001
            07701,  // ACL
            03002,  // DCA 0002
            06004,  // GTF
            03003,  // DCA 0003
            05323,  // JMP 7600
    };
    uint16_t buffer[3];
    _pins->captureWrites(SAVE, NELEM(SAVE), &_pc, buffer, NELEM(buffer));
    _ac = buffer[0];
    _mq = buffer[1];
    _flags = buffer[2];

    // Pre-increment indirect
    const uint16_t TMP[] = {
            03410,  // DCA I 0010
    };
    _pins->captureWrites(TMP, NELEM(TMP), nullptr, buffer, 1);
}

void RegsIm6100::restore() {
    const uint16_t RESTORE[] = {
            07200,          // CLA
            01002, _mq,     // TAD 0002  ; Load MQ
            07501,          // MQA       ; AC->MQ
            07200,          // CLA
            01003, _flags,  // TAD 0003  ; Load FLAGS
            06005,          // RTF       ; AC->FLAGS
            07200,          // CLA
            01001, _ac,     // TAD 0001  ; Load AC
            05400, _pc,     // JMP I 0000; Jump to PC
    };
    _pins->injectReads(RESTORE, NELEM(RESTORE));
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
