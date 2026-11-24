#include "disk_drives.h"
#include <SD.h>
#include <string.h>
#include "debugger.h"

namespace debugger {

uint8_t DiskDrives::formatIndex(uint8_t unit) const {
    const auto g = _units[unit].geometry();
    for (auto i = 0; i < _formatCount; ++i) {
        if (g == &_formats[i])
            return i;
    }
    return 0;
}

bool DiskDrives::place(uint8_t, uint32_t bytes, const DiskGeometry *&format,
        uint32_t &offset) const {
    offset = 0;
    for (auto i = 0; i < _formatCount; ++i) {
        if (i == 0 ? bytes <= _formats[0].bytes()
                   : bytes == _formats[i].bytes()) {
            format = &_formats[i];
            return true;
        }
    }
    return false;
}

Disk::Mounted DiskDrives::mountUnit(uint8_t unit, const char *path) {
    const auto was = _units[unit].mounted();
    const auto mounted = mountImage(unit, path);
    if (mounted == MOUNT_OK) {
        printDrive(unit);
    } else if (was) {
        printUnmounted(unit);
    }
    return mounted;
}

void DiskDrives::unmountUnit(uint8_t unit) {
    if (!_units[unit].mounted())
        return;
    _units[unit].unmount();
    printUnmounted(unit);
}

Disk::Mounted DiskDrives::mountImage(uint8_t unit, const char *path) {
    _units[unit].unmount();
    if (!SD.exists(path))
        return MOUNT_NOT_FOUND;
    uint32_t bytes;
    {
        auto file = SD.open(path);
        bytes = file.size();
        file.close();
    }
    const DiskGeometry *format;
    uint32_t offset;
    if (!place(unit, bytes, format, offset))
        return MOUNT_BAD_SIZE;
    if (!_units[unit].mount(path, offset, format))
        return MOUNT_NOT_FOUND;
    return MOUNT_OK;
}

Disk::Mounted DiskDrives::mount(const char *name, const char *path) {
    const auto u = unitOf(name);
    return u < _count ? mountUnit(u, path) : MOUNT_NO_UNIT;
}

bool DiskDrives::unmount(const char *name) {
    const auto u = unitOf(name);
    if (u >= _count || !_units[u].mounted())
        return false;
    unmountUnit(u);
    return true;
}

void DiskDrives::printDrive(uint8_t u) const {
    const auto &unit = _units[u];
    cli.print("  ");
    cli.printStr(unitName(u), -4);
    cli.print(' ');
    cli.printStr(unit.geometry()->name, -9);
    cli.print(' ');
    cli.print(unit.path());
    if (unit.offset()) {
        cli.print('+');
        cli.print(unit.offset() / 1024);
        cli.print('K');
    }
    if (unit.cached()) {
        cli.print(" cached");
    } else if (unit.cachedBytes()) {
        const auto tenths = (unit.cachedBytes() + 52428) / 104858;  // of a MB
        cli.print(' ');
        cli.print(tenths / 10);
        cli.print('.');
        cli.print(tenths % 10);
        cli.print(" MB cached");
    }
    cli.println();
}

void DiskDrives::printUnmounted(uint8_t unit) const {
    cli.print("  ");
    cli.print(unitName(unit));
    cli.println(" unmounted");
}

bool DiskDrives::anyMounted() const {
    for (auto u = 0; u < _count; ++u) {
        if (_units[u].mounted())
            return true;
    }
    return false;
}

void DiskDrives::printDrives() const {
    if (!anyMounted()) {
        cli.println("  no disk mounted");
        return;
    }
    for (auto u = 0; u < _count; ++u) {
        if (_units[u].mounted())
            printDrive(u);
    }
}

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
