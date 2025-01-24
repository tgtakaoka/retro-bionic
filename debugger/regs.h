#ifndef __DEBUGGER_REGS_H__
#define __DEBUGGER_REGS_H__

#include <stdint.h>

namespace debugger {
struct Regs {
    virtual ~Regs() {}

    /** Return CPU name for Assembler/Disassembler */
    virtual const char *cpu() const = 0;
    /** Return hardware CPU name */
    virtual const char *cpuName() const;

    /** Print CPU registers */
    virtual void print() const = 0;
    /** Reset sequence */
    virtual void reset() {}
    /** Save CPU registers */
    virtual void save() = 0;
    /** Restore CPU registers */
    virtual void restore() = 0;

    /** Return address of next instruction */
    virtual uint32_t nextIp() const = 0;
    /** Set address of next instruction */
    virtual void setIp(uint32_t) {}
    /** Parse |word| and return register index */
    uint_fast8_t validRegister(const char *word, uint32_t &max) const;
    /** Print CPU register names */
    virtual void helpRegisters() const = 0;
    /**
     * Set register indexed by |reg| with |value|. Return true if next
     * instruction address has changed
     */
    virtual bool setRegister(uint_fast8_t reg, uint32_t value) { return false; }

protected:
    struct RegList {
        const char *const *head;  // pointer to array of register names
        uint8_t num;              // number of registers in this list
        uint8_t name;             // name of the |head| register
        uint32_t max;             // maximum value of registers
    };
    /** Return |n|-th |RegList|, nullptr if no more list */
    virtual const RegList *listRegisters(uint_fast8_t n) const = 0;

    static constexpr uint8_t hi4(uint8_t v) {
        return static_cast<uint8_t>(v >> 4);
    }
    static constexpr uint8_t lo4(uint8_t v) {
        return static_cast<uint8_t>(v & 0xF);
    }
    static constexpr uint8_t uint8(uint8_t v) { return v; }
    static constexpr uint8_t uint8(uint8_t hi, uint8_t lo) {
        return static_cast<uint8_t>(hi) << 4 | lo;
    }
    static constexpr uint8_t hi(uint16_t v) {
        return static_cast<uint8_t>(v >> 8);
    }
    static constexpr uint8_t lo(uint16_t v) {
        return static_cast<uint8_t>(v >> 0);
    }
    static constexpr uint8_t hi8(uint16_t v) {
        return static_cast<uint8_t>(v >> 8);
    }
    static constexpr uint8_t lo8(uint16_t v) {
        return static_cast<uint8_t>(v >> 0);
    }
    static constexpr uint16_t uint16(uint16_t v) { return v; }
    static constexpr uint16_t uint16(uint8_t hi, uint8_t lo) {
        return static_cast<uint16_t>(hi) << 8 | lo;
    }
    static constexpr uint16_t le16(const uint8_t *p) {
        return uint16(p[1], p[0]);
    }
    static constexpr uint16_t be16(const uint8_t *p) {
        return uint16(p[0], p[1]);
    }
    static void le16(uint8_t *p, uint16_t v) {
        p[0] = lo(v);
        p[1] = hi(v);
    }
    static void be16(uint8_t *p, uint16_t v) {
        p[0] = hi(v);
        p[1] = lo(v);
    }
    static constexpr uint16_t hi16(uint32_t v) {
        return static_cast<uint16_t>(v >> 16);
    }
    static constexpr uint16_t lo16(uint32_t v) {
        return static_cast<uint16_t>(v >> 0);
    }
    static constexpr uint32_t uint32(uint16_t hi, uint16_t lo) {
        return static_cast<uint16_t>(hi) << 8 | lo;
    }
    static void swap8(uint8_t &a, uint8_t &b) {
        auto tmp = a;
        a = b;
        b = tmp;
    }
};
}  // namespace debugger

#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
