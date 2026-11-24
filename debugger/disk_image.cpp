#include "disk_image.h"
#include <SD.h>
#include <string.h>
#include <strings.h>
#include "debugger.h"
#include "disk_cache.h"
#include "watchdog.h"

namespace debugger {

// An open image file, and the cache's store for it.
struct DiskFile : BlockStore {
    File file;
    char path[64];
    uint8_t refs;

    uint32_t size() override { return file.size(); }
    bool read(uint32_t at, uint8_t *data, uint16_t len) override {
        const auto ok = file.seek(at) && file.read(data, len) == int(len);
        Watchdog::feed();
        return ok;
    }
    bool write(uint32_t at, const uint8_t *data, uint16_t len) override {
        const auto ok = file.seek(at) && file.write(data, len) == len;
        Watchdog::feed();
        return ok;
    }
    bool flush() override {
        file.flush();
        return true;
    }
};

namespace {
DiskFile files[DiskCache::FILES];
DiskCache block_cache;
uint8_t sector_buffer[DiskUnit::BUFFER_BYTES];

uint8_t indexOf(const DiskFile *f) {
    return f - files;
}

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
    block_cache.attach(indexOf(free), free);
    return free;
}

// Blocks the card would not take are lost with the handle.
void closeFile(DiskFile *f) {
    if (f && --f->refs == 0) {
        const auto left = block_cache.dirty(indexOf(f));
        if (!block_cache.detach(indexOf(f))) {
            cli.print("disk: ");
            cli.print(left);
            cli.print(" blocks lost on ");
            cli.println(f->path);
        }
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
    const auto size = _file->file.size();
    const auto bytes = size <= offset ? 0 : size - offset;
    _bytes = bytes < geometry->bytes() ? bytes : geometry->bytes();
    const auto blocks = block_cache.preload(indexOf(_file), offset, _bytes);
    const auto cached = blocks * DiskCache::BLOCK;
    _cachedBytes = cached < _bytes ? cached : _bytes;
    return true;
}

void DiskUnit::unmount() {
    closeFile(_file);
    _file = nullptr;
    _geometry = nullptr;
    _offset = 0;
    _bytes = _cachedBytes = 0;
}

const char *DiskUnit::path() const {
    return _file ? _file->path : "";
}

bool DiskUnit::lsn(
        uint16_t cylinder, uint8_t head, uint16_t sector, uint32_t &lsn) const {
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
    block_cache.wrote(indexOf(_file));
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
    const auto bytes = _geometry->sectorBytes;
    uint16_t got;
    if (!block_cache.read(indexOf(_file), _offset + lsn * bytes, sector_buffer,
                bytes, got))
        return false;
    memset(sector_buffer + got, _geometry->fill, bytes - got);
    return true;
}

// The file reaches the sector before the cache takes it, so a dirty block
// never starts past the file's end.
bool DiskUnit::write(uint32_t lsn) {
    if (!mounted() || lsn >= lsns())
        return false;
    const auto bytes = _geometry->sectorBytes;
    const auto at = _offset + lsn * bytes;
    return grow(at) &&
           block_cache.write(indexOf(_file), at, sector_buffer, bytes);
}

bool DiskUnit::flush() {
    return mounted() && block_cache.flush(indexOf(_file));
}

bool CardFile::open(const char *path, bool create) {
    close();
    if (strstr(path, ".."))
        return false;
    _file = openFile(path, create);
    if (_file == nullptr)
        return false;
    block_cache.sync(indexOf(_file));
    block_cache.drop(indexOf(_file));
    return true;
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
    if (_file == nullptr)
        return false;
    block_cache.invalidate(indexOf(_file), at, len);
    return _file->file.seek(at) && _file->file.write(data, len) == len;
}

bool CardFile::truncate(uint32_t size) {
    if (_file == nullptr)
        return false;
    const auto was = _file->file.size();
    if (was <= size)
        return true;
    block_cache.invalidate(indexOf(_file), size, was - size);
    return _file->file.truncate(size);
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

DiskCache &DiskImages::cache() {
    return block_cache;
}

void DiskImages::beginCache(uint8_t *arena, uint32_t bytes) {
    block_cache.begin(arena, bytes);
}

void DiskImages::sync() {
    for (auto &f : files) {
        const auto i = indexOf(&f);
        if (f.refs && !block_cache.sync(i) && block_cache.dirty(i)) {
            cli.print("disk: ");
            cli.print(block_cache.dirty(i));
            cli.print(" blocks not written to ");
            cli.println(f.path);
        }
    }
}

void DiskImages::printCache() {
    cli.print("  cache ");
    if (block_cache.capacity() == 0) {
        cli.println("none: no memory to spare");
        return;
    }
    if (!block_cache.on()) {
        cli.println("off");
        return;
    }
    const auto tenths = (block_cache.capacity() + 52428) / 104858;  // of a MB
    cli.print(tenths / 10);
    cli.print('.');
    cli.print(tenths % 10);
    cli.print(block_cache.policy() == DiskCache::WRITE_BACK
                      ? " MB write-back: "
                      : " MB write-through: ");
    cli.print(block_cache.hits());
    cli.print(" hits, ");
    cli.print(block_cache.misses());
    cli.print(" misses, ");
    cli.print(block_cache.dirty());
    cli.println(" dirty");
}

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
