#ifndef __DEBUGGER_DISK_H__
#define __DEBUGGER_DISK_H__

#include <stddef.h>
#include <stdint.h>

namespace debugger {

/**
 * Disk drives whose media are image files on the SD card, mounted at the
 * prompt: by O under a label its user picks, and by L as the set beside
 * the file it loaded. The device puts each on a unit of its own.
 */
struct Disk {
    // The longest label, which has no spaces.
    static constexpr size_t LABEL_MAX = 15;

    enum Mounted : uint8_t {
        MOUNT_OK,
        MOUNT_NOT_FOUND,  // no such file, which is never made
        MOUNT_NO_UNIT,    // every unit carries another label
        MOUNT_BAD_SIZE,   // no format has the file's size
    };

    virtual ~Disk() {}

    // Mounts the image file at |path| under |label|: on the unit that
    // already carries |label|, else on the first free one.
    virtual Mounted mount(const char *label, const char *path) = 0;
    // False when no unit carries |label|.
    virtual bool unmount(const char *label) = 0;
    // Mounts the images that go with |path|, a file loaded from the card.
    virtual void mountBeside(const char *path) = 0;
    virtual void printDrives() const = 0;
};

}  // namespace debugger
#endif /* __DEBUGGER_DISK_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
