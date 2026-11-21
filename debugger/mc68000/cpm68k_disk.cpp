#include "cpm68k_disk.h"
#include <string.h>

namespace debugger {
namespace mc68000 {

namespace {
// CP/M's disk formats, by the index DRIVE reads back, which
// cpm68kbios.asm counts on: ibm-3740 holds any image up to 256256 bytes,
// hd8 an image of exactly 8 MB, with the DPB every CP/M gives it.
const DiskGeometry CPM68K_FORMATS[] = {
        {"ibm-3740", 77, 1, 26, 128, 1, 0xE5},  // 8" SSSD, sectors from 1
        {"hd8", 256, 1, 256, 128, 0, 0xE5},     // 8 MB, sectors from 0
};
}  // namespace

Cpm68kDisk::Cpm68kDisk(const Mems *mems) : SectorDisk(mems, CPM68K_FORMATS, 2) {}

void Cpm68kDisk::mountBeside(const char *path) {
    const auto dot = strrchr(path, '.');
    const auto slash = strrchr(path, '/');
    const size_t stem = (dot && dot > slash) ? dot - path : strlen(path);
    char name[64];
    for (auto d = 0; d < DRIVES; ++d) {
        if (stem + sizeof("a.dsk") > sizeof(name)) {
            unmountUnit(d);
            continue;
        }
        memcpy(name, path, stem);
        name[stem] = 0;
        strncat(name, "abcd" + d, 1);
        strcat(name, ".dsk");
        mountDrive(d, name);
    }
    enable(unit(0).mounted());
}

}  // namespace mc68000
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
