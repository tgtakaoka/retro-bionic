#ifndef __MEMS_Z8000_H__
#define __MEMS_Z8000_H__

#include "mems.h"

namespace debugger {
namespace z8000 {

struct RegsZ8000;

// Big-endian; the even byte rides AD8-AD15. Z8001 addresses are
// seg<<16|off.
struct MemsZ8000 : ExtMemory {
    // A memory segment is the high half of an EXTMEM address,
    // segment<<16 | offset. A Z8001 cycle carries its own segment in the
    // address; a Z8002 has none, so for split instruction/data it
    // synthesizes three: the system (kernel and a merged program), the
    // running program's code, and its data, each a 64K segment.
    static constexpr uint32_t SEG_SIZE = 0x10000;
    enum Seg : uint8_t { SYS = 0, CODE = 1, DATA = 2, SEGS = 3 };
    // A Z8001 has real segments; with routing on, a program in segment
    // ISEG fetches from ISPACE and reads and writes data in ISEG, as on
    // the Olivetti M20, the machine DRI's CP/M-8000 was released for.
    static constexpr uint8_t ISEG = 0x08;
    static constexpr uint8_t ISPACE = 0x0B;

    MemsZ8000(bool segmented);
    void setRegs(const RegsZ8000 *regs) { _regs = regs; }

    uint32_t maxAddr() const override { return _maxAddr; }

    // The debugger's m/L/U and the disk see the system segment.
    uint16_t read(uint32_t addr) const override { return read16(addr & ~1); }
    void write(uint32_t addr, uint16_t data) const override {
        write16(addr & ~1, data);
    }

    // Turns the split routing on or off; off (the reset state) leaves
    // every access where the bus puts it, so main's samples are unchanged.
    void setSplit(bool on) { _split = on; }
    bool split() const { return _split; }

    // A byte in a synthesized segment, for the split device's copy.
    uint16_t readByteSeg(uint32_t addr, uint8_t seg) const {
        return read_byte(seg * SEG_SIZE + addr);
    }
    void writeByteSeg(uint32_t addr, uint16_t data, uint8_t seg) const {
        write_byte(seg * SEG_SIZE + addr, data);
    }

    // How a Z8001 instruction is disassembled: in the mode the FCW now
    // says, or, for a backtrace line, the mode it ran in.
    enum Decode : uint8_t { DECODE_FCW = 0, DECODE_NONSEG, DECODE_SEG };
    void setDecode(Decode mode) const { _decode = mode; }
#ifdef WITH_DISASSEMBLER
    // The words of the instruction at |addr| decoded in |mode|, 0 if it
    // does not decode.
    uint_fast8_t insnWords(uint32_t addr, Decode mode) const;
    // Nonsegmented code outside segment 0 decodes at its offsets.
    uint32_t disassemble(uint32_t addr, uint8_t numInsn) const override;
#endif

    // The disk cache sits above the three synthesized segments, not in them.
    Area cacheArea() const override {
        return _segmented ? ExtMemory::cacheArea()
                          : extArea(SEGS * SEG_SIZE, MEM_SIZE);
    }

protected:
#ifdef WITH_ASSEMBLER
    libasm::Assembler *assembler() const override;
#endif
#ifdef WITH_DISASSEMBLER
    libasm::Disassembler *disassembler() const override;
#endif

private:
    const bool _segmented;
    const uint32_t _maxAddr;
    bool _split = false;
    const RegsZ8000 *_regs = nullptr;
    mutable Decode _decode = DECODE_FCW;
    // Z8002 code: a Z8001 program with SEG clear in its FCW, or a
    // backtrace line that ran so.
    bool nonsegmented() const;
};

}  // namespace z8000
}  // namespace debugger
#endif /* __MEMS_Z8000_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
