#include "pc_disk.h"
#include <string.h>

namespace debugger {
namespace i8086 {

namespace {
// The formats by the index DRIVE reads back: CP/M's two first, which
// cpm86bios.asm counts on, ibm-3740 holding any image up to 256256 bytes
// and hd8 one of exactly 8 MB, with CP/M-80's DPBs; then the DOS floppies,
// each an image of exactly its size, and the 32 MB hard disk of 65536
// sectors, linear, which the PC BIOS addresses as 1024 x 4 x 16.
// scripts/msdos-disk.py writes the DOS ones.
const DiskGeometry PC_FORMATS[] = {
        {"ibm-3740", 77, 1, 26, 128, 1, 0xE5},  // 8" SSSD, sectors from 1
        {"hd8", 256, 1, 256, 128, 0, 0xE5},     // 8 MB, sectors from 0
        {"fd360", 40, 2, 9, 512, 1, 0xF6},      // 5.25" DS DD
        {"fd720", 80, 2, 9, 512, 1, 0xF6},      // 3.5" DD
        {"fd1200", 80, 2, 15, 512, 1, 0xF6},    // 5.25" HD
        {"fd1440", 80, 2, 18, 512, 1, 0xF6},    // 3.5" HD
        {"hd32", 256, 1, 256, 512, 0, 0x00},    // 32 MB, sectors from 0
};
constexpr uint8_t FORMATS = sizeof(PC_FORMATS) / sizeof(PC_FORMATS[0]);
}  // namespace

PcDisk::PcDisk(const Mems *mems) : SectorDisk(mems, PC_FORMATS, FORMATS) {}

void PcDisk::mountBeside(const char *path) {
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
    enable(unit(0).mounted() || unit(2).mounted());
}

}  // namespace i8086
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
