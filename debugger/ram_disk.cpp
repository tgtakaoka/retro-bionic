#include "ram_disk.h"
#include <DMAChannel.h>
#include <string.h>
#include "debugger.h"
#include "disk_cache.h"
#include "mems.h"

namespace debugger {

namespace {
constexpr uint8_t DRIVE = 0;
constexpr uint8_t LSNH = 1;
constexpr uint8_t LSNM = 2;
constexpr uint8_t LSNL = 3;
constexpr uint8_t ADDRH = 4;
constexpr uint8_t ADDRL = 5;
constexpr uint8_t COMMAND = 6;
constexpr uint8_t INTR = 7;
constexpr uint8_t COUNT = 8;

constexpr uint8_t CMD_ACK = 0;
constexpr uint8_t CMD_READ = 1;
constexpr uint8_t CMD_WRITE = 2;

constexpr uint8_t ST_DONE = 0x80;
constexpr uint8_t ST_BUSY = 0x01;
constexpr uint8_t ERR_NONE = 0;
constexpr uint8_t ERR_UNIT = 1;
constexpr uint8_t ERR_LSN = 2;
constexpr uint8_t ERR_ADDR = 3;
constexpr uint8_t ERR_CACHE = 4;
constexpr uint8_t ERR_COMMAND = 5;

// One channel moves bytes between the cache's arena and target memory:
// a single minor loop of the whole run, started by software.
DMAChannel dma(false);
bool dmaBegun = false;
}  // namespace

const char *RamDisk::name() const {
    return "DISK";
}

const char *RamDisk::description() const {
    return "RAM disk";
}

void RamDisk::print() const {
    const auto radix = Debugger.target().inputRadix();
    cli.print("  #IRQ: ");
    cli.printNum(_intr, radix, Debugger::numDigits(8, radix));
    cli.println();
    printDrives();
}

const char *RamDisk::unitName(uint8_t unit) const {
    static constexpr const char *NAMES[DRIVES] = {"D0", "D1", "D2", "D3"};
    return unit < DRIVES ? NAMES[unit] : "";
}

uint8_t RamDisk::unitOf(const char *name) const {
    if ((name[0] | 0x20) != 'd' || name[2] != 0)
        return DRIVES;
    const auto d = name[1] - '0';
    return d >= 0 && d < DRIVES ? d : DRIVES;
}

// The cache holds the whole image, or the mount is undone: a miss in a
// run would reach for the card while the CPU cannot wait.
bool RamDisk::admit(uint8_t unit) {
    if (!this->unit(unit).mounted())
        return false;
    if (!this->unit(unit).cached()) {
        cli.print(this->unit(unit).path());
        cli.println(": not whole in the disk cache");
        unmountUnit(unit);
        return false;
    }
    DiskImages::cache().setPolicy(DiskCache::WRITE_BACK);
    enable(true);
    return true;
}

Disk::Mounted RamDisk::mount(const char *name, const char *path) {
    const auto mounted = DiskDrives::mount(name, path);
    if (mounted == MOUNT_OK)
        admit(unitOf(name));
    return mounted;
}

void RamDisk::mountBeside(const char *path) {
    const auto dot = strrchr(path, '.');
    const auto slash = strrchr(path, '/');
    const size_t stem = (dot && dot > slash) ? dot - path : strlen(path);
    char name[64];
    for (auto d = 0; d < DRIVES; ++d) {
        if (stem + sizeof("d0.dsk") > sizeof(name)) {
            unmountUnit(d);
            continue;
        }
        memcpy(name, path, stem);
        name[stem] = 0;
        strcat(name, "d0.dsk");
        name[stem + 1] += d;
        if (mountUnit(d, name) == MOUNT_BAD_SIZE) {
            cli.print(name);
            cli.println(": no disk format of its size");
        }
        admit(d);
    }
}

void RamDisk::reset() {
    settle();
    acknowledge();
    _drive = 0;
    _lsn = 0;
    _addr = 0;
    _intr = 0;
    _count = 1;
    _error = ERR_NONE;
}

bool RamDisk::loop() {
    if (!_enabled)
        return false;
    if (_busy && dmaDone())
        step();
    return true;
}

// A DMA takes microseconds; one that never ends is given up on.
void RamDisk::settle() {
    const auto start = micros();
    while (_busy) {
        if (dmaDone()) {
            step();
        } else if (micros() - start > 10000) {
            _dst = nullptr;
            finish(ERR_CACHE);
        }
    }
}

uint16_t RamDisk::peek(uint32_t addr) const {
    switch (addr - _base_addr) {
    case DRIVE: {
        uint8_t bits = 0;
        for (auto d = 0; d < DRIVES; ++d) {
            if (unit(d).mounted())
                bits |= 0x10 << d;
        }
        return bits | (_drive & 0x0F);
    }
    case LSNH:
        return (_lsn >> 16) & 0xFF;
    case LSNM:
        return (_lsn >> 8) & 0xFF;
    case LSNL:
        return _lsn & 0xFF;
    case ADDRH:
        return _addr >> 8;
    case ADDRL:
        return _addr & 0xFF;
    case COMMAND:
        return (_done ? ST_DONE : 0) | (_busy ? ST_BUSY : 0) | (_error << 1);
    case INTR:
        return _intr;
    case COUNT:
        return _count;
    default:
        return 0xFF;
    }
}

void RamDisk::write(uint32_t addr, uint16_t data) {
    data &= 0xFF;
    switch (addr - _base_addr) {
    case DRIVE:
        _drive = data;
        break;
    case LSNH:
        _lsn = (_lsn & 0x00FFFF) | (uint32_t(data) << 16);
        break;
    case LSNM:
        _lsn = (_lsn & 0xFF00FF) | (uint32_t(data) << 8);
        break;
    case LSNL:
        _lsn = (_lsn & 0xFFFF00) | data;
        break;
    case ADDRH:
        _addr = (_addr & 0x00FF) | (data << 8);
        break;
    case ADDRL:
        _addr = (_addr & 0xFF00) | data;
        break;
    case COMMAND:
        command(data);
        break;
    case INTR: {
        // DONE waiting on the old name moves to the new one.
        const auto done = _done;
        acknowledge();
        _intr = data;
        if (done) {
            _done = true;
            if (_intr)
                Debugger.target().assertInt(_intr);
        }
        break;
    }
    case COUNT:
        _count = data ? data : 1;
        break;
    }
}

void RamDisk::acknowledge() {
    if (_done) {
        _done = false;
        if (_intr)
            Debugger.target().negateInt(_intr);
    }
}

// A command while one runs waits for it first, so each one's DONE is
// seen.
void RamDisk::command(uint8_t cmd) {
    settle();
    acknowledge();
    if (cmd == CMD_ACK)
        return;
    if (cmd != CMD_READ && cmd != CMD_WRITE) {
        finish(ERR_COMMAND);
        return;
    }
    if (_drive >= DRIVES || !unit(_drive).mounted()) {
        finish(ERR_UNIT);
        return;
    }
    _toDisk = cmd == CMD_WRITE;
    _left = _count;
    _busy = true;
    step();
}

// Moves what the running DMA left to do: the sectors of one cache
// block at a time, as far as the count goes; then DONE.
void RamDisk::step() {
    if (_dst != nullptr) {
        // The DMA wrote behind the data cache: drop what it holds there.
        arm_dcache_delete(_dst, _len);
        _dst = nullptr;
    }
    if (_left == 0) {
        finish(ERR_NONE);
        return;
    }
    auto &drive = unit(_drive);
    const auto sector = drive.geometry()->sectorBytes;
    if (_lsn >= drive.lsns()) {
        finish(ERR_LSN);
        return;
    }
    // The bytes to the end of this cache block, and not past memory.
    const auto at = drive.offset() + _lsn * sector;
    auto len = DiskCache::BLOCK - uint16_t(at % DiskCache::BLOCK);
    if (len > _left * sector)
        len = _left * sector;
    if (uint32_t(_addr) + len > _memoryBytes) {
        finish(ERR_ADDR);
        return;
    }
    const auto image = drive.cached(_lsn, len, _toDisk);
    if (image == nullptr) {
        finish(ERR_CACHE);
        return;
    }
    const auto memory = DmaMemory::byteAt(_addr);
    _len = len;
    if (_toDisk) {
        _dst = image;
        dmaStart(memory, image, len);
    } else {
        _dst = memory;
        dmaStart(image, memory, len);
    }
    const auto sectors = len / sector;
    _lsn += sectors;
    _addr += len;
    _left -= sectors;
}

void RamDisk::finish(uint8_t error) {
    _busy = false;
    _left = 0;
    _error = error;
    _done = true;
    if (_intr)
        Debugger.target().assertInt(_intr);
}

bool RamDisk::dmaDone() const {
    return dma.complete();
}

// The source's lines go to memory first, the destination's are dropped
// so no stale or dirty line hides or overwrites what the DMA moves.
void RamDisk::dmaStart(const uint8_t *src, uint8_t *dst, uint16_t len) {
    if (!dmaBegun) {
        dma.begin(true);
        dmaBegun = true;
    }
    arm_dcache_flush(const_cast<uint8_t *>(src), len);
    arm_dcache_flush_delete(dst, len);
    dma.TCD->SADDR = src;
    dma.TCD->SOFF = 1;
    dma.TCD->ATTR = 0;  // 8-bit source and destination
    dma.TCD->NBYTES_MLNO = len;
    dma.TCD->SLAST = 0;
    dma.TCD->DADDR = dst;
    dma.TCD->DOFF = 1;
    dma.TCD->CITER_ELINKNO = 1;
    dma.TCD->DLASTSGA = 0;
    dma.TCD->BITER_ELINKNO = 1;
    dma.TCD->CSR = DMA_TCD_CSR_DREQ;
    dma.clearComplete();
    dma.triggerManual();
}

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
