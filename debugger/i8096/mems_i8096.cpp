#include "mems_i8096.h"
#include <asm_i8096.h>
#include <dis_i8096.h>
#include "regs_i8096.h"

namespace debugger {
namespace i8096 {

// The 80C196KC and 80296SA have register RAM up to 1FFH, the 80C196NP up
// to 3FFH.
MemsI8096::MemsI8096(Devs *devs, RegsI8096 *regs, CpuType cpu)
    : DmaMemory(Endian::ENDIAN_LITTLE), _devs(devs), _regs(regs) {
    setCpu(cpu);
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::i8096::AsmI8096();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::i8096::DisI8096();
#endif
}

void MemsI8096::setCpu(CpuType cpu) {
    _cpu = cpu;
    _regsEnd = cpu == CPU_80C196NP                              ? 0x400
               : cpu == CPU_80C196KC || cpu == CPU_80296SA ? 0x200
                                                           : 0x100;
}

uint16_t MemsI8096::read(uint32_t addr) const {
    if (_cpu >= CPU_80C196NP) {
        // CCB0: no wait states, multiplexed, #WR/#BHE, 16-bit bus, no
        // powerdown; CCB1: 64-Kbyte mode
        if (addr == CCB)
            return 0xC6;
        if (addr == CCB1)
            return 0xDA;
    } else if (addr == CCB) {
        // Chip Configuration Register
        // 16 Bit width, #WR/#BHE, ALE, infinite READY, no ROM protection
        return 0xFF;
    }
    return _devs->isSelected(addr) ? _devs->read(addr) : read_byte(addr);
}

void MemsI8096::write(uint32_t addr, uint16_t data) const {
    if (_devs->isSelected(addr)) {
        _devs->write(addr, data);
    } else {
        write_byte(addr, data);
    }
}

uint16_t MemsI8096::get_data(uint32_t addr) const {
    return onChip(addr) ? _regs->read_data(addr) : read_byte(addr);
}

void MemsI8096::put_data(uint32_t addr, uint16_t data) const {
    if (onChip(addr)) {
        _regs->write_data(addr, data);
    } else {
        write_byte(addr, data);
    }
}

uint16_t MemsI8096::get_prog(uint32_t addr) const {
    return read_byte(addr);
}

void MemsI8096::put_prog(uint32_t addr, uint16_t data) const {
    write_byte(addr, data);
}

}  // namespace i8096
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
