#ifndef __DEBUGGER_DISK_IMAGE_H__
#define __DEBUGGER_DISK_IMAGE_H__

#include <stdint.h>

namespace debugger {

/**
 * A disk format: |cylinders| x |heads| x |sectors| a track of
 * |sectorBytes|, numbered from |firstSector|. Past an image file's end a
 * sector reads |fill|.
 */
struct DiskGeometry {
    const char *name;
    uint16_t cylinders;
    uint8_t heads;
    uint16_t sectors;
    uint16_t sectorBytes;
    uint8_t firstSector;  // 0 or 1
    uint8_t fill;

    uint32_t lsns() const { return uint32_t(cylinders) * heads * sectors; }
    uint32_t bytes() const { return lsns() * sectorBytes; }
};

/**
 * One drive's medium: |geometry| at |offset| in an SD card image file,
 * whose handle units on the same file share. Mounts happen at the prompt
 * and never make a file. read()/write()/flush() run inside a held bus
 * cycle, and fail on a card error, which the controller reports. Past
 * the file's end a read returns fill and a write first fills the gap.
 * The caller pads or trims partial sectors in buffer(). A RAM cache
 * would sit behind read(), write() and flush().
 */
struct DiskUnit {
    static constexpr uint16_t BUFFER_BYTES = 512;
    static uint8_t *buffer();

    // |reserve| grows the file to full size now, not inside a bus cycle.
    bool mount(const char *path, uint32_t offset, const DiskGeometry *geometry,
            bool reserve = false);
    void unmount();
    bool mounted() const { return _file != nullptr; }
    const DiskGeometry *geometry() const { return _geometry; }
    const char *path() const;
    uint32_t offset() const { return _offset; }
    uint32_t lsns() const { return _geometry ? _geometry->lsns() : 0; }
    // The logical sector of |cylinder|, |head| and |sector|; false when one
    // is out of range.
    bool lsn(uint16_t cylinder, uint8_t head, uint16_t sector,
            uint32_t &lsn) const;
    // Sector |lsn| between the image and buffer(); false when it is not
    // there or the card fails. write() leaves flushing to flush().
    bool read(uint32_t lsn);
    bool write(uint32_t lsn);
    bool flush();

private:
    struct DiskFile *_file = nullptr;
    uint32_t _offset = 0;
    const DiskGeometry *_geometry = nullptr;

    bool grow(uint32_t size);
};

/**
 * A card file for the debugger to read and write at the prompt. A mounted
 * image is reached through the handle its drives hold, so no second copy
 * of it goes out of step; a RAM cache would sit behind read() and write().
 */
struct CardFile {
    // |create| makes the file, and the directories on its way, when it is
    // not there. False for a path with a ".." or longer than a handle holds.
    bool open(const char *path, bool create = false);
    void close();
    uint32_t size() const;
    // The number of bytes read at |at|: fewer than |len| at the file's end.
    uint16_t read(uint32_t at, uint8_t *data, uint16_t len);
    // False past the file's end, which leaves no gap to fill.
    bool write(uint32_t at, const uint8_t *data, uint16_t len);
    // Cuts a longer file to |size|.
    bool truncate(uint32_t size);
    void flush();

    enum Removed : uint8_t { REMOVED, NOT_FOUND, IN_USE };
    // Deletes the file at |path|, unless a drive or a transfer holds it.
    static Removed remove(const char *path);

private:
    struct DiskFile *_file = nullptr;
};

}  // namespace debugger
#endif /* __DEBUGGER_DISK_IMAGE_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
