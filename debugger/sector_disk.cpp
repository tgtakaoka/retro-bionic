#include "sector_disk.h"
#include <SD.h>
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

constexpr uint8_t CMD_READ = 1;
constexpr uint8_t CMD_WRITE = 2;

constexpr uint8_t ST_OK = 0;
constexpr uint8_t ST_ERROR = 1;

File images[SectorDisk::DRIVES];
char image_names[SectorDisk::DRIVES][64];
uint8_t buffer[SectorDisk::SECTOR_BYTES];
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

void SectorDisk::printDrives() const {
    auto none = true;
    for (auto d = 0; d < DRIVES; ++d) {
        if (*image_names[d] == 0)
            continue;
        cli.print("  ");
        cli.print(char('A' + d));
        cli.print(": ");
        cli.print(image_names[d]);
        cli.println(images[d] ? "" : " closed");
        none = false;
    }
    if (none)
        cli.println("  no disk mounted");
}

// Mounting happens at the prompt, never in a transfer, where a missing
// card would hold the CPU for long. A file that is not there is not made.
bool SectorDisk::mount(uint8_t drive, const char *path) {
    if (drive >= DRIVES)
        return false;
    unmount(drive);
    if (strlen(path) >= sizeof(image_names[drive]) || !SD.exists(path))
        return false;
    images[drive] = SD.open(path, FILE_WRITE_BEGIN);
    if (!images[drive])
        return false;
    strcpy(image_names[drive], path);
    enable(true);
    return true;
}

void SectorDisk::unmount(uint8_t drive) {
    if (drive >= DRIVES)
        return;
    images[drive].close();
    *image_names[drive] = 0;
}

void SectorDisk::mountBeside(const char *path) {
    const auto dot = strrchr(path, '.');
    const auto slash = strrchr(path, '/');
    const size_t stem = (dot && dot > slash) ? dot - path : strlen(path);
    char name[sizeof(image_names[0])];
    for (auto d = 0; d < DRIVES; ++d) {
        unmount(d);
        if (stem + sizeof("b.dsk") > sizeof(name))
            continue;
        memcpy(name, path, stem);
        name[stem] = 0;
        if (d)
            strncat(name, "abcd" + d, 1);
        strcat(name, ".dsk");
        mount(d, name);
    }
    enable(images[0]);
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
        return _drive;
    case TRACK:
        return _track;
    case SECTOR:
        return _sector;
    case DMAL:
        return _dma & 0xFF;
    case DMAH:
        return _dma >> 8;
    case COMMAND:
        return _status;
    case MOUNTED: {
        uint8_t bits = 0;
        for (auto d = 0; d < SectorDisk::DRIVES; ++d) {
            if (images[d])
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
        _dma = (_dma & 0xFF00) | (data & 0xFF);
        break;
    case DMAH:
        _dma = (_dma & 0x00FF) | ((data & 0xFF) << 8);
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

// Past the file's end a drive reads E5, the CP/M fill, and a write
// grows the file to the sector.
void SectorDisk::transfer(bool toDisk) {
    _status = ST_ERROR;
    if (_drive >= DRIVES || _track >= TRACKS || _sector < 1 ||
            _sector > SECTORS)
        return;
    auto &image = images[_drive];
    if (!image)
        return;
    const uint32_t at =
            (uint32_t(_track) * SECTORS + _sector - 1) * SECTOR_BYTES;
    if (toDisk) {
        if (image.size() < at) {
            memset(buffer, 0xE5, sizeof(buffer));
            image.seek(image.size());
            while (image.size() < at) {
                const auto left = at - image.size();
                if (image.write(buffer, left < sizeof(buffer) ? left : sizeof(buffer)) == 0)
                    return;
                Watchdog::feed();
            }
        }
        for (auto i = 0U; i < SECTOR_BYTES; ++i)
            buffer[i] = _mems->read(_dma_base + uint16_t(_dma + i));
        if (!image.seek(at) || image.write(buffer, SECTOR_BYTES) != SECTOR_BYTES)
            return;
        image.flush();
    } else {
        const auto got = image.seek(at) ? int(image.read(buffer, SECTOR_BYTES)) : 0;
        for (auto i = got < 0 ? 0 : got; i < int(SECTOR_BYTES); ++i)
            buffer[i] = 0xE5;
        for (auto i = 0U; i < SECTOR_BYTES; ++i)
            _mems->write(_dma_base + uint16_t(_dma + i), buffer[i]);
    }
    Watchdog::feed();
    _status = ST_OK;
}

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
