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

    Mounted mount(const char *name, const char *path) override;
    bool unmount(const char *name) override;
    void printDrives() const override;

    DiskUnit &unit(uint8_t unit) { return _units[unit]; }
    const DiskUnit &unit(uint8_t unit) const { return _units[unit]; }
    uint8_t units() const { return _count; }
    bool anyMounted() const;
    // The index of |unit|'s format in |formats|, 0 when it is not mounted.
    uint8_t formatIndex(uint8_t unit) const;

protected:
    // Such as "B".
    virtual const char *unitName(uint8_t unit) const = 0;
    // The unit named |name|, units() when none is.
    virtual uint8_t unitOf(const char *name) const = 0;
    // Up to the first format's size takes the first, else the format of
    // exactly that size; false when none fits.
    virtual bool place(uint8_t unit, uint32_t bytes,
            const DiskGeometry *&format, uint32_t &offset) const;
    // Mounts |path| on |unit|, placed by its size. Prints
    // the drive's line when it mounts, or that the unit's previous image
    // is unmounted when it does not.
    Mounted mountUnit(uint8_t unit, const char *path);
    // Unmounts |unit|, saying so when it had an image.
    void unmountUnit(uint8_t unit);

private:
    DiskUnit *const _units;
    const uint8_t _count;
    const DiskGeometry *const _formats;
    const uint8_t _formatCount;

    Mounted mountImage(uint8_t unit, const char *path);
    void printDrive(uint8_t unit) const;
    void printUnmounted(uint8_t unit) const;
};

}  // namespace debugger
#endif /* __DEBUGGER_DISK_DRIVES_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
