#ifndef __CPM80_DISK_H__
#define __CPM80_DISK_H__

#include "sector_disk.h"

namespace debugger {

/**
 * The CP/M-80 disk of the 8080 and Z80 targets. A: is the boot disk.
 */
struct Cpm80Disk final : SectorDisk {
    Cpm80Disk(const Mems *mems);

    // |path|'s a.dsk, b.dsk, c.dsk and d.dsk as A-D.
    void mountBeside(const char *path) override;
};

}  // namespace debugger
#endif /* __CPM80_DISK_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
