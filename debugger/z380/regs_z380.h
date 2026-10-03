#ifndef __REGS_Z380_H__
#define __REGS_Z380_H__

#include "char_buffer.h"
#include "regs.h"

namespace debugger {
namespace z380 {

struct PinsZ380;
struct MemsZ380;

struct RegsZ380 final : Regs {
    RegsZ380(PinsZ380 *pins, MemsZ380 *mems);

    const char *cpu() const override { return cpuName(); }

    void print() const override;
    // The state reset leaves (User's Manual Table 7), applied on resume.
    void reset() override;
    void save() override;
    void restore() override;

    uint32_t nextIp() const override { return _pc; }
    void setIp(uint32_t addr) override { park(addr); }
    // Parked at the fetch of the program's own |pc|.
    void park(uint32_t pc) {
        _pc = pc;
        _parkedAddr = pc;
        _vector = NONE;
    }
    // How the CPU was stopped: at the fetch of a vector, with the
    // program's PC pushed at |frame|.
    enum Vector : uint8_t {
        NONE,  // parked at the program's own fetch (after reset)
        RST,   // RST 38H: the return address pushed
        NMI,   // NMI: PC pushed, IEF1 kept in IEF2 for RETN
    };
    // A pushed PC as the bus showed it: where, and what.
    struct Frame {
        bool valid;
        uint32_t pc;
        uint32_t addr;
    };
    // Whether the push was one word or two depends on a mode the program
    // may have changed, so both readings are kept until save() has read
    // the SR.
    void parkInVector(Vector how, uint32_t vecAddr, const Frame &native,
            const Frame &extended) {
        _vector = how;
        _vecAddr = vecAddr;
        _frames[0] = native;
        _frames[1] = extended;
        selectFrame();
    }
    uint32_t pc() const { return _pc; }
    // Changes with any register the program can change.
    uint32_t fingerprint() const;
    // Where the CPU is parked, as the bus showed it, which is where
    // injection resumes.
    uint32_t parkedAt() const {
        return _vector != NONE ? _vecAddr : _parkedAddr;
    }
    // Extended mode: 32-bit PC and SP arithmetic, and pushes of 4 bytes.
    bool extended() const { return (_sr & SR_XM) != 0; }
    // Long Word mode: register pairs move 4 bytes through memory.
    bool longWord() const { return (_sr & SR_LW) != 0; }
    // The bytes a CALL, RST or NMI pushes.
    uint_fast8_t pcSize() const { return extended() ? 4 : 2; }

    void helpRegisters() const override;
    const RegList *listRegisters(uint_fast8_t n) const override;
    bool setRegister(uint_fast8_t reg, uint32_t value) override;

    // Select Register (User's Manual 5.3).
    static constexpr uint32_t SR_XM = 0x80;
    static constexpr uint32_t SR_LW = 0x40;
    // XM, IEF1, IM and LCK: restore() cannot load these.
    static constexpr uint32_t SR_FIXED = 0xBA;

private:
    PinsZ380 *const _pins;
    MemsZ380 *const _mems;

    uint32_t _pc;
    uint32_t _parkedAddr;  // the fetch the CPU is parked in, outside a vector
    Vector _vector = NONE;
    uint32_t _frameAddr;  // the pushed PC
    Frame _frames[2];     // as a Native and as an Extended mode push
    uint32_t _vecAddr;    // the vector fetch the CPU is parked in
    uint32_t _sp;
    uint32_t _sr;     // as shown and edited: its selections apply on resume
    uint32_t _srCpu;  // as the CPU holds it, read by the last save()
    uint32_t _i;      // with its extension Iz
    uint8_t _r;
    // Every register bank, each register by its physical side: [0] the
    // primary, [1] the alternate. The SR's selections pick which of them
    // the program sees as A, BC, IX and so on.
    struct Bank {
        uint8_t a[2];
        uint8_t f[2];
        uint32_t bc[2];
        uint32_t de[2];
        uint32_t hl[2];
    } _bank[4];
    uint32_t _ix[4][2];
    uint32_t _iy[4][2];

    // The SR's register selections.
    static uint_fast8_t mainBank(uint32_t sr) { return (sr >> 9) & 3; }
    static uint_fast8_t alt(uint32_t sr) { return (sr >> 8) & 1; }
    static uint_fast8_t afp(uint32_t sr) { return sr & 1; }
    static uint_fast8_t ixBank(uint32_t sr) { return (sr >> 17) & 3; }
    static uint_fast8_t ixp(uint32_t sr) { return (sr >> 16) & 1; }
    static uint_fast8_t iyBank(uint32_t sr) { return (sr >> 25) & 3; }
    static uint_fast8_t iyp(uint32_t sr) { return (sr >> 24) & 1; }
    // What the program sees through the selections in _sr.
    Bank &live() { return _bank[mainBank(_sr)]; }
    const Bank &live() const { return _bank[mainBank(_sr)]; }
    uint32_t &ix(uint_fast8_t prime = 0) {
        return _ix[ixBank(_sr)][ixp(_sr) ^ prime];
    }
    uint32_t &iy(uint_fast8_t prime = 0) {
        return _iy[iyBank(_sr)][iyp(_sr) ^ prime];
    }
    uint32_t ix(uint_fast8_t prime = 0) const {
        return _ix[ixBank(_sr)][ixp(_sr) ^ prime];
    }
    uint32_t iy(uint_fast8_t prime = 0) const {
        return _iy[iyBank(_sr)][iyp(_sr) ^ prime];
    }

    void saveBank(uint_fast8_t bank, uint32_t &org);
    void saveIndex(uint32_t (&reg)[4][2], uint8_t prefix, uint32_t &org);
    void restoreBank(uint_fast8_t bank, uint32_t &org) const;
    void restoreIndex(
            const uint32_t (&reg)[4][2], uint8_t prefix, uint32_t &org) const;
    void updateMode() const;
    void selectFrame();
    // SP moved by |delta|: Native mode wraps it within 64K.
    uint32_t spPlus(uint32_t sp, int32_t delta) const {
        return extended() ? sp + delta
                          : (sp & ~UINT32_C(0xFFFF)) | ((sp + delta) & 0xFFFF);
    }
    uint_fast8_t jumpTo(uint8_t *buf, uint32_t addr) const;

    mutable CharBuffer _buffer1;
    mutable CharBuffer _buffer2;
    mutable CharBuffer _buffer3;
    mutable CharBuffer _buffer4;
};

}  // namespace z380
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
