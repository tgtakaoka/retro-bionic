#include "disk_image.h"
#include <SD.h>
#include <string.h>
#include <strings.h>
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

// A path names its file one way: without the leading slashes.
const char *cardPath(const char *path) {
    while (*path == '/')
        ++path;
    return path;
}

// The open handle on |path|, if any; the card's names know no case.
DiskFile *findFile(const char *path) {
    for (auto &f : files) {
        if (f.refs && strcasecmp(f.path, path) == 0)
            return &f;
    }
    return nullptr;
}

DiskFile *openFile(const char *path, bool create = false) {
    path = cardPath(path);
    if (const auto f = findFile(path)) {
        ++f->refs;
        return f;
    }
    DiskFile *free = nullptr;
    for (auto &f : files) {
        if (f.refs == 0) {
            free = &f;
            break;
        }
    }
    if (free == nullptr || *path == 0 || strlen(path) >= sizeof(free->path))
        return nullptr;
    if (!SD.exists(path)) {
        if (!create)
            return nullptr;
        strcpy(free->path, path);
        const auto slash = strrchr(free->path, '/');
        if (slash) {
            *slash = 0;
            SD.mkdir(free->path);
        }
        *free->path = 0;
    }
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

// Only the part of the sector past the file's end may come up short.
bool DiskUnit::read(uint32_t lsn) {
    if (!mounted() || lsn >= lsns())
        return false;
    auto &file = _file->file;
    const auto bytes = _geometry->sectorBytes;
    const auto at = _offset + lsn * bytes;
    const auto size = file.size();
    const auto want = size <= at ? 0 : (size - at < bytes ? size - at : bytes);
    if (want && (!file.seek(at) || file.read(sector_buffer, want) != int(want)))
        return false;
    memset(sector_buffer + want, _geometry->fill, bytes - want);
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

bool CardFile::open(const char *path, bool create) {
    close();
    if (strstr(path, ".."))
        return false;
    _file = openFile(path, create);
    return _file != nullptr;
}

void CardFile::close() {
    closeFile(_file);
    _file = nullptr;
}

uint32_t CardFile::size() const {
    return _file ? _file->file.size() : 0;
}

uint16_t CardFile::read(uint32_t at, uint8_t *data, uint16_t len) {
    if (_file == nullptr || !_file->file.seek(at))
        return 0;
    const auto got = int(_file->file.read(data, len));
    return got < 0 ? 0 : got;
}

bool CardFile::write(uint32_t at, const uint8_t *data, uint16_t len) {
    return _file && _file->file.seek(at) && _file->file.write(data, len) == len;
}

bool CardFile::truncate(uint32_t size) {
    return _file && (_file->file.size() <= size || _file->file.truncate(size));
}

void CardFile::flush() {
    if (_file)
        _file->file.flush();
}

CardFile::Removed CardFile::remove(const char *path) {
    path = cardPath(path);
    if (strstr(path, "..") || !SD.exists(path))
        return NOT_FOUND;
    if (findFile(path))
        return IN_USE;
    return SD.remove(path) ? REMOVED : IN_USE;
}

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
