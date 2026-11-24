#ifndef __MSCP_H__
#define __MSCP_H__

#include <stdint.h>

namespace debugger {
namespace pdp11 {

/**
 * What an MSCP controller works on: the memory its rings, packets and
 * buffers are in, the drives, and its interrupt line.
 */
struct MscpHost {
    virtual ~MscpHost() {}
    virtual uint16_t readWord(uint32_t addr) const = 0;
    virtual void writeWord(uint32_t addr, uint16_t data) const = 0;
    // |unit|'s blocks of 512 bytes, 0 for a unit without a medium.
    virtual uint32_t unitBlocks(uint8_t unit) const = 0;
    // Moves |bytes| between |unit|'s blocks from |lbn| and memory at
    // |addr|; a write's last block is padded with zeros. False when the
    // medium fails.
    virtual bool transfer(uint8_t unit, uint32_t lbn, uint32_t addr,
            uint32_t bytes, bool toDisk) = 0;
    virtual void interrupt() = 0;
};

/**
 * An MSCP disk controller of the RQDX3's kind (MSCP Basic Disk Functions
 * Manual, UQSSP): the registers IP and SA, and the command and response
 * rings in the host's memory. A write to IP starts the four-step
 * initialization, SA showing each step and the host answering with its
 * word: the ring sizes, interrupt vector and the rings' base. Once up,
 * the host puts command packets on its ring and reads IP; the controller
 * takes each, writes its end packet on the response ring, flags the
 * ring's transition word and interrupts. service() does the work a
 * register access left.
 */
struct Mscp {
    static constexpr uint8_t UNITS = 4;

    Mscp(MscpHost &host) : _host(host) {}

    // A bus reset or a write to IP.
    void reset();
    uint16_t sa() const { return _sa; }
    // A read of IP: the host asks for its command ring to be polled.
    void readIp();
    void writeSa(uint16_t data);
    // Runs what a register access left to do; true when it did something.
    bool service();
    // The initialization step SA shows, 1 to 4; 0 once up.
    uint8_t step() const;
    // The interrupt vector the host gave, 0 for none.
    uint16_t vector() const { return (_s1 & 0177) << 2; }

private:
    enum State : uint8_t {
        STEP1,
        WRAP,  // SA echoes what is written: a diagnostic
        STEP2,
        STEP3,
        PURGE_ACK,   // the host writes 0 to SA
        PURGE_POLL,  // the host reads IP
        STEP4,
        UP,
    };
    struct Ring {
        uint32_t base = 0;
        uint16_t length = 0;    // bytes, a power of two
        uint16_t index = 0;     // the next descriptor's offset
        int8_t transition = 0;  // the word flagged from |_comm|
    };

    MscpHost &_host;
    State _state = STEP1;
    uint16_t _sa = 0;
    uint16_t _saw = 0;  // what the host wrote to SA
    bool _written = false;
    bool _poll = false;
    bool _step4 = false;
    uint16_t _s1 = 0;  // the host's step 1 word
    // The rings' base: the response ring, then the command ring.
    uint32_t _comm = 0;
    bool _purgeInt = false;
    Ring _rsp;
    Ring _cmd;
    uint8_t _credits = 0;
    bool _online[UNITS] = {};

    void reply();
    void step4();
    void initInterrupt();
    bool poll();
    uint32_t descriptor(const Ring &ring) const;
    void release(Ring &ring, uint32_t desc);
    void ringInterrupt(const Ring &ring);
    uint16_t execute(uint16_t *pkt);
    void unitFields(uint16_t *pkt, uint16_t unit, bool size) const;
    uint16_t readWrite(uint16_t *pkt, bool toDisk);
};

}  // namespace pdp11
}  // namespace debugger
#endif /* __MSCP_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
