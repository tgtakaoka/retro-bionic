#include "disk_image.h"
#include <SD.h>
#include <string.h>
#include "watchdog.h"

namespace debugger {

struct DiskFile {
    File file;
    char path[64];
    uint8_t refs;
};

namespace {
DiskFile files[8];
uint8_t sector_buffer[DiskUnit::BUFFER_BYTES];

DiskFile *openFile(const char *path) {
    DiskFile *free = nullptr;
    for (auto &f : files) {
        if (f.refs && strcmp(f.path, path) == 0) {
            ++f.refs;
            return &f;
        }
        if (f.refs == 0 && free == nullptr)
            free = &f;
    }
    if (free == nullptr || strlen(path) >= sizeof(free->path) ||
            !SD.exists(path))
        return nullptr;
    free->file = SD.open(path, FILE_WRITE_BEGIN);
    if (!free->file)
        return nullptr;
    strcpy(free->path, path);
    free->refs = 1;
    return free;
}

void closeFile(DiskFile *f) {
    if (f && --f->refs == 0) {
        f->file.close();
        *f->path = 0;
    }
}
}  // namespace

uint8_t *DiskUnit::buffer() {
    return sector_buffer;
}

bool DiskUnit::mount(const char *path, uint32_t offset,
        const DiskGeometry *geometry, bool reserve) {
    unmount();
    if (geometry == nullptr || geometry->sectorBytes > BUFFER_BYTES)
        return false;
    _file = openFile(path);
    if (_file == nullptr)
        return false;
    _offset = offset;
    _geometry = geometry;
    if (reserve)
        grow(offset + geometry->bytes());
    return true;
}

void DiskUnit::unmount() {
    closeFile(_file);
    _file = nullptr;
    _geometry = nullptr;
    _offset = 0;
}

const char *DiskUnit::path() const {
    return _file ? _file->path : "";
}

bool DiskUnit::lsn(uint16_t cylinder, uint8_t head, uint16_t sector,
        uint32_t &lsn) const {
    if (_geometry == nullptr || cylinder >= _geometry->cylinders ||
            head >= _geometry->heads || sector < _geometry->firstSector ||
            sector - _geometry->firstSector >= _geometry->sectors)
        return false;
    lsn = (uint32_t(cylinder) * _geometry->heads + head) * _geometry->sectors +
          (sector - _geometry->firstSector);
    return true;
}

// Fills the file up to |size| with fill, a sector at a time.
bool DiskUnit::grow(uint32_t size) {
    auto &file = _file->file;
    if (file.size() >= size)
        return true;
    uint8_t fill[64];
    memset(fill, _geometry->fill, sizeof(fill));
    file.seek(file.size());
    while (file.size() < size) {
        const auto left = size - file.size();
        if (file.write(fill, left < sizeof(fill) ? left : sizeof(fill)) == 0)
            return false;
        Watchdog::feed();
    }
    return true;
}

bool DiskUnit::read(uint32_t lsn) {
    if (!mounted() || lsn >= lsns())
        return false;
    auto &file = _file->file;
    const auto bytes = _geometry->sectorBytes;
    const auto at = _offset + lsn * bytes;
    const auto got = file.seek(at) ? int(file.read(sector_buffer, bytes)) : 0;
    for (auto i = got < 0 ? 0 : got; i < int(bytes); ++i)
        sector_buffer[i] = _geometry->fill;
    Watchdog::feed();
    return true;
}

bool DiskUnit::write(uint32_t lsn) {
    if (!mounted() || lsn >= lsns())
        return false;
    auto &file = _file->file;
    const auto bytes = _geometry->sectorBytes;
    const auto at = _offset + lsn * bytes;
    if (!grow(at) || !file.seek(at) || file.write(sector_buffer, bytes) != bytes)
        return false;
    Watchdog::feed();
    return true;
}

bool DiskUnit::flush() {
    if (!mounted())
        return false;
    _file->file.flush();
    return true;
}

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
