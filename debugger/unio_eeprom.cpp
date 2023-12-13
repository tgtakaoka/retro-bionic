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

#include "unio_eeprom.h"

namespace unio {

bool Eeprom::read(uint16_t addr, uint16_t len, uint8_t *buffer) {
    uint8_t cmd[] = {READ, hi(addr), lo(addr)};
    return _bus.transfer(_dev_addr, cmd, sizeof(cmd), buffer, len);
}

bool Eeprom::write(uint16_t addr, uint16_t len, const uint8_t *buffer) {
    while (len) {
        auto plen = len;
        if ((addr & 0xF) + plen > 16)
            plen = 16 - (addr & 0xF);
        enableWrite();
        writePage(addr, plen, buffer);
        if (!waitForWrite())
            break;
        addr += plen;
        len -= plen;
        buffer += plen;
    }
    return _bus.isStandby();
}

bool Eeprom::readStatus(uint8_t &status) {
    static constexpr uint8_t cmd[] = {RDSR};
    return _bus.transfer(_dev_addr, cmd, sizeof(cmd), &status, sizeof(status));
}

bool Eeprom::writeStatus(uint8_t status) {
    uint8_t cmd[] = {WRSR, status};
    return _bus.transfer(_dev_addr, cmd, sizeof(cmd));
}

bool Eeprom::eraseAll() {
    static constexpr uint8_t cmd[] = {ERAL};
    enableWrite();
    _bus.transfer(_dev_addr, cmd, sizeof(cmd));
    return waitForWrite();
}

bool Eeprom::setAll() {
    static constexpr uint8_t cmd[] = {SETAL};
    enableWrite();
    _bus.transfer(_dev_addr, cmd, sizeof(cmd));
    return waitForWrite();
}

bool Eeprom::enableWrite() {
    static constexpr uint8_t cmd[] = {WREN};
    _bus.transfer(_dev_addr, cmd, sizeof(cmd));
    return waitForWrite();
}

bool Eeprom::disableWrite() {
    static constexpr uint8_t cmd[] = {WRDI};
    _bus.transfer(_dev_addr, cmd, sizeof(cmd));
    return waitForWrite();
}

bool Eeprom::writePage(uint16_t addr, uint16_t len, const uint8_t *buffer) {
    uint8_t cmd[] = {WRITE, hi(addr), lo(addr)};
    return _bus.transfer(_dev_addr, cmd, sizeof(cmd), nullptr, 0, buffer, len);
}

bool Eeprom::waitForWrite() {
    uint8_t status;
    auto r = true;
    do {
        r = readStatus(status);
    } while (r && (status & WIP));
    return r;
}

}  // namespace unio

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
