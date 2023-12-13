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
    virtual uint32_t maxAddr() const = 0;
    virtual uint16_t raw_read(uint32_t addr) const = 0;
    virtual void raw_write(uint32_t addr, uint16_t data) const = 0;
    uint16_t raw_read16(uint32_t addr) const {
        return _endian == ENDIAN_BIG ? raw_read16be(addr) : raw_read16le(addr);
    }
    void raw_write16(uint32_t addr, uint16_t data) const {
        if (_endian == ENDIAN_BIG)
            raw_write16be(addr, data);
        else
            raw_write16le(addr, data);
    }

    // Read and write from CPU
    virtual uint16_t read(uint32_t addr) const { return raw_read(addr); }
    virtual void write(uint32_t addr, uint16_t data) const {
        raw_write(addr, data);
    }

    // Setup and restore break point
    virtual uint16_t get_inst(uint32_t addr) const { return raw_read(addr); }
    virtual void put_inst(uint32_t addr, uint16_t data) const {
        raw_write(addr, data);
    }

    // Read and write from debugger
    virtual uint16_t get(uint32_t addr, const char *space = nullptr) const {
        (void)space;
        return read(addr);
    }
    virtual void put(
            uint32_t addr, uint16_t data, const char *space = nullptr) const {
        (void)space;
        write(addr, data);
    }
    void put(uint32_t addr, const uint8_t *buffer, uint8_t len) const;

    virtual bool hasRomArea() const { return false; }
    void setRomArea(uint32_t begin, uint32_t end);
    bool romArea(uint32_t addr) const {
        return addr >= _rom_begin && addr < _rom_end;
    }
    void printRomArea();

    uint32_t assemble(uint32_t addr, const char *line) const;
    uint32_t disassemble(uint32_t addr, uint8_t numInsn) const;

protected:
    Mems(Endian endian) : _endian(endian), _rom_begin(0), _rom_end(0) {}

    const Endian _endian;
    uint32_t _rom_begin;
    uint32_t _rom_end;

    uint16_t raw_read16be(uint32_t addr) const;
    uint16_t raw_read16le(uint32_t addr) const;
    void raw_write16be(uint32_t addr, uint16_t data) const;
    void raw_write16le(uint32_t addr, uint16_t data) const;

    virtual libasm::Assembler *assembler() const { return nullptr; }
    virtual libasm::Disassembler *disassembler() const { return nullptr; }

    static constexpr uint8_t hi(uint16_t v) {
        return static_cast<uint8_t>(v >> 8);
    }
    static constexpr uint8_t lo(uint16_t v) {
        return static_cast<uint8_t>(v >> 0);
    }
    static constexpr uint16_t uint16(uint8_t hi, uint8_t lo) {
        return static_cast<uint16_t>(hi) << 8 | lo;
    }
};

/**
 * Emulated Memory on 128KB DMAMEM.
 */
struct DmaMemory : Mems {
    uint32_t maxAddr() const override { return MEM_SIZE - 1; }
    uint16_t raw_read(uint32_t addr) const override;
    void raw_write(uint32_t addr, uint16_t data) const override;

    static constexpr uint32_t MEM_SIZE = 128U * 1024U;

protected:
    DmaMemory(Endian endian) : Mems(endian) {}
};

/**
 * Emulated Memory on 16MB EXTMEM.
 */
struct ExtMemory : Mems {
    uint32_t maxAddr() const override { return MEM_SIZE - 1; }
    uint16_t raw_read(uint32_t addr) const override;
    void raw_write(uint32_t addr, uint16_t data) const override;

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
