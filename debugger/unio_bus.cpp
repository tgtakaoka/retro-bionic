/*
 * Copyright 2023 Tadashi G. Takaoka
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <Arduino.h>

#include "unio_bus.h"

namespace unio {

/**
 * UNI/O Bus timing in micro second
 *         __________       _     ___     ___     ___     _   _
 *        | standby  |start| |   |   |   |   |   |   |   | | | |____/device
 * SCIO __|  pulse   |_____| v___^   v___^   v___^   v___^ |_^  No  \address
 *                         | 0 | 1 | 0 | 1 | 0 | 1 | 0 | 1 |MAK|SAK|
 *                            ___     ___   _   _   _   _     _   _
 *                           |   |   |   | | | | | | | | |   | | | |/
 *                          _^   v___^   v_| v_| v_| v_| v___^ |_^  \
 *                         | 1 | 0 | 1 | 0 | 0 | 0 | 0 | 0 |MAK|SAK|
 *        _     _____       _     ___     ___     ___     _   _
 *      \| |   |  TSS|start| |   |   |   |   |   |   |   | | | |____/device
 *      /  v___^     |_____| v___^   v___^   v___^   v___^ |_^  No |\address
 *       |MAK|SAK|         | 0 | 1 | 0 | 1 | 0 | 1 | 0 | 1 |MAK|SAK|
 */

enum Timing {
    TE_MIN = 12,   // Bit period; 10 us minimum
    TE_MAX = 100,  // Bit period; 100 us maximum
    TSTBY = 600,   // Standby pulse time; 600 us minimum
    TSS = 10,      // Start header setup time; 10 us minimum
    THDR = 5,      // Start header low pulse time; 5 us minimum
};

namespace {
constexpr uint8_t limit(uint8_t a, uint8_t min, uint8_t max) {
    return a < min ? min : (a > max ? max : a & ~3);  // round to 4
}
}  // namespace

UnioBus::UnioBus(uint8_t scio, uint8_t bit_us)
    : _scio(scio),
      _bit_us(limit(bit_us, TE_MIN, TE_MAX)),
      _bit_half(_bit_us / 2),
      _bit_quater(_bit_half / 2),
      _state(IDLE) {}

void UnioBus::begin() {
    standby();
}

void UnioBus::standby() {
    digitalWriteFast(_scio, LOW);
    pinMode(_scio, OUTPUT);
    delayMicroseconds(10);
    digitalWriteFast(_scio, HIGH);
    delayMicroseconds(TSTBY);
    _state = STANDBY;
}

UnioBus::State UnioBus::header(uint8_t addr) {
    digitalWriteFast(_scio, LOW);
    delayMicroseconds(THDR);
    writeByte(0x55, MAK);
    return writeByte(addr, MAK);
}

void UnioBus::writeBit(bool data) {
    static constexpr uint8_t BEGIN[] = {HIGH, LOW};
    static constexpr uint8_t END[] = {LOW, HIGH};
    digitalWriteFast(_scio, BEGIN[data]);
    pinMode(_scio, OUTPUT);
    delayMicroseconds(_bit_half);
    digitalWriteFast(_scio, END[data]);
    delayMicroseconds(_bit_half);
}

uint8_t UnioBus::readBit(bool sak) {
    pinMode(_scio, INPUT_PULLUP);
    delayMicroseconds(_bit_quater);
    const auto begin = digitalReadFast(_scio);
    delayMicroseconds(_bit_half);
    const auto end = digitalReadFast(_scio);
    delayMicroseconds(_bit_quater);
    return !begin && end ? 1 : 0;
}

UnioBus::State UnioBus::acknowledge(bool mak) {
    writeBit(mak);
    auto r = readBit(SAK);
    if (mak == NOMAK) {
        digitalWriteFast(_scio, HIGH);
        pinMode(_scio, OUTPUT);
    }
    return r ? STANDBY : IDLE;
}

UnioBus::State UnioBus::writeByte(uint8_t data, bool mak) {
    for (auto bit = 8; bit; --bit) {
        writeBit(data & 0x80);
        data <<= 1;
    }
    return acknowledge(mak);
}

UnioBus::State UnioBus::readByte(uint8_t &data, bool mak) {
    uint8_t val = 0;
    for (auto bit = 8; bit; --bit) {
        val <<= 1;
        val |= readBit();
    }
    data = val;
    return acknowledge(mak);
}

bool UnioBus::transfer(uint8_t addr, const uint8_t *command, uint8_t clen, uint8_t *response,
        uint8_t rlen, const uint8_t *data, uint8_t dlen) {
    if (isStandby()) {
        auto mak = (rlen | dlen);
        auto r = header(addr);
        while (clen-- && r)
            r = writeByte(*command++, clen | mak);
        while (rlen-- && r)
            r = readByte(*response++, rlen);
        while (dlen-- && r)
            r = writeByte(*data++, dlen);
        _state = r;
    }
    return _state;
}

}  // namespace unio

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
