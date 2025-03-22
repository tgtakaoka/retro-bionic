#ifndef __MEMS_PDP8_H__
#define __MEMS_PDP8_H__

#include "devs.h"
#include "mems.h"

namespace debugger {
namespace pdp8 {

struct ControlPanel;

struct MemsPdp8 final : DmaMemory {
    MemsPdp8(uint8_t addr_bit);

    uint32_t maxAddr() const override { return _max_addr; }
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

    uint16_t get(uint32_t addr, const char * = nullptr) const override;
    void put(uint32_t addr, uint16_t data,
            const char * = nullptr) const override;
    uint16_t get_inst(uint32_t addr) const override;
    void put_inst(uint32_t addr, uint16_t data) const override;

private:
    const uint16_t _max_addr;

    void isPrint(const uint8_t *data, char buf[2]) const override;

    friend ControlPanel;
#ifdef WITH_ASSEMBLER
    libasm::Assembler *getAsm() const { return _assembler; }
#endif
#ifdef WITH_DISASSEMBLER
    libasm::Disassembler *getDis() const { return _disassembler; }
#endif
};

struct ControlPanel final : DmaMemory {
    ControlPanel(MemsPdp8 *mems);

    uint32_t maxAddr() const override { return 07777; }
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

private:
    MemsPdp8 *const _mems;
    static constexpr auto OFFSET = UINT32_C(0100000);

    void isPrint(const uint8_t *data, char buf[2]) const override {
        return _mems->isPrint(data, buf);
    }
};

}  // namespace pdp8
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
