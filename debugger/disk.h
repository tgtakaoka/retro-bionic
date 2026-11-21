#ifndef __DEBUGGER_DISK_H__
#define __DEBUGGER_DISK_H__

#include <stdint.h>

namespace debugger {

/**
 * Disk drives whose media are image files on the SD card, mounted at the
 * prompt: by O one drive at a time, and by L as the set beside the file
 * it loaded.
 */
struct Disk {
    virtual ~Disk() {}

    virtual uint8_t drives() const = 0;
    // Mounts the image file at |path| as |drive|; false when there is no
    // such file, which is never made.
    virtual bool mount(uint8_t drive, const char *path) = 0;
    virtual void unmount(uint8_t drive) = 0;
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
