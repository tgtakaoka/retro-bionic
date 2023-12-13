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

#ifndef __UNIOBUS_H__
#define __UNIOBUS_H__

#include <stdint.h>

#define UNIOBUS_VERSION_MAJOR 1
#define UNIOBUS_VERSION_MINOR 0
#define UNIOBUS_VERSION_PATCH 0
#define UNIOBUS_VERSION_STRING "1.0.0"

namespace unio {

/**
 * Microchip UNI/O Serial bus.
 */
struct UnioBus final {
    /**
     * @param scio digital I/O pin number for SCIO.
     * @param bit_us UNI/O bit period in micro second.
     */
    UnioBus(uint8_t scio, uint8_t bit_us);

    void begin();
    void standby();

    bool isIdle() const { return _state == IDLE; }
    bool isStandby() const { return _state == STANDBY; }

    bool transfer(uint8_t addr, const uint8_t *command, uint8_t clen, uint8_t *response = nullptr,
            uint8_t rlen = 0, const uint8_t *data = nullptr, uint8_t dlen = 0);

private:
    const uint8_t _scio;
    const uint8_t _bit_us;
    const uint8_t _bit_half;
    const uint8_t _bit_quater;
    bool _state;

    enum State : uint8_t {
        IDLE = 0,
        STANDBY = 1,
    };

    enum MasterAck : uint8_t {
        MAK = 1,
        NOMAK = 0,
    };
    enum SlaveAck : uint8_t {
        SAK = 1,
        NOSAK = 0,
    };

    void writeBit(bool data);
    uint8_t readBit(bool sak = NOSAK);
    State acknowledge(bool mak);
    State header(uint8_t addr);
    State writeByte(uint8_t data, bool mak);
    State readByte(uint8_t &data, bool mak);
};

}  // namespace unio

#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
