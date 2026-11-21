#include "disk_drives.h"
#include <SD.h>
#include <string.h>
#include <strings.h>
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
        if (i == 0 ? bytes <= _formats[0].bytes() : bytes == _formats[i].bytes()) {
            format = &_formats[i];
            return true;
        }
    }
    return false;
}

Disk::Mounted DiskDrives::mountUnit(
        uint8_t unit, const char *label, const char *path) {
    _units[unit].unmount();
    *_labels[unit] = 0;
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
    strncpy(_labels[unit], label, LABEL_MAX);
    _labels[unit][LABEL_MAX] = 0;
    return MOUNT_OK;
}

Disk::Mounted DiskDrives::mount(const char *label, const char *path) {
    auto unit = _count;
    for (auto u = 0; u < _count; ++u) {
        if (_units[u].mounted() && strcasecmp(_labels[u], label) == 0) {
            unit = u;
            break;
        }
        if (!_units[u].mounted() && unit == _count)
            unit = u;
    }
    if (unit == _count)
        return MOUNT_NO_UNIT;
    return mountUnit(unit, label, path);
}

bool DiskDrives::unmount(const char *label) {
    for (auto u = 0; u < _count; ++u) {
        if (_units[u].mounted() && strcasecmp(_labels[u], label) == 0) {
            _units[u].unmount();
            *_labels[u] = 0;
            return true;
        }
    }
    return false;
}

void DiskDrives::printDrives() const {
    auto none = true;
    for (auto u = 0; u < _count; ++u) {
        const auto &unit = _units[u];
        if (!unit.mounted())
            continue;
        cli.print("  ");
        cli.printStr(_labels[u], -int8_t(LABEL_MAX));
        cli.print(' ');
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
        cli.println();
        none = false;
    }
    if (none)
        cli.println("  no disk mounted");
}

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
