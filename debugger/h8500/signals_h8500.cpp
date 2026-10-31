#include "signals_h8500.h"
#include "char_buffer.h"
#include "chip_h8500.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_h8500.h"

namespace debugger {
namespace h8500 {

namespace {
// 74HCS153 and 74AHCT157 select to output: 15ns at most at 5V.
constexpr auto asel_delay_ns = 15;

const ChipH8500 *chip_ = &H8_510;
}  // namespace

void Signals::setChip(const ChipH8500 *chip) {
    chip_ = chip;
}

bool Signals::bus16() {
    return chip_->bus16;
}

namespace {
inline uint32_t nibbles(uint32_t port) {
    const auto v = busValue(ADDR, port);
    return (v & 0xF) | (v >> 12 & 0xF0);  // P30-P33, then P34-P37
}
}  // namespace

// The H8/510's address through the muxes, ASEL1:ASEL0 stepped in gray
// order 00, 01, 11, 10 so one select line changes at a time; it rests at
// 00. The H8/536 has A0-A7 and A12-A15 direct and only A16-A19 behind
// ASEL0.
void Signals::getAddr() {
    if (chip_->bus16) {
        const auto sel00 = nibbles(PIN(ADDR));  // A0-A3, A16-A19
        digitalWriteFast(PIN_ASEL0, HIGH);
        delayNanoseconds(asel_delay_ns);
        const auto sel01 = nibbles(PIN(ADDR));  // A4-A7, A20-A23
        digitalWriteFast(PIN_ASEL1, HIGH);
        delayNanoseconds(asel_delay_ns);
        const auto sel11 = nibbles(PIN(ADDR));  // A8-A11
        digitalWriteFast(PIN_ASEL0, LOW);
        delayNanoseconds(asel_delay_ns);
        const auto sel10 = nibbles(PIN(ADDR));  // A12-A15
        digitalWriteFast(PIN_ASEL1, LOW);
        addr = (sel00 & 0xF) | (sel01 & 0xF) << 4 | (sel11 & 0xF) << 8 |
               (sel10 & 0xF) << 12 | (sel00 >> 4) << 16 | (sel01 >> 4) << 20;
    } else {
        const auto low = busRead(AL);
        const auto mid = nibbles(PIN(ADDR));  // A8-A11, A12-A15
        digitalWriteFast(PIN_ASEL0, HIGH);
        delayNanoseconds(asel_delay_ns);
        const auto high = nibbles(PIN(ADDR)) & 0xF;  // A16-A19
        digitalWriteFast(PIN_ASEL0, LOW);
        addr = low | mid << 8 | high << 16;
    }
}

void Signals::getData() {
    data = chip_->bus16 ? busRead(D16) : busRead(D8);
}

// #IRQ0, NMI and #WAIT are on GPIO7, TXD and RXD on GPIO6; busWriteFast()
// touches only the data bits, and the H8/536's A0-A7 stay inputs.
void Signals::stageData() const {
    if (chip_->bus16) {
        busWriteFast(D16, data);
    } else {
        busWriteFast(D8, data);
    }
}

void Signals::driveData() const {
    if (chip_->bus16) {
        busMode(D16, OUTPUT);
    } else {
        busMode(D8, OUTPUT);
    }
}

void Signals::inputMode() const {
    if (chip_->bus16) {
        busMode(D16, INPUT);
    } else {
        busMode(D8, INPUT);
    }
}

void Signals::print() const {
#ifdef PROFILE_CYCLES
    // slot, and inject/capture flags
    cli.print(readMemory() ? ' ' : 'i');
    cli.print(writeMemory() ? ' ' : 'c');
    cli.print(' ');
    cli.printDec(pos(), -4);
#endif
    //                              01234567890123456789012
#ifdef PROFILE_CYCLES
    // The raw strobes as #AS opened the cycle and once they settled.
    static constexpr char line[] = "R A=xxxxxx D=xxxx c=xx c=xx";
#else
    static constexpr char line[] = "R A=xxxxxx D=xxxx";
#endif
    auto &buffer = Cycles::buffer();
    buffer.set(line);
    buffer[0] = read() ? 'R' : 'W';
    buffer.hex24(4, addr);
    // Columns 13-14 show D8-D15, the even byte; columns 15-16 D0-D7. A
    // read shows the aligned pair it was served.
    if (!chip_->bus16) {
        buffer.hex8(13, data);
        buffer[15] = buffer[16] = ' ';
    } else if (read() || wordWrite()) {
        buffer.hex16(13, data);
    } else if (addr & 1) {
        buffer[13] = buffer[14] = ' ';
        buffer.hex8(15, data);
    } else {
        buffer.hex8(13, data >> 8);
        buffer[15] = buffer[16] = ' ';
    }
#ifdef PROFILE_CYCLES
    buffer.hex8(20, cntl());
    buffer.hex8(25, strobes());
#endif
    cli.println(buffer);
}

}  // namespace h8500
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
