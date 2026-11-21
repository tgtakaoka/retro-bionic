#include "sector_disk.h"
#include <SD.h>
#include <strings.h>
#include "debugger.h"
#include "watchdog.h"

namespace debugger {

namespace {
constexpr uint8_t DRIVE = 0;
constexpr uint8_t TRACK = 1;
constexpr uint8_t SECTOR = 2;
constexpr uint8_t DMAL = 3;
constexpr uint8_t DMAH = 4;
constexpr uint8_t COMMAND = 5;
constexpr uint8_t MOUNTED = 6;
constexpr uint8_t DMAX = 7;

constexpr uint8_t CMD_READ = 1;
constexpr uint8_t CMD_WRITE = 2;

constexpr uint8_t ST_OK = 0;
constexpr uint8_t ST_ERROR = 1;

}  // namespace

const char *SectorDisk::name() const {
    return "DISK";
}

const char *SectorDisk::description() const {
    return "CP/M disk";
}

void SectorDisk::print() const {
    printDrives();
}

const char *SectorDisk::unitName(uint8_t unit) const {
    static constexpr const char *NAMES[DRIVES] = {"A", "B", "C", "D"};
    return unit < DRIVES ? NAMES[unit] : "";
}

uint8_t SectorDisk::unitOf(const char *name) const {
    const auto d = (name[0] | 0x20) - 'a';
    return name[1] == 0 && d >= 0 && d < DRIVES ? d : DRIVES;
}

Disk::Mounted SectorDisk::mount(const char *name, const char *path) {
    const auto mounted = DiskDrives::mount(name, path);
    if (mounted == MOUNT_OK)
        enable(true);
    return mounted;
}

void SectorDisk::mountBeside(const char *path) {
    const auto dot = strrchr(path, '.');
    const auto slash = strrchr(path, '/');
    const size_t stem = (dot && dot > slash) ? dot - path : strlen(path);
    char name[64];
    for (auto d = 0; d < DRIVES; ++d) {
        if (stem + sizeof("b.dsk") > sizeof(name)) {
            unmountUnit(d);
            continue;
        }
        memcpy(name, path, stem);
        name[stem] = 0;
        if (d)
            strncat(name, "abcd" + d, 1);
        strcat(name, ".dsk");
        if (mountUnit(d, name) == MOUNT_BAD_SIZE) {
            cli.print(name);
            cli.println(": no disk format of its size");
        }
    }
    enable(unit(0).mounted());
}

void SectorDisk::reset() {
    _drive = _track = 0;
    _sector = 1;
    _dma = 0;
    _status = ST_OK;
}

uint16_t SectorDisk::peek(uint32_t addr) const {
    switch (addr - _base_addr) {
    case DRIVE:
        return _drive < DRIVES ? _drive | (formatIndex(_drive) << 4) : _drive;
    case TRACK:
        return _track;
    case SECTOR:
        return _sector;
    case DMAL:
        return _dma & 0xFF;
    case DMAH:
        return (_dma >> 8) & 0xFF;
    case DMAX:
        return _dma >> 16;
    case COMMAND:
        return _status;
    case MOUNTED: {
        uint8_t bits = 0;
        for (auto d = 0; d < DRIVES; ++d) {
            if (unit(d).mounted())
                bits |= 1 << d;
        }
        return bits;
    }
    default:
        return 0xFF;
    }
}

void SectorDisk::write(uint32_t addr, uint16_t data) {
    switch (addr - _base_addr) {
    case DRIVE:
        _drive = data;
        break;
    case TRACK:
        _track = data;
        break;
    case SECTOR:
        _sector = data;
        break;
    case DMAL:
        _dma = (_dma & ~0xFF) | (data & 0xFF);
        break;
    case DMAH:
        _dma = (_dma & ~0xFF00) | ((data & 0xFF) << 8);
        break;
    case DMAX:
        _dma = (_dma & 0xFFFF) | (uint32_t(data & 0xFF) << 16);
        break;
    case COMMAND:
        if (data == CMD_READ || data == CMD_WRITE) {
            transfer(data == CMD_WRITE);
        } else {
            _status = ST_ERROR;
        }
        break;
    }
}

// One whole sector, flushed as it is written.
void SectorDisk::transfer(bool toDisk) {
    _status = ST_ERROR;
    if (_drive >= DRIVES)
        return;
    auto &drive = unit(_drive);
    const auto format = drive.geometry();
    uint32_t lsn;
    if (format == nullptr ||
            !drive.lsn(_track / format->heads, _track % format->heads, _sector,
                    lsn))
        return;
    const auto buffer = DiskUnit::buffer();
    if (toDisk) {
        for (auto i = 0U; i < format->sectorBytes; ++i)
            buffer[i] = _mems->read_byte(dmaAt(i));
        if (!drive.write(lsn) || !drive.flush())
            return;
    } else {
        if (!drive.read(lsn))
            return;
        for (auto i = 0U; i < format->sectorBytes; ++i)
            _mems->write_byte(dmaAt(i), buffer[i]);
    }
    _status = ST_OK;
}

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
