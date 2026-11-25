#ifndef __CPM86_DISK_H__
#define __CPM86_DISK_H__

#include "sector_disk.h"

namespace debugger {

/**
 * The CP/M-86 disk of the 8086 targets, which CP/M-80 in 8080 emulation
 * shares. A: is the boot disk.
 */
struct Cpm86Disk final : SectorDisk {
    Cpm86Disk(const Mems *mems);

    // |path|'s a.dsk, b.dsk, c.dsk and d.dsk as A-D.
    void mountBeside(const char *path) override;
};

}  // namespace debugger
#endif /* __CPM86_DISK_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
