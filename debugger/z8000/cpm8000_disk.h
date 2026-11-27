#ifndef __CPM8000_DISK_H__
#define __CPM8000_DISK_H__

#include "sector_disk.h"

namespace debugger {
namespace z8000 {

/**
 * The CP/M-8000 disk of the Z8002 and Z8001 targets, 10H above the
 * USART in the I/O space. A: is the boot disk.
 */
struct Cpm8000Disk final : SectorDisk {
    Cpm8000Disk(const Mems *mems);

    // |path|'s a.dsk, b.dsk, c.dsk and d.dsk as A-D.
    void mountBeside(const char *path) override;
};

}  // namespace z8000
}  // namespace debugger
#endif /* __CPM8000_DISK_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
