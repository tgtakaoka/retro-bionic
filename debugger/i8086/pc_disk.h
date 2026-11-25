#ifndef __PC_DISK_H__
#define __PC_DISK_H__

#include "sector_disk.h"

namespace debugger {
namespace i8086 {

/**
 * The disk of the 8086 targets: CP/M-86's, which CP/M-80 in 8080
 * emulation shares, with A: its boot disk, and the PC BIOS's
 * (samples/i8086/rombios.asm), which boots MS-DOS from A: or, with A:
 * empty, from the hard disk C:.
 */
struct PcDisk final : SectorDisk {
    PcDisk(const Mems *mems);

    // |path|'s a.dsk, b.dsk, c.dsk and d.dsk as A-D, on while a boot
    // disk, A: or C:, is mounted.
    void mountBeside(const char *path) override;
};

}  // namespace i8086
}  // namespace debugger
#endif /* __PC_DISK_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
