#include "rk11.h"
#include <string.h>
#include "debugger.h"
#include "watchdog.h"

namespace debugger {
namespace pdp11 {

namespace {
// RKDS
constexpr uint16_t DS_SC_EQ_SA = 0000020;  // the sector is under the heads
constexpr uint16_t DS_WPS = 0000040;       // write protected
constexpr uint16_t DS_RWS_RDY = 0000100;   // read/write/seek ready
constexpr uint16_t DS_DRY = 0000200;       // drive ready
constexpr uint16_t DS_SOK = 0000400;       // sector counter OK
constexpr uint16_t DS_RK05 = 0004000;
// RKER
constexpr uint16_t ER_WCE = 0000001;  // write check: memory and disk differ
constexpr uint16_t ER_NXS = 0000040;  // non-existent sector
constexpr uint16_t ER_NXC = 0000100;  // non-existent cylinder
constexpr uint16_t ER_NXD = 0000200;  // non-existent disk
constexpr uint16_t ER_NXM = 0002000;  // non-existent memory
constexpr uint16_t ER_WLO = 0020000;  // a write to a locked drive
constexpr uint16_t ER_DRE = 0100000;  // drive error: the card failed
constexpr uint16_t ER_HARD = 0177740;
// RKCS
constexpr uint16_t CS_GO = 0000001;
constexpr uint16_t CS_MEX = 0000060;  // RKBA's bits 17 and 16
constexpr uint16_t CS_IE = 0000100;
constexpr uint16_t CS_RDY = 0000200;
constexpr uint16_t CS_SSE = 0000400;  // stop on a soft error
constexpr uint16_t CS_IBA = 0004000;  // inhibit RKBA's increment
constexpr uint16_t CS_SCP = 0020000;  // search complete
constexpr uint16_t CS_HE = 0040000;
constexpr uint16_t CS_ERR = 0100000;
constexpr uint16_t CS_RW = 0007576;   // what a program writes
// RKCS function
constexpr uint8_t FN_RESET = 0;
constexpr uint8_t FN_WRITE = 1;
constexpr uint8_t FN_READ = 2;
constexpr uint8_t FN_WRITE_CHECK = 3;
constexpr uint8_t FN_SEEK = 4;
constexpr uint8_t FN_READ_CHECK = 5;
constexpr uint8_t FN_DRIVE_RESET = 6;
constexpr uint8_t FN_WRITE_LOCK = 7;

constexpr uint8_t SECTORS = 12;
constexpr uint16_t CYLINDERS = 203;
constexpr uint16_t SECTOR_WORDS = 256;
// Past an image's end a drive reads zeros.
constexpr DiskGeometry RK05 = {"RK05", CYLINDERS, 2, SECTORS, 2 * SECTOR_WORDS, 0, 0};
// The slotted image's blocks: the drum's, then a drive's slot.
constexpr uint32_t RK_START = 1024;
constexpr uint32_t DRIVE_BLOCKS = 512 * SECTORS;
// The 18-bit bus's I/O page.
constexpr uint32_t IO_PAGE = 0760000;

uint8_t drive(uint16_t rkda) {
    return rkda >> 13;
}
}  // namespace

Rk11::Rk11(const Mems *mems)
    : DiskDrives(_drives, DRIVES, &RK05, 1), _mems(mems) {}

const char *Rk11::name() const {
    return "RK11";
}

const char *Rk11::description() const {
    return "RK11";
}

void Rk11::print() const {
    printDrives();
    cli.print("  RKER=");
    cli.printNum(_rker, 8, 6);
    cli.print(" RKCS=");
    cli.printNum(peek(_base_addr + RKCS), 8, 6);
    cli.print(" RKWC=");
    cli.printNum(_rkwc, 8, 6);
    cli.print(" RKBA=");
    cli.printNum(_rkba, 8, 6);
    cli.print(" RKDA=");
    cli.printNum(_rkda, 8, 6);
    cli.println(_request ? " requesting" : "");
}

const char *Rk11::unitName(uint8_t unit) const {
    static char name[] = "rk0";
    name[2] = '0' + unit;
    return name;
}

uint8_t Rk11::unitOf(const char *name) const {
    if ((name[0] | 0x20) != 'r' || (name[1] | 0x20) != 'k')
        return DRIVES;
    const auto d = name[2] - '0';
    return d >= 0 && d < DRIVES && name[3] == 0 ? d : DRIVES;
}

bool Rk11::place(uint8_t unit, uint32_t bytes, const DiskGeometry *&format,
        uint32_t &offset) const {
    format = &RK05;
    if (bytes <= RK05.bytes()) {
        offset = 0;
        return true;
    }
    const uint32_t drum = RK_START * RK05.sectorBytes;
    const uint32_t slot = DRIVE_BLOCKS * RK05.sectorBytes;
    if (bytes < drum + slot || (bytes - drum) % slot != 0 ||
            (bytes - drum) / slot > DRIVES)
        return false;
    offset = drum + unit * slot;
    return offset < bytes;
}

// As a file is loaded, never in a transfer, where a missing card would
// hold the CPU for long.
void Rk11::mountBeside(const char *path) {
    const auto dot = strrchr(path, '.');
    const auto slash = strrchr(path, '/');
    const size_t stem = (dot && dot > slash) ? dot - path : strlen(path);
    char name[64];
    auto d = 0;
    if (stem + sizeof(".dsk") <= sizeof(name)) {
        memcpy(name, path, stem);
        strcpy(name + stem, ".dsk");
        while (d < DRIVES && mountUnit(d, name) == MOUNT_OK)
            ++d;
    }
    for (; d < DRIVES; ++d)
        unmountUnit(d);
}

void Rk11::reset() {
    _rker = _rkcs = _rkwc = _rkba = _rkda = 0;
    _locked = 0;
    _ready = true;
    _done = false;
    acknowledge();
}

bool Rk11::loop() {
    if (!_enabled || !_done)
        return false;
    _done = false;
    _ready = true;
    if ((_rkcs & CS_IE) && !_request) {
        _request = true;
        Debugger.target().assertInt(VECTOR);
    }
    return true;
}

void Rk11::enable(bool enabled) {
    if (!enabled)
        reset();
    Device::enable(enabled);
}

void Rk11::acknowledge() {
    if (_request) {
        _request = false;
        Debugger.target().negateInt(VECTOR);
    }
}

// The sector counter steps on each look, as the pack turns under it.
uint16_t Rk11::read(uint32_t addr) {
    if (addr - _base_addr == RKDS && ++_sector == SECTORS)
        _sector = 0;
    return peek(addr);
}

uint16_t Rk11::peek(uint32_t addr) const {
    switch (addr - _base_addr) {
    case RKDS: {
        const auto d = drive(_rkda);
        return (d << 13) | DS_RK05 | DS_SOK |
               (unit(d).mounted() ? (DS_DRY | DS_RWS_RDY) : 0) |
               (((_locked >> d) & 1) ? DS_WPS : 0) |
               ((_rkda & 017) == _sector ? DS_SC_EQ_SA : 0) | _sector;
    }
    case RKER:
        return _rker;
    case RKCS:
        return _rkcs | (_ready ? CS_RDY : 0) | (_rker ? CS_ERR : 0) |
               ((_rker & ER_HARD) ? CS_HE : 0);
    case RKWC:
        return _rkwc;
    case RKBA:
        return _rkba;
    case RKDA:
        return _rkda;
    }
    return 0;
}

void Rk11::write(uint32_t addr, uint16_t data) {
    switch (addr - _base_addr) {
    case RKCS:
        _rkcs = (_rkcs & ~CS_RW) | (data & CS_RW);
        if ((data & CS_IE) == 0)
            acknowledge();
        if ((data & CS_GO) && _ready)
            start();
        break;
    case RKWC:
        _rkwc = data;
        break;
    case RKBA:
        _rkba = data;
        break;
    case RKDA:
        _rkda = data;
        break;
    }
}

void Rk11::start() {
    const uint8_t function = (_rkcs >> 1) & 7;
    if (function == FN_RESET) {
        // A control reset keeps the drives' write locks.
        const auto locked = _locked;
        reset();
        _locked = locked;
        return;
    }
    _ready = false;
    _done = true;
    _rker = 0;
    _rkcs &= ~CS_SCP;
    const uint8_t bit = 1 << drive(_rkda);
    if (function == FN_WRITE_LOCK) {
        _locked |= bit;
    } else if (function == FN_SEEK || function == FN_DRIVE_RESET) {
        if (function == FN_DRIVE_RESET)
            _locked &= ~bit;
        _rkcs |= CS_SCP;
    } else if (function == FN_WRITE && (_locked & bit)) {
        _rker |= ER_WLO;
    } else {
        transfer(function);
    }
}

// RKWC is the words left, negative; RKBA and RKDA end past the last
// word and sector. A write's last sector is filled with zeros. A write
// check compares the memory with the disk, a read check only reads.
void Rk11::transfer(uint8_t function) {
    const auto toDisk = function == FN_WRITE;
    const uint8_t sect = _rkda & 017;
    const uint16_t cyl = (_rkda >> 5) & 0377;
    if (sect >= SECTORS) {
        _rker |= ER_NXS;
        return;
    }
    if (cyl >= CYLINDERS) {
        _rker |= ER_NXC;
        return;
    }
    auto &disk = unit(drive(_rkda));
    if (!disk.mounted()) {
        _rker |= ER_NXD;
        return;
    }
    uint32_t block = ((_rkda >> 4) & 0777) * SECTORS + sect;
    uint32_t addr = (uint32_t(_rkcs & CS_MEX) << 12) | (_rkba & ~1);
    uint32_t words = 0200000 - _rkwc;
    const auto sector = DiskUnit::buffer();
    while (words != 0 && block < disk.lsns()) {
        const auto n = words < SECTOR_WORDS ? words : SECTOR_WORDS;
        if (addr + 2 * n > IO_PAGE) {
            _rker |= ER_NXM;
            break;
        }
        if (toDisk) {
            for (auto i = 0U; i < SECTOR_WORDS; ++i) {
                const uint16_t w = i < n ? _mems->read16(addr + 2 * i) : 0;
                sector[2 * i] = w;
                sector[2 * i + 1] = w >> 8;
            }
            if (!disk.write(block)) {
                _rker |= ER_DRE;
                break;
            }
        } else {
            if (!disk.read(block)) {
                _rker |= ER_DRE;
                break;
            }
            for (auto i = 0U; i < n && function != FN_READ_CHECK; ++i) {
                const uint16_t w = sector[2 * i] | (sector[2 * i + 1] << 8);
                if (function == FN_READ) {
                    _mems->write16(addr + 2 * i, w);
                } else if (_mems->read16(addr + 2 * i) != w) {
                    _rker |= ER_WCE;
                }
            }
        }
        words -= n;
        addr += 2 * n;
        ++block;
        Watchdog::feed();
        if ((_rker & ER_WCE) && (_rkcs & CS_SSE))
            break;
    }
    if (toDisk && !disk.flush())
        _rker |= ER_DRE;
    if (words != 0 && block >= disk.lsns())
        _rker |= ER_NXC;  // ran off the last cylinder
    _rkwc = -words;
    if ((_rkcs & CS_IBA) == 0) {
        _rkba = addr;
        _rkcs = (_rkcs & ~CS_MEX) | ((addr >> 12) & CS_MEX);
    }
    _rkda = (_rkda & 0160000) | ((block / SECTORS) << 4) | (block % SECTORS);
}

}  // namespace pdp11
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
