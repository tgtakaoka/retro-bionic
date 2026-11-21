#ifndef __DEBUGGER_DISK_H__
#define __DEBUGGER_DISK_H__

#include <stddef.h>
#include <stdint.h>

namespace debugger {

/**
 * Disk drives whose media are image files on the SD card, mounted at the
 * prompt: by O on the drive it names, and by L as the set beside the file
 * it loaded. A drive goes by its name in the OS, such as B or rk1.
 */
struct Disk {
    enum Mounted : uint8_t {
        MOUNT_OK,
        MOUNT_NOT_FOUND,  // no such file, which is never made
        MOUNT_NO_UNIT,    // no drive has the name
        MOUNT_BAD_SIZE,   // no format has the file's size
    };

    virtual ~Disk() {}

    // Mounts the image file at |path| on drive |name|.
    virtual Mounted mount(const char *name, const char *path) = 0;
    // False when drive |name| has no image.
    virtual bool unmount(const char *name) = 0;
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
