#ifndef __CPM68K_DISK_H__
#define __CPM68K_DISK_H__

#include "sector_disk.h"

namespace debugger {
namespace mc68000 {

/**
 * The CP/M-68K disk of the 68000 boards. A: is the boot disk.
 */
struct Cpm68kDisk final : SectorDisk {
    Cpm68kDisk(const Mems *mems);

    // |path|'s a.dsk, b.dsk, c.dsk and d.dsk as A-D.
    void mountBeside(const char *path) override;
};

}  // namespace mc68000
}  // namespace debugger
#endif /* __CPM68K_DISK_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
