#ifndef __DEBUGGER_MEMS_H__
#define __DEBUGGER_MEMS_H__

#include <stdint.h>

#include "config_debugger.h"

namespace libasm {
struct Assembler;
struct Disassembler;
}  // namespace libasm

namespace debugger {

enum Endian : uint16_t {
    ENDIAN_BIG,
    ENDIAN_LITTLE,
};

struct Mems {
    virtual ~Mems();

    /// Maximum address of program memory
    virtual uint32_t maxAddr() const = 0;
    uint8_t addressWidth() const;
    uint8_t addressUnit() const;
    uint8_t opCodeWidth() const;
    uint8_t listRadix() const;
    Endian endian() const { return _endian; }
    // True if ADDRESS_BYTE and access 16-bit at once
    virtual bool wordAccess() const { return false; }

    // Maximum address of data memory
    virtual uint32_t maxData() const { return maxAddr(); }

    // Raw memory access for ADDRESS_BYTE
    virtual uint16_t read_byte(uint32_t byte_addr) const = 0;
    virtual void write_byte(uint32_t byte_addr, uint16_t data) const = 0;

    // Raw memory access for ADDRESS_WORD
    virtual uint16_t read_word(uint32_t word_addr) const = 0;
    virtual void write_word(uint32_t word_addr, uint16_t data) const = 0;

    // Access from CPU
    virtual uint16_t read(uint32_t addr) const { return read_byte(addr); }
    virtual void write(uint32_t addr, uint16_t data) const {
        write_byte(addr, data);
    }
    // 16 bit access from CPU for ADDRESS_BYTE memory
    uint16_t read16(uint32_t byte_addr) const;
    void write16(uint32_t byte_addr, uint16_t data) const;

    // Access from debugger for program memory
    virtual uint16_t get_prog(uint32_t addr) const { return read(addr); }
    virtual void put_prog(uint32_t addr, uint16_t data) const {
        write(addr, data);
    }
    // Access from debugger for data memory
    virtual uint16_t get_data(uint32_t addr) const { return get_prog(addr); }
    virtual void put_data(uint32_t addr, uint16_t data) const {
        put_prog(addr, data);
    }

    struct RomArea {
        void set(uint32_t begin, uint32_t end);
        bool readOnly(uint32_t addr) const;
        void print() const;

    private:
        uint32_t _begin;
        uint32_t _end;
    };
    virtual RomArea *romArea() { return nullptr; }

    uint32_t assemble(uint32_t addr, const char *line) const;
    uint32_t disassemble(uint32_t addr, uint8_t numInsn) const;
    void dumpMemory(uint32_t addr, uint16_t len, bool prog = false) const;
    void writeMemory(uint32_t addr, const uint16_t *buffer, uint_fast8_t len,
            bool prog = false) const;

    uint_fast8_t get_code(uint32_t byte_addr) const;
    void put_code(uint32_t addr, const uint8_t *bytes, uint_fast8_t len) const;

protected:
    Mems(Endian endian);

    const Endian _endian;
#ifdef WITH_ASSEMBLER
    libasm::Assembler *_assembler;
    virtual libasm::Assembler *assembler() const;
#endif
#ifdef WITH_DISASSEMBLER
    libasm::Disassembler *_disassembler;
    virtual libasm::Disassembler *disassembler() const;
#endif

    virtual void isPrint(const uint8_t *data, char buf[2]) const;
    uint8_t dataColumns(uint8_t len) const;
    void dump(const uint8_t *buf, uint_fast8_t start, uint_fast8_t len) const;
    void printAddress(uint32_t addr) const;

    uint16_t raw_read16be(uint32_t byte_addr) const;
    uint16_t raw_read16le(uint32_t byte_addr) const;
    void raw_write16be(uint32_t byte_addr, uint16_t data) const;
    void raw_write16le(uint32_t byte_addr, uint16_t data) const;

    static constexpr uint8_t hi(uint16_t v) {
        return static_cast<uint8_t>(v >> 8);
    }
    static constexpr uint8_t lo(uint16_t v) {
        return static_cast<uint8_t>(v >> 0);
    }
    static constexpr uint16_t uint16(uint8_t hi, uint8_t lo) {
        return static_cast<uint16_t>(hi) << 8 | lo;
    }
    static void le16(uint8_t *p, uint16_t v) {
        p[0] = lo(v);
        p[1] = hi(v);
    }
    static void be16(uint8_t *p, uint16_t v) {
        p[0] = hi(v);
        p[1] = lo(v);
    }
};

/**
 * Emulated Memory on 128KB DMAMEM.
 */
struct DmaMemory : Mems {
    uint32_t maxAddr() const override { return MEM_SIZE - 1; }
    uint16_t read_byte(uint32_t addr) const override;
    void write_byte(uint32_t addr, uint16_t data) const override;
    uint16_t read_word(uint32_t addr) const override;
    void write_word(uint32_t addr, uint16_t data) const override;

    static constexpr uint32_t MEM_SIZE = 128U * 1024U;

protected:
    DmaMemory(Endian endian) : Mems(endian) {}
};

/**
 * Emulated Memory on 16MB EXTMEM.
 */
struct ExtMemory : Mems {
    uint32_t maxAddr() const override { return MEM_SIZE - 1; }
    uint16_t read_byte(uint32_t addr) const override;
    void write_byte(uint32_t addr, uint16_t data) const override;
    uint16_t read_word(uint32_t addr) const override;
    void write_word(uint32_t addr, uint16_t data) const override;

    static constexpr uint32_t MEM_SIZE = 16U * 1024U * 1024U;

protected:
    ExtMemory(Endian endian) : Mems(endian) {}
};

}  // namespace debugger

#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
