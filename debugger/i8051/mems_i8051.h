#ifndef __MEMS_I8051_H__
#define __MEMS_I8051_H__

#include "mems.h"

namespace debugger {
namespace i8051 {

struct RegsI8051;

struct ProgI8051 final : DmaMemory {
    ProgI8051() : DmaMemory(Endian::ENDIAN_BIG) {}

    uint32_t maxAddr() const override { return UINT16_MAX; }
    uint16_t get(uint32_t addr, const char *space = nullptr) const override;
    void put(uint32_t addr, uint16_t data,
            const char *space = nullptr) const override;

protected:
#ifdef WITH_ASSEMBLER
    libasm::Assembler *assembler() const override;
#endif
#ifdef WITH_DISASSEMBLER
    libasm::Disassembler *disassembler() const override;
#endif
};

struct DataI8051 final : DmaMemory {
    DataI8051() : DmaMemory(Endian::ENDIAN_BIG) {}

    uint32_t maxAddr() const override { return UINT16_MAX; }
    uint16_t raw_read(uint32_t addr) const override {
        return DmaMemory::raw_read(OFFSET + addr);
    }
    void raw_write(uint32_t addr, uint16_t data) const override {
        DmaMemory::raw_write(OFFSET + addr, data);
    }
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

private:
    static constexpr uint32_t OFFSET = 0x10000U;
};

extern struct ProgI8051 ProgMemory;
extern struct DataI8051 DataMemory;

}  // namespace i8051
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
