#include "mems.h"
#include "debugger.h"

#ifdef WITH_ASSEMBLER
#include <asm_base.h>
#endif
#ifdef WITH_DISASSEMBLER
#include <dis_base.h>
#endif
#include <dis_memory.h>

namespace debugger {

namespace {

struct DisMemory final : libasm::DisMemory {
    DisMemory(const Mems *mems, uint32_t max_bytes)
        : libasm::DisMemory(0), _mems(mems), _max_bytes(max_bytes) {}
    const Mems *_mems;
    const uint32_t _max_bytes;

    bool hasNext() const override { return address() <= _max_bytes; }
    void setAddress(uint32_t addr) { resetAddress(addr); }
    uint8_t nextByte() override { return _mems->raw_read(address()); }
};

uint8_t DMA_MEMORY[DmaMemory::MEM_SIZE] DMAMEM;
uint8_t EXT_MEMORY[ExtMemory::MEM_SIZE] EXTMEM;

}  // namespace

uint16_t DmaMemory::raw_read(uint32_t addr) const {
    return addr < MEM_SIZE ? DMA_MEMORY[addr] : 0;
}

void DmaMemory::raw_write(uint32_t addr, uint16_t data) const {
    if (addr < MEM_SIZE)
        DMA_MEMORY[addr] = data;
}

uint16_t ExtMemory::raw_read(uint32_t addr) const {
    return addr < MEM_SIZE ? EXT_MEMORY[addr] : 0;
}

void ExtMemory::raw_write(uint32_t addr, uint16_t data) const {
    if (addr < MEM_SIZE)
        EXT_MEMORY[addr] = data;
}

Mems::Mems(Endian endian)
    : _endian(endian)
#ifdef WITH_ASSEMBLER
      ,
      _assembler(nullptr)
#endif
#ifdef WITH_DISASSEMBLER
      ,
      _disassembler(nullptr)
#endif
{
}

Mems::~Mems() {
#ifdef WITH_ASSEMBLER
    delete _assembler;
#endif
#ifdef WITH_DISASSEMBLER
    delete _disassembler;
#endif
}

uint16_t Mems::raw_read16(uint32_t addr) const {
    return _endian == ENDIAN_BIG ? raw_read16be(addr) : raw_read16le(addr);
}

uint16_t Mems::raw_read16be(uint32_t addr) const {
    auto data = raw_read(addr + 0) << 8;
    data |= raw_read(addr + 1) & 0xFF;
    return data;
}

uint16_t Mems::raw_read16le(uint32_t addr) const {
    auto data = raw_read(addr + 1) << 8;
    data |= raw_read(addr + 0) & 0xFF;
    return data;
}

void Mems::raw_write16(uint32_t addr, uint16_t data) const {
    if (_endian == ENDIAN_BIG)
        raw_write16be(addr, data);
    else
        raw_write16le(addr, data);
}

void Mems::raw_write16be(uint32_t addr, uint16_t data) const {
    raw_write(addr + 0, hi(data));
    raw_write(addr + 1, lo(data));
}

void Mems::raw_write16le(uint32_t addr, uint16_t data) const {
    raw_write(addr + 0, lo(data));
    raw_write(addr + 1, hi(data));
}

void Mems::put(uint32_t addr, const uint8_t *buffer, uint8_t len) const {
    for (auto i = 0; i < len; ++i) {
        put(addr + i, buffer[i]);
    }
}

void Mems::RomArea::set(uint32_t begin, uint32_t end) {
    _begin = begin;
    _end = end;
}

bool Mems::RomArea::readOnly(uint32_t addr) const {
    return addr >= _begin && addr <= _end;
}

void Mems::RomArea::print() const {
    cli.print("ROM area: ");
    if (_begin < _end) {
        cli.printHex(_begin, 4);
        cli.print('-');
        cli.printlnHex(_end, 4);
    } else {
        cli.println("none");
    }
}

namespace {

void printSpaces(uint_fast8_t n) {
    for (uint_fast8_t i = 0; i < n; ++i)
        cli.print(' ');
}

}  // namespace

uint8_t Mems::addressWidth() const {
#ifdef WITH_DISASSEMBLER
    if (_disassembler)
        return _disassembler->config().addressWidth();
#endif
    return 16;
}

uint8_t Mems::addressUnit() const {
#ifdef WITH_DISASSEMBLER
    if (_disassembler)
        return _disassembler->config().addressUnit();
#endif
    return 1;
}

uint8_t Mems::opCodeWidth() const {
#ifdef WITH_DISASSEMBLER
    if (_disassembler)
        return _disassembler->config().opCodeWidth();
#endif
    return 8;
}

uint8_t Mems::listRadix() const {
#ifdef WITH_DISASSEMBLER
    if (_disassembler)
        return _disassembler->listRadix();
#endif
    return 16;
}

void Mems::isPrint(const uint8_t *data, char buf[2]) const {
    buf[0] = isprint(data[0]) ? data[0] : '.';
    buf[1] = isprint(data[1]) ? data[1] : '.';
}

/** Returns the number of columns to print |len| bytes data */
uint8_t Mems::dataColumns(uint8_t len) const {
    const auto bits = opCodeWidth();
    const auto unit = addressUnit();
    // if |len| is odd, |count| may be round down.
    const auto count = (bits == 16 || unit == 2) ? len / 2 : len;
    return (1 + Debugger::numDigits(bits, listRadix())) * count;
}

void Mems::printAddress(uint32_t addr) const {
    const auto radix = listRadix();
    cli.printNum(addr, radix, Debugger::numDigits(addressWidth(), radix));
    cli.print(':');
}

/** Print |len| bytes data from |buf[start] until |buf[start+len]| */
void Mems::dump(
        const uint8_t *buf, uint_fast8_t start, uint_fast8_t len) const {
    const auto radix = listRadix();
    const auto bits = opCodeWidth();
    const auto digits = Debugger::numDigits(bits, radix);
    const auto unit = addressUnit();
    const auto chunk = (bits == 16 || unit == 2) ? 2 : 1;
    if (bits == 16 && start % 2 != 0) {
        printSpaces(1 + digits / 2);
        cli.printNum(buf[start++], radix, digits / 2);
        len--;
    }
    for (uint_fast8_t i = 0; i + chunk <= len;) {
        printSpaces(1);
        if (chunk == 2) {
            uint16_t data;
            if (_endian == ENDIAN_BIG) {
                data = uint16(buf[start + i + 0], buf[start + i + 1]);
            } else {
                data = uint16(buf[start + i + 1], buf[start + i + 0]);
            }
            if (bits == 12)
                data &= 07777;
            cli.printNum(data, radix, digits);
            i += 2;
        } else {
            cli.printNum(buf[start + i++], radix, digits);
        }
    }
    if (bits == 16 && (start + len) % 2 != 0) {
        printSpaces(1);
        cli.printNum(buf[start + len - 1], radix, digits / 2);
        printSpaces(digits / 2);
    }
}

void Mems::dumpMemory(uint32_t addr, uint16_t len, const char *space) const {
    const auto start = addr;
    auto end = addr + len;
    if (end > maxAddr())
        end = maxAddr() + 1;
    const auto bits = opCodeWidth();
    const auto unit = addressUnit();
    // Quantize address to a multiple of 8 or 16.
    const auto chunk = (bits == 16 || unit == 2) ? 2 : 1;
    const auto step = 16 / unit;
    // Print 16 bytes of data
    for (addr &= ~(step - 1); addr < end; addr += step) {
        printAddress(addr);
        const auto head = (addr < start) ? start - addr : 0;
        const auto tail = (end < addr + step) ? addr + step - end : 0;
        const auto count = step - head - tail;
        // Read 16 bytes into |buf|.
        uint8_t buf[16];
        for (auto n = 0; n < 16; n += unit) {
            const auto a = addr + n / unit;
            if (a >= start && a < end) {
                if (unit == 1) {
                    buf[n] = get(a, space);
                } else {
                    const auto data = get(a, space);
                    if (_endian == ENDIAN_BIG) {
                        buf[n + 0] = hi(data);
                        buf[n + 1] = lo(data);
                    } else {
                        buf[n + 0] = lo(data);
                        buf[n + 1] = hi(data);
                    }
                }
            }
        }
        printSpaces(dataColumns(head * unit));
        dump(buf, head * unit, count * unit);
        printSpaces(dataColumns(tail * unit));
        printSpaces(1);
        printSpaces(head * chunk);
        for (auto n = 0; n < 16; n += unit) {
            const auto a = addr + n / unit;
            if (a >= start && a < end) {
                if (unit == 1) {
                    const char c = isprint(buf[n]) ? buf[n] : '.';
                    cli.print(c);
                } else {
                    char chars[2];
                    isPrint(buf + n, chars);
                    cli.print(chars[0]);
                    cli.print(chars[1]);
                }
            }
        }
        cli.println();
    }
}

#ifdef WITH_DISASSEMBLER
libasm::Disassembler *Mems::disassembler() const {
    auto dis = _disassembler;
    if (dis)
        dis->setCpu(Debugger.target().cpu());
    return dis;
}
#endif

uint32_t Mems::disassemble(uint32_t addr, uint8_t numInsn) const {
#ifdef WITH_DISASSEMBLER
    auto dis = disassembler();
    if (dis) {
        constexpr auto chunk = 6;
        const auto unit = addressUnit();
        const auto nameWidth = dis->config().nameMax() + 1;
        dis->setOption("uppercase", "true");
        const auto codeMax = dis->config().codeMax();
        DisMemory mem(this, (maxAddr() + 1) * unit - 1);
        uint16_t num = 0;
        while (num < numInsn) {
            char operands[80];
            libasm::Insn insn(addr);
            mem.setAddress(addr * unit);
            dis->decode(mem, insn, operands, sizeof(operands));
            auto step = codeMax < chunk ? codeMax : chunk;
            for (auto i = 0; i < insn.length(); i += step) {
                printAddress(insn.address() + i / unit);
                auto len = insn.length() - i;
                if (len >= step)
                    len = step;
                dump(insn.bytes(), i, len);
                if (len < step)
                    printSpaces(dataColumns(step - len));
                if (i == 0) {
                    printSpaces(2);
                    cli.printStr(insn.name(), -nameWidth);
                    cli.printStr(operands);
                }
                cli.println();
            }
            if (insn.getError()) {
                cli.print("Error: ");
                cli.printStr_P(insn.errorText_P());
                if (*insn.errorAt()) {
                    cli.print(" at '");
                    cli.printStr(insn.errorAt());
                    cli.print('\'');
                }
                cli.println();
                if (insn.getError() == libasm::NO_MEMORY)
                    break;
            }
            if ((addr += insn.length() / unit) >= maxAddr())
                break;
            ++num;
        }
    }
#endif
    return addr;
}

#ifdef WITH_ASSEMBLER
libasm::Assembler *Mems::assembler() const {
    auto as = _assembler;
    if (as)
        as->setCpu(Debugger.target().cpu());
    return as;
}
#endif

uint32_t Mems::assemble(uint32_t addr, const char *line) const {
#ifdef WITH_ASSEMBLER
    auto as = assembler();
    if (as) {
        libasm::Insn insn(addr);
        as->encode(line, insn);
        if (insn.getError()) {
            cli.print("Error: ");
            cli.print(insn.errorText_P());
            if (*insn.errorAt()) {
                cli.print(" at '");
                cli.printStr(insn.errorAt());
                cli.print('\'');
            }
            cli.println();
        } else {
            put(insn.address(), insn.bytes(), insn.length());
            disassemble(insn.address(), 1);
            addr += insn.length();
        }
    }
#endif
    return addr;
}

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
