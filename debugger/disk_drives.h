#ifndef __DEBUGGER_DISK_DRIVES_H__
#define __DEBUGGER_DISK_DRIVES_H__

#include "disk.h"
#include "disk_image.h"

namespace debugger {

/**
 * The drives O and L mount: |count| units on |formats|. A front-end names
 * its units and may place images its own way.
 */
struct DiskDrives : Disk {
    static constexpr uint8_t UNITS_MAX = 8;

    DiskDrives(DiskUnit *units, uint8_t count, const DiskGeometry *formats,
            uint8_t formatCount)
        : _units(units),
          _count(count),
          _formats(formats),
          _formatCount(formatCount) {}

    Mounted mount(const char *label, const char *path) override;
    bool unmount(const char *label) override;
    void printDrives() const override;

    DiskUnit &unit(uint8_t unit) { return _units[unit]; }
    const DiskUnit &unit(uint8_t unit) const { return _units[unit]; }
    uint8_t units() const { return _count; }
    // The index of |unit|'s format in |formats|, 0 when it is not mounted.
    uint8_t formatIndex(uint8_t unit) const;

protected:
    // Such as "B:".
    virtual const char *unitName(uint8_t unit) const = 0;
    // Up to the first format's size takes the first, else the format of
    // exactly that size; false when none fits.
    virtual bool place(uint32_t bytes, const DiskGeometry *&format,
            uint32_t &offset) const;
    // Mounts |path| on |unit| under |label|, placed by its size.
    Mounted mountUnit(uint8_t unit, const char *label, const char *path);

private:
    DiskUnit *const _units;
    const uint8_t _count;
    const DiskGeometry *const _formats;
    const uint8_t _formatCount;
    char _labels[UNITS_MAX][LABEL_MAX + 1] = {};
};

}  // namespace debugger
#endif /* __DEBUGGER_DISK_DRIVES_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
