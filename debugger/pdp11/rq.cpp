#include "rq.h"
#include <string.h>
#include "debugger.h"
#include "watchdog.h"

namespace debugger {
namespace pdp11 {

namespace {
constexpr uint16_t BLOCK = 512;
constexpr uint16_t CYLINDER_BLOCKS = 256;
}  // namespace

Rq::Rq(const Mems *mems)
    : DiskDrives(_units, UNITS, _geometry, UNITS), _mems(mems), _mscp(*this) {
    for (auto &b : _blocks)
        b = 0;
}

const char *Rq::name() const {
    return "MSCP";
}

const char *Rq::description() const {
    return "RQDX3 MSCP disk";
}

void Rq::print() const {
    printDrives();
    cli.print("  SA=");
    cli.printNum(_mscp.sa(), 8, 6);
    const auto step = _mscp.step();
    if (step) {
        cli.print(" step ");
        cli.print(step);
    } else {
        cli.print(" vector ");
        cli.printNum(_mscp.vector(), 8, 3);
    }
    cli.println(_request ? " requesting" : "");
}

const char *Rq::unitName(uint8_t unit) const {
    static char name[] = "ra0";
    name[2] = '0' + unit;
    return name;
}

uint8_t Rq::unitOf(const char *name) const {
    if ((name[0] | 0x20) != 'r' || (name[1] | 0x20) != 'a')
        return UNITS;
    const auto u = name[2] - '0';
    return u >= 0 && u < UNITS && name[3] == 0 ? u : UNITS;
}

bool Rq::place(uint8_t unit, uint32_t bytes, const DiskGeometry *&format,
        uint32_t &offset) const {
    const uint32_t blocks = bytes / BLOCK;
    const uint32_t cylinders = (blocks + CYLINDER_BLOCKS - 1) / CYLINDER_BLOCKS;
    if (blocks == 0 || cylinders > UINT16_MAX)
        return false;
    _blocks[unit] = blocks;
    _geometry[unit] = {
            "RD54", uint16_t(cylinders), 1, CYLINDER_BLOCKS, BLOCK, 0, 0};
    format = &_geometry[unit];
    offset = 0;
    return true;
}

// As a file is loaded, never in a transfer, where a missing card would
// hold the CPU for long.
void Rq::mountBeside(const char *path) {
    const auto dot = strrchr(path, '.');
    const auto slash = strrchr(path, '/');
    const size_t stem = (dot && dot > slash) ? dot - path : strlen(path);
    char name[64];
    for (auto u = 0; u < UNITS; ++u) {
        if (stem + sizeof("1.dsk") > sizeof(name)) {
            unmountUnit(u);
            continue;
        }
        memcpy(name, path, stem);
        name[stem] = 0;
        if (u)
            strncat(name, "0123" + u, 1);
        strcat(name, ".dsk");
        if (mountUnit(u, name) == MOUNT_BAD_SIZE) {
            cli.print(name);
            cli.println(": not a disk image");
        }
    }
}

void Rq::unmountAll() {
    for (auto u = 0; u < UNITS; ++u)
        unmountUnit(u);
}

void Rq::reset() {
    _mscp.reset();
    acknowledge();
}

bool Rq::loop() {
    return _enabled && _mscp.service();
}

void Rq::enable(bool enabled) {
    if (!enabled)
        reset();
    Device::enable(enabled);
}

void Rq::acknowledge() {
    if (_request) {
        _request = false;
        Debugger.target().negateInt(VECTOR);
    }
}

void Rq::interrupt() {
    if (!_request) {
        _request = true;
        Debugger.target().assertInt(VECTOR);
    }
}

uint16_t Rq::peek(uint32_t addr) const {
    return (addr - _base_addr) == SA ? _mscp.sa() : 0;
}

uint16_t Rq::read(uint32_t addr) {
    if ((addr - _base_addr) == IP)
        _mscp.readIp();
    return peek(addr);
}

void Rq::write(uint32_t addr, uint16_t data) {
    switch (addr - _base_addr) {
    case IP:
        reset();
        break;
    case SA:
        _mscp.writeSa(data);
        break;
    }
}

uint32_t Rq::unitBlocks(uint8_t u) const {
    return unit(u).mounted() ? _blocks[u] : 0;
}

bool Rq::transfer(
        uint8_t u, uint32_t lbn, uint32_t addr, uint32_t bytes, bool toDisk) {
    auto &disk = unit(u);
    const auto sector = DiskUnit::buffer();
    while (bytes != 0) {
        const auto n = bytes < BLOCK ? bytes : BLOCK;
        if (toDisk) {
            for (auto i = 0U; i < BLOCK; i += 2) {
                const uint16_t w = i < n ? _mems->read16(addr + i) : 0;
                sector[i] = w;
                sector[i + 1] = w >> 8;
            }
            if (!disk.write(lbn))
                return false;
        } else {
            if (!disk.read(lbn))
                return false;
            for (auto i = 0U; i < n; i += 2)
                _mems->write16(addr + i, sector[i] | (sector[i + 1] << 8));
        }
        bytes -= n;
        addr += n;
        ++lbn;
        Watchdog::feed();
    }
    return !toDisk || disk.flush();
}

}  // namespace pdp11
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
