#ifndef __MEMS_I8048_H__
#define __MEMS_I8048_H__

#include "mems.h"

namespace debugger {
namespace i8048 {

struct RegsI8048;

struct ProgI8048 final : DmaMemory {
    ProgI8048() : DmaMemory(Endian::ENDIAN_BIG) {}

    uint32_t maxAddr() const override { return 0xFFF; }
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

struct DataI8048 final : DmaMemory {
    DataI8048() : DmaMemory(Endian::ENDIAN_BIG) {}

    uint32_t maxAddr() const override { return 0xFF; }
    uint16_t raw_read(uint32_t addr) const override {
        return DmaMemory::raw_read(OFFSET + addr);
    }
    void raw_write(uint32_t addr, uint16_t data) const override {
        DmaMemory::raw_write(OFFSET + addr, data);
    }
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

private:
    static constexpr uint16_t OFFSET = 0x1000;
};

extern struct ProgI8048 ProgMemory;
extern struct DataI8048 DataMemory;

}  // namespace i8048
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
