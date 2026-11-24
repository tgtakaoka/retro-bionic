#include "mscp.h"

namespace debugger {
namespace pdp11 {

namespace {
// SA, controller to host
constexpr uint16_t SA_ERR = 0100000;
constexpr uint16_t SA_STEP4 = 0040000;
constexpr uint16_t SA_STEP3 = 0020000;
constexpr uint16_t SA_STEP2 = 0010000;
constexpr uint16_t SA_STEP1 = 0004000;
constexpr uint16_t SA_Q22 = 0001000;    // 22-bit addresses
constexpr uint16_t SA_DIAG = 0000400;   // extended diagnostics
constexpr uint16_t SA_MAP = 0000100;    // address mapping
constexpr uint16_t SA_MODEL = 19 << 4;  // an RQDX3
constexpr uint16_t SA_VERSION = 3;
// SA, host to controller
constexpr uint16_t S1_VALID = 0100000;
constexpr uint16_t S1_WRAP = 0040000;
constexpr uint16_t S1_IE = 0000200;
constexpr uint16_t S2_PURGE_INT = 0000001;
constexpr uint16_t S3_PURGE_POLL = 0100000;
constexpr uint16_t S4_GO = 0000001;
// A ring descriptor: the packet's address and two bits.
constexpr uint32_t DESC_OWN = 0x80000000;  // the controller's
constexpr uint32_t DESC_FLAG = 0x40000000;
constexpr uint32_t DESC_ADDR = 0x003FFFFE;
// The communications area's words below the rings.
constexpr int8_t COMM_PURGE = -8;
constexpr int8_t COMM_CMD_INT = -4;
constexpr int8_t COMM_RSP_INT = -2;
// The 22-bit I/O page; a transfer never reaches it.
constexpr uint32_t IO_PAGE = 017760000;

// Packet words, from the text a descriptor points at; the length and the
// credits, type and connection id are the two words before it.
constexpr uint8_t P_UNIT = 2;
constexpr uint8_t P_OPCODE = 4;  // and the end flags
constexpr uint8_t P_STATUS = 5;  // the command's modifiers
constexpr uint8_t P_BYTES = 6;   // a transfer's count, address, block
constexpr uint8_t P_ADDR = 8;
constexpr uint8_t P_LBN = 14;
constexpr uint8_t P_VERSION = 6;  // set controller characteristics
constexpr uint8_t P_CFLAGS = 7;
constexpr uint8_t P_TIMEOUT = 8;
constexpr uint8_t P_CVERSION = 9;
constexpr uint8_t P_CID = 10;
constexpr uint8_t P_MAX_BYTES = 14;
constexpr uint8_t P_MULTI = 6;  // a unit's fields
constexpr uint8_t P_UFLAGS = 7;
constexpr uint8_t P_UID = 10;
constexpr uint8_t P_MEDIA = 14;
constexpr uint8_t P_SHADOW = 16;
constexpr uint8_t P_SIZE = 18;
constexpr uint8_t P_SERIAL = 20;
constexpr uint8_t P_TRACK = 18;  // get unit status
constexpr uint8_t P_GROUP = 19;
constexpr uint8_t P_CYLINDER = 20;
constexpr uint8_t P_UVERSION = 21;
constexpr uint8_t P_RCT_SIZE = 22;
constexpr uint8_t P_RBN = 23;
constexpr uint8_t PKT_WORDS = 30;
// Opcodes
constexpr uint8_t OP_GET_UNIT_STATUS = 003;
constexpr uint8_t OP_SET_CONTROLLER = 004;
constexpr uint8_t OP_AVAILABLE = 010;
constexpr uint8_t OP_ONLINE = 011;
constexpr uint8_t OP_READ = 041;
constexpr uint8_t OP_WRITE = 042;
constexpr uint8_t OP_END = 0200;
// End packet lengths
constexpr uint16_t LEN_END = 12;
constexpr uint16_t LEN_SET_CONTROLLER = 32;
constexpr uint16_t LEN_ONLINE = 44;
constexpr uint16_t LEN_GET_UNIT_STATUS = 48;
constexpr uint16_t LEN_READ_WRITE = 32;
// Status and its subcodes
constexpr uint16_t ST_SUCCESS = 0;
constexpr uint16_t ST_INVALID = 1;
constexpr uint16_t ST_OFFLINE = 3;
constexpr uint16_t ST_AVAILABLE = 4;
constexpr uint16_t ST_HOST = 9;
constexpr uint16_t ST_DRIVE = 11;
constexpr uint16_t SUB_ALREADY_ONLINE = 8 << 5;
constexpr uint16_t SUB_NO_VOLUME = 1 << 5;
constexpr uint16_t SUB_ODD_ADDR = 1 << 5;
constexpr uint16_t SUB_ODD_COUNT = 2 << 5;
constexpr uint16_t SUB_NXM = 3 << 5;
constexpr uint16_t INV_OPCODE = 8 << 8;
constexpr uint16_t INV_VERSION = 12 << 8;
constexpr uint16_t INV_COUNT = 12 << 8;
constexpr uint16_t INV_LBN = 28 << 8;
// Modifiers
constexpr uint16_t MD_NEXT_UNIT = 1;
// Set controller characteristics
constexpr uint16_t CF_REPLACE = 0100000;  // the controller replaces bad blocks
constexpr uint16_t CF_HOST = 0000360;     // what the host may set
constexpr uint16_t HOST_TIMEOUT = 120;
constexpr uint16_t CONTROLLER_VERSION = (1 << 8) | 3;  // hardware, software
constexpr uint16_t CONTROLLER_ID = (1 << 8) | 19;      // mass storage, RQDX3
// A unit
constexpr uint16_t UF_REPLACE = 0100000;
constexpr uint16_t UNIT_ID = (2 << 8) | 13;  // a disk, an RD54
constexpr uint32_t MEDIA_RD54 = 0x25644036;
constexpr uint16_t TRACK = 17;  // an RD54's sectors a track and tracks a group
constexpr uint16_t GROUP = 15;
constexpr uint16_t RCT_SIZE = 7;
constexpr uint16_t RBN = (1 << 8) | 1;  // RCT copies, RBNs a track
constexpr uint16_t SERIAL = 01234;
// The controller's credits: all to the host at first, then one an end packet.
constexpr uint8_t CREDITS = 15;
constexpr uint8_t CREDITS_MAX = 14;
constexpr uint16_t BLOCK = 512;

uint32_t word32(const uint16_t *pkt, uint8_t at) {
    return pkt[at] | (uint32_t(pkt[at + 1]) << 16);
}

void put32(uint16_t *pkt, uint8_t at, uint32_t value) {
    pkt[at] = value;
    pkt[at + 1] = value >> 16;
}
}  // namespace

void Mscp::reset() {
    _state = STEP1;
    _sa = SA_STEP1 | SA_Q22 | SA_DIAG | SA_MAP;
    _written = _poll = _step4 = false;
    _s1 = 0;
    _credits = CREDITS;
    for (auto &o : _online)
        o = false;
}

uint8_t Mscp::step() const {
    switch (_state) {
    case STEP1:
    case WRAP:
        return 1;
    case STEP2:
        return 2;
    case STEP3:
    case PURGE_ACK:
    case PURGE_POLL:
        return 3;
    case STEP4:
        return 4;
    default:
        return 0;
    }
}

// A poll is kept until the controller is up: a host reads IP right
// after its GO, before the service that takes it.
void Mscp::readIp() {
    if (_state == PURGE_POLL) {
        _step4 = true;
    } else {
        _poll = true;
    }
}

void Mscp::writeSa(uint16_t data) {
    _saw = data;
    _written = true;
}

bool Mscp::service() {
    if (_written) {
        _written = false;
        reply();
        return true;
    }
    if (_step4) {
        _step4 = false;
        step4();
        return true;
    }
    if (_poll && _state == UP)
        return poll();
    return false;
}

// The host's word for the step SA shows; step 4 and the initialization
// interrupts are the controller's part.
void Mscp::reply() {
    switch (_state) {
    case STEP1:
        if ((_saw & S1_VALID) == 0)
            return;
        if (_saw & S1_WRAP) {
            _state = WRAP;
            _sa = _saw;
            return;
        }
        _s1 = _saw;
        _sa = SA_STEP2 | (_s1 >> 8);
        _state = STEP2;
        initInterrupt();
        return;
    case WRAP:
        _sa = _saw;
        return;
    case STEP2:
        _comm = _saw & ~1;
        _purgeInt = (_saw & S2_PURGE_INT) != 0;
        _sa = SA_STEP3 | (_s1 & 0377);
        _state = STEP3;
        initInterrupt();
        return;
    case STEP3:
        _comm |= uint32_t(_saw & ~S3_PURGE_POLL) << 16;
        if (_saw & S3_PURGE_POLL) {
            _sa = 0;
            _state = PURGE_ACK;
        } else {
            step4();
        }
        return;
    case PURGE_ACK:
        if (_saw == 0) {
            _state = PURGE_POLL;
        } else {
            reset();
            _sa |= SA_ERR;
        }
        return;
    case STEP4:
        if (_saw & S4_GO) {
            _state = UP;
            _sa = 0;
        }
        return;
    default:
        return;
    }
}

// The rings' sizes come from the step 1 word: the response ring at the
// base, the command ring after it, their words and the transition words
// below cleared.
void Mscp::step4() {
    _rsp.base = _comm;
    _rsp.length = 4 << ((_s1 >> 8) & 7);
    _rsp.index = 0;
    _rsp.transition = COMM_RSP_INT;
    _cmd.base = _comm + _rsp.length;
    _cmd.length = 4 << ((_s1 >> 11) & 7);
    _cmd.index = 0;
    _cmd.transition = COMM_CMD_INT;
    const auto from = _comm + (_purgeInt ? COMM_PURGE : COMM_CMD_INT);
    const auto to = _cmd.base + _cmd.length;
    for (auto addr = from; addr < to; addr += 2)
        _host.writeWord(addr, 0);
    _sa = SA_STEP4 | SA_MODEL | SA_VERSION;
    _state = STEP4;
    initInterrupt();
}

void Mscp::initInterrupt() {
    if ((_s1 & S1_IE) && vector())
        _host.interrupt();
}

uint32_t Mscp::descriptor(const Ring &ring) const {
    const auto at = ring.base + ring.index;
    return _host.readWord(at) | (uint32_t(_host.readWord(at + 2)) << 16);
}

// Hands |desc| back to the host, its flag set, and interrupts on the
// ring's transition when the host asked: with a single descriptor, or
// when the previous one is still the controller's, as the ring was
// empty (a command ring) or full (a response ring) until now.
void Mscp::release(Ring &ring, uint32_t desc) {
    const auto at = ring.base + ring.index;
    const uint32_t back = (desc & ~DESC_OWN) | DESC_FLAG;
    _host.writeWord(at, back);
    _host.writeWord(at + 2, back >> 16);
    if (desc & DESC_FLAG) {
        if (ring.length <= 4) {
            ringInterrupt(ring);
        } else {
            const auto prev =
                    ring.base + ((ring.index - 4) & (ring.length - 1));
            if (_host.readWord(prev + 2) & (DESC_OWN >> 16))
                ringInterrupt(ring);
        }
    }
    ring.index = (ring.index + 4) & (ring.length - 1);
}

void Mscp::ringInterrupt(const Ring &ring) {
    _host.writeWord(_comm + ring.transition, 1);
    if (vector())
        _host.interrupt();
}

// Takes the command the host's ring holds, when the response ring has
// room for its end packet, and answers it.
bool Mscp::poll() {
    const auto cdesc = descriptor(_cmd);
    if ((cdesc & DESC_OWN) == 0) {
        _poll = false;
        return false;
    }
    const auto rdesc = descriptor(_rsp);
    if ((rdesc & DESC_OWN) == 0)
        return false;
    uint16_t pkt[PKT_WORDS];
    const auto caddr = cdesc & DESC_ADDR;
    for (auto i = 0; i < PKT_WORDS; ++i)
        pkt[i] = _host.readWord(caddr + 2 * i);
    release(_cmd, cdesc);
    const auto len = execute(pkt);
    const auto raddr = rdesc & DESC_ADDR;
    for (auto i = 0; i < len / 2; ++i)
        _host.writeWord(raddr + 2 * i, pkt[i]);
    const uint8_t credits = _credits < CREDITS_MAX ? _credits : CREDITS_MAX;
    _credits -= credits;
    _host.writeWord(raddr - 4, len);
    _host.writeWord(raddr - 2, credits + 1);  // a sequential MSCP message
    release(_rsp, rdesc);
    return true;
}

// Turns the command in |pkt| into its end packet and returns its length.
uint16_t Mscp::execute(uint16_t *pkt) {
    auto opcode = pkt[P_OPCODE] & 0377;
    auto unit = pkt[P_UNIT];
    uint16_t status = ST_SUCCESS;
    uint16_t len = LEN_END;
    switch (opcode) {
    case OP_SET_CONTROLLER:
        if (pkt[P_VERSION] != 0) {
            status = ST_INVALID | INV_VERSION;
            opcode = 0;
            break;
        }
        pkt[P_CFLAGS] = CF_REPLACE | (pkt[P_CFLAGS] & CF_HOST);
        pkt[P_TIMEOUT] = HOST_TIMEOUT;
        pkt[P_CVERSION] = CONTROLLER_VERSION;
        pkt[P_CID] = pkt[P_CID + 1] = pkt[P_CID + 2] = 0;
        pkt[P_CID + 3] = CONTROLLER_ID;
        put32(pkt, P_MAX_BYTES, 0);
        len = LEN_SET_CONTROLLER;
        break;
    case OP_ONLINE:
        if (unit >= UNITS) {
            status = ST_OFFLINE;
        } else if (_host.unitBlocks(unit) == 0) {
            status = ST_OFFLINE | SUB_NO_VOLUME;
            unitFields(pkt, unit, true);
        } else {
            if (_online[unit])
                status |= SUB_ALREADY_ONLINE;
            _online[unit] = true;
            unitFields(pkt, unit, true);
        }
        pkt[P_SHADOW] = unit;
        pkt[P_SHADOW + 1] = 0;
        len = LEN_ONLINE;
        break;
    case OP_GET_UNIT_STATUS:
        if ((pkt[P_STATUS] & MD_NEXT_UNIT) && unit >= UNITS)
            pkt[P_UNIT] = unit = 0;
        if (unit >= UNITS) {
            status = ST_OFFLINE;
        } else {
            const auto blocks = _host.unitBlocks(unit);
            status = blocks == 0     ? (ST_OFFLINE | SUB_NO_VOLUME)
                     : _online[unit] ? ST_SUCCESS
                                     : ST_AVAILABLE;
            unitFields(pkt, unit, false);
            pkt[P_TRACK] = TRACK;
            pkt[P_GROUP] = GROUP;
            pkt[P_CYLINDER] = (blocks + TRACK * GROUP - 1) / (TRACK * GROUP);
            pkt[P_UVERSION] = 0;
            pkt[P_RCT_SIZE] = RCT_SIZE;
            pkt[P_RBN] = RBN;
        }
        pkt[P_SHADOW] = unit;
        pkt[P_SHADOW + 1] = 0;
        len = LEN_GET_UNIT_STATUS;
        break;
    case OP_AVAILABLE:
        if (unit >= UNITS) {
            status = ST_OFFLINE;
        } else {
            _online[unit] = false;
        }
        break;
    case OP_READ:
    case OP_WRITE:
        status = readWrite(pkt, opcode == OP_WRITE);
        len = LEN_READ_WRITE;
        break;
    default:
        status = ST_INVALID | INV_OPCODE;
        break;
    }
    pkt[P_OPCODE] = opcode | OP_END;
    pkt[P_STATUS] = status;
    return len;
}

void Mscp::unitFields(uint16_t *pkt, uint16_t unit, bool size) const {
    pkt[P_MULTI] = unit;
    pkt[P_UFLAGS] = UF_REPLACE;
    pkt[P_UFLAGS + 1] = pkt[P_UFLAGS + 2] = 0;
    pkt[P_UID] = unit;
    pkt[P_UID + 1] = pkt[P_UID + 2] = 0;
    pkt[P_UID + 3] = UNIT_ID;
    put32(pkt, P_MEDIA, MEDIA_RD54);
    if (size) {
        put32(pkt, P_SIZE, _host.unitBlocks(unit));
        pkt[P_SERIAL] = SERIAL + unit;
        pkt[P_SERIAL + 1] = 0;
    }
}

// The end packet's count is what was moved; the words after it are
// the controller's working copies, cleared.
uint16_t Mscp::readWrite(uint16_t *pkt, bool toDisk) {
    const auto unit = pkt[P_UNIT];
    const auto bytes = word32(pkt, P_BYTES);
    const auto addr = word32(pkt, P_ADDR);
    const auto lbn = word32(pkt, P_LBN);
    for (auto i = P_ADDR; i < P_LBN; ++i)
        pkt[i] = 0;
    put32(pkt, P_BYTES, 0);
    if (unit >= UNITS)
        return ST_OFFLINE;
    const auto blocks = _host.unitBlocks(unit);
    if (blocks == 0)
        return ST_OFFLINE | SUB_NO_VOLUME;
    if (!_online[unit])
        return ST_AVAILABLE;
    if (addr & 1)
        return ST_HOST | SUB_ODD_ADDR;
    if (bytes & 1)
        return ST_HOST | SUB_ODD_COUNT;
    if (bytes & 0xF0000000)
        return ST_INVALID | INV_COUNT;
    if (lbn >= blocks)
        return ST_INVALID | INV_LBN;
    if (lbn + (bytes + BLOCK - 1) / BLOCK > blocks)
        return ST_INVALID | INV_COUNT;
    if ((addr & DESC_ADDR) + bytes > IO_PAGE)
        return ST_HOST | SUB_NXM;
    if (!_host.transfer(unit, lbn, addr & DESC_ADDR, bytes, toDisk))
        return ST_DRIVE;
    put32(pkt, P_BYTES, bytes);
    return ST_SUCCESS;
}

}  // namespace pdp11
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
