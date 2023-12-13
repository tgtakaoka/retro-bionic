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

#ifndef __UNIO_EEPROM_H__
#define __UNIO_EEPROM_H__

#include "unio_bus.h"

namespace unio {

/**
 * Microchip 11AAXXX Serial EEOROM
 */
struct Eeprom final {
    /**
     * @param bus UNI/O bus on which this device is connected.
     * @param addr The address of this device (default 0xA0).
     */
    Eeprom(UnioBus &bus, uint8_t addr = 0xA0) : _bus(bus), _dev_addr(addr) {}

    bool read(uint16_t addr, uint16_t len, uint8_t *buffer);
    bool write(uint16_t addr, uint16_t len, const uint8_t *buffer);
    bool readStatus(uint8_t &status);
    bool writeStatus(uint8_t status);
    bool eraseAll();
    bool setAll();

private:
    UnioBus &_bus;
    const uint8_t _dev_addr;

    bool enableWrite();
    bool disableWrite();
    bool writePage(uint16_t addr, uint16_t len, const uint8_t *buffer);
    bool waitForWrite();

    enum Command : uint8_t {
        READ = 0x03,   // Read data from address
        CRRD = 0x06,   // Read data from current location
        WRITE = 0x6C,  // Write data to address
        WREN = 0x96,   // Set the write enable latch
        WRDI = 0x91,   // Reset the write enable latch
        RDSR = 0x05,   // Read STATUS register
        WRSR = 0X6E,   // Write STATUS register
        ERAL = 0X6D,   // Write 0x00 to entire array
        SETAL = 0X67,  // Write 0xFF to entire array
    };

    enum Status : uint8_t {
        BP_gp = 2,  // Block Protection position
        BP_gm = 3,  // Block Protection mask
        WEL = 2,    // Write Enable Latch
        WIP = 1,    // Write-In-Process when 1
    };

    static constexpr uint8_t hi(uint16_t val) { return val >> 8; }
    static constexpr uint8_t lo(uint16_t val) { return val & 0xFF; }
};

}  // namespace unio

#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
