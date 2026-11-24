// Host-side tests for the MSCP controller core, against a memory and
// drives in arrays: `pio test -e native -f test_mscp`. The sequences are
// the ones simh's RQDX3 saw from 2.11BSD's boot ROM, rauboot and kernel
// (debugger/pdp11/mscp.cpp).

#include <unity.h>

#include <string.h>
#include <vector>

#include "pdp11/mscp.h"

using namespace debugger::pdp11;

namespace {

constexpr uint32_t MEM_WORDS = 1 << 21;  // the 22-bit space
constexpr uint32_t BLOCK = 512;

struct FakeHost final : MscpHost {
    std::vector<uint16_t> mem;
    std::vector<uint8_t> disk[Mscp::UNITS];
    int interrupts = 0;
    bool fail = false;

    FakeHost() : mem(MEM_WORDS, 0) {}

    uint16_t readWord(uint32_t addr) const override { return mem[addr / 2]; }
    void writeWord(uint32_t addr, uint16_t data) const override {
        const_cast<FakeHost *>(this)->mem[addr / 2] = data;
    }
    uint32_t unitBlocks(uint8_t unit) const override {
        return disk[unit].size() / BLOCK;
    }
    bool transfer(uint8_t unit, uint32_t lbn, uint32_t addr, uint32_t bytes,
            bool toDisk) override {
        if (fail)
            return false;
        auto &d = disk[unit];
        for (uint32_t i = 0; i < bytes; i += 2) {
            const auto at = lbn * BLOCK + i;
            if (toDisk) {
                const auto w = mem[(addr + i) / 2];
                d[at] = w;
                d[at + 1] = w >> 8;
            } else {
                mem[(addr + i) / 2] = d[at] | (d[at + 1] << 8);
            }
        }
        return true;
    }
    void interrupt() override { ++interrupts; }

    uint32_t read32(uint32_t addr) const {
        return readWord(addr) | (uint32_t(readWord(addr + 2)) << 16);
    }
    void write32(uint32_t addr, uint32_t value) {
        writeWord(addr, value);
        writeWord(addr + 2, value >> 16);
    }
};

constexpr uint32_t OWN = 0x80000000;
constexpr uint32_t FLAG = 0x40000000;

// A host with rings of |cmds| command and |rsps| response descriptors at
// |comm|, their packets at |pkts| on, 128 bytes apart after a header.
struct Rings {
    FakeHost &host;
    Mscp &mscp;
    uint32_t comm;
    uint8_t cmds, rsps;
    uint32_t pkts;
    uint8_t cmdIdx = 0, rspIdx = 0;

    uint32_t rspDesc(uint8_t i) const { return comm + 4 * i; }
    uint32_t cmdDesc(uint8_t i) const { return comm + 4 * rsps + 4 * i; }
    uint32_t rspPkt(uint8_t i) const { return pkts + 4 + 128 * i; }
    uint32_t cmdPkt(uint8_t i) const { return pkts + 4 + 128 * (rsps + i); }

    void run() {
        while (mscp.service())
            ;
    }
    // Initializes as the kernel does: |s1| with the ring sizes and
    // vector, the base in two parts, GO.
    void init(uint16_t s1) {
        mscp.reset();
        TEST_ASSERT_EQUAL_HEX16(0x0B40, mscp.sa());
        mscp.writeSa(s1);
        run();
        TEST_ASSERT_EQUAL_HEX16(0x1000 | (s1 >> 8), mscp.sa());
        mscp.writeSa(comm & 0xFFFF);
        run();
        TEST_ASSERT_EQUAL_HEX16(0x2000 | (s1 & 0xFF), mscp.sa());
        mscp.writeSa(comm >> 16);
        run();
        TEST_ASSERT_EQUAL_HEX16(0x4133, mscp.sa());
        TEST_ASSERT_EQUAL(4, mscp.step());
        mscp.writeSa(1);
        run();
        TEST_ASSERT_EQUAL_HEX16(0, mscp.sa());
        TEST_ASSERT_EQUAL(0, mscp.step());
        armResponses();
    }
    // Gives the controller every response slot, as the kernel does.
    void armResponses() {
        for (auto i = 0; i < rsps; ++i)
            arm(i);
        rspIdx = 0;
    }
    void arm(uint8_t i) { host.write32(rspDesc(i), OWN | FLAG | rspPkt(i)); }
    // Puts a command on the ring: |words| from word 6 on.
    uint8_t command(uint16_t unit, uint16_t opcode, uint16_t modifier,
            std::vector<uint16_t> words = {}) {
        const auto i = cmdIdx++ % cmds;
        const auto p = cmdPkt(i);
        for (auto w = 0; w < 32; ++w)
            host.writeWord(p + 2 * w, 0);
        host.writeWord(p - 4, 48);
        host.writeWord(p - 2, 0);
        host.write32(p, 0x1234 + i);  // the reference
        host.writeWord(p + 4, unit);
        host.writeWord(p + 8, opcode);
        host.writeWord(p + 10, modifier);
        for (size_t w = 0; w < words.size(); ++w)
            host.writeWord(p + 12 + 2 * w, words[w]);
        host.write32(cmdDesc(i), OWN | FLAG | p);
        return i;
    }
    // Polls and services; the response to command |i| is in slot |i|,
    // which goes back to the controller as the kernel's would, unless
    // |keep|.
    void poll(bool keep = false) {
        mscp.readIp();
        run();
        if (!keep)
            arm(rspIdx % rsps);
        ++rspIdx;
    }
    uint16_t rsp(uint8_t i, uint8_t word) const {
        return host.readWord(rspPkt(i % rsps) + 2 * word);
    }
    uint16_t rspLength(uint8_t i) const {
        return host.readWord(rspPkt(i % rsps) - 4);
    }
    uint16_t rspCredits(uint8_t i) const {
        return host.readWord(rspPkt(i % rsps) - 2) & 0xF;
    }
};

FakeHost *host;
Mscp *mscp;

// simh's boot ROM: rings of one, no vector, ONLINE and a read of block 0
// to address 0, polling the descriptors.
void test_rom_boot() {
    Rings r{*host, *mscp, 007204, 1, 1, 006000};
    mscp->reset();
    TEST_ASSERT_EQUAL_HEX16(0x0B40, mscp->sa());
    mscp->writeSa(0100000);
    r.run();
    TEST_ASSERT_EQUAL_HEX16(0x1080, mscp->sa());
    mscp->writeSa(007204);
    r.run();
    TEST_ASSERT_EQUAL_HEX16(0x2000, mscp->sa());
    mscp->writeSa(0);
    r.run();
    TEST_ASSERT_EQUAL_HEX16(0x4133, mscp->sa());
    mscp->writeSa(1);
    r.run();
    TEST_ASSERT_EQUAL_HEX16(0, mscp->sa());
    TEST_ASSERT_EQUAL(0, host->interrupts);

    r.armResponses();
    r.command(0, 011, 0);
    r.poll(true);
    TEST_ASSERT_EQUAL_HEX32(FLAG | r.cmdPkt(0), host->read32(r.cmdDesc(0)));
    TEST_ASSERT_EQUAL_HEX32(FLAG | r.rspPkt(0), host->read32(r.rspDesc(0)));
    TEST_ASSERT_EQUAL_HEX16(0211, r.rsp(0, 4));
    TEST_ASSERT_EQUAL_HEX16(0, r.rsp(0, 5));
    TEST_ASSERT_EQUAL(1000, r.rsp(0, 18) | (r.rsp(0, 19) << 16));
    TEST_ASSERT_EQUAL(44, r.rspLength(0));
    TEST_ASSERT_EQUAL(15, r.rspCredits(0));
    // The transition words are flagged, the interrupt not requested.
    TEST_ASSERT_EQUAL(1, host->readWord(r.comm - 2));
    TEST_ASSERT_EQUAL(0, host->interrupts);

    r.arm(0);
    r.command(0, 041, 0, {512, 0, 0, 0});
    r.poll(true);
    TEST_ASSERT_EQUAL_HEX16(0241, r.rsp(0, 4));
    TEST_ASSERT_EQUAL_HEX16(0, r.rsp(0, 5));
    TEST_ASSERT_EQUAL(512, r.rsp(0, 6));
    for (auto i = 0; i < 512; i += 2) {
        TEST_ASSERT_EQUAL_HEX16(host->disk[0][i] | (host->disk[0][i + 1] << 8),
                host->readWord(i));
    }
    // The credits left after the first grant, then one an end packet.
    TEST_ASSERT_EQUAL(2, r.rspCredits(0));
}

// The kernel's initialization: 8 and 8, vector 154, interrupts enabled.
void test_kernel_init() {
    Rings r{*host, *mscp, 0x3F1000, 8, 8, 0x3F2000};
    host->writeWord(r.comm - 4, 0x5555);
    host->writeWord(r.comm + 32, 0x5555);  // the command ring's first word
    host->writeWord(r.comm + 62, 0x5555);  // and its last
    r.init(0115633);
    TEST_ASSERT_EQUAL(3, host->interrupts);
    TEST_ASSERT_EQUAL(0154, mscp->vector());
    TEST_ASSERT_EQUAL(0, host->readWord(r.comm - 4));
    TEST_ASSERT_EQUAL(0, host->readWord(r.comm + 32));
    TEST_ASSERT_EQUAL(0, host->readWord(r.comm + 62));
}

// Set controller characteristics hands all the credits over, then one
// an end packet.
void test_set_controller() {
    Rings r{*host, *mscp, 0x1000, 8, 8, 0x2000};
    r.init(0115633);
    host->interrupts = 0;
    r.command(0, 004, 0, {0, 0360, 0});
    r.poll();
    TEST_ASSERT_EQUAL_HEX16(0204, r.rsp(0, 4));
    TEST_ASSERT_EQUAL_HEX16(0, r.rsp(0, 5));
    TEST_ASSERT_EQUAL_HEX16(0100360, r.rsp(0, 7));
    TEST_ASSERT_EQUAL(120, r.rsp(0, 8));
    TEST_ASSERT_EQUAL_HEX16(0x0103, r.rsp(0, 9));
    TEST_ASSERT_EQUAL_HEX16(0x0113, r.rsp(0, 13));
    TEST_ASSERT_EQUAL(32, r.rspLength(0));
    TEST_ASSERT_EQUAL(15, r.rspCredits(0));
    TEST_ASSERT_EQUAL(1, host->readWord(r.comm - 2));
    TEST_ASSERT_EQUAL(1, host->interrupts);

    r.command(0, 004, 0, {1});
    r.poll();
    TEST_ASSERT_EQUAL_HEX16(0200, r.rsp(1, 4));
    TEST_ASSERT_EQUAL_HEX16(0x0C01, r.rsp(1, 5));
    TEST_ASSERT_EQUAL(2, r.rspCredits(1));
}

// A write and a read of 1024 bytes at a 22-bit address.
void test_read_write_22bit() {
    Rings r{*host, *mscp, 0x1000, 8, 8, 0x2000};
    r.init(0115633);
    r.command(0, 011, 0);
    r.poll();
    TEST_ASSERT_EQUAL_HEX16(0, r.rsp(0, 5));
    host->interrupts = 0;

    const uint32_t buf = 0x3E0000;
    for (auto i = 0; i < 1024; i += 2)
        host->writeWord(buf + i, 0xA000 + i);
    r.command(0, 042, 0, {1024, 0, buf & 0xFFFF, buf >> 16, 0, 0, 0, 0, 5, 0});
    r.poll();
    TEST_ASSERT_EQUAL_HEX16(0242, r.rsp(1, 4));
    TEST_ASSERT_EQUAL_HEX16(0, r.rsp(1, 5));
    TEST_ASSERT_EQUAL(1024, r.rsp(1, 6));
    TEST_ASSERT_EQUAL(0, r.rsp(1, 8));
    TEST_ASSERT_EQUAL_HEX8(0x00, host->disk[0][5 * BLOCK]);
    TEST_ASSERT_EQUAL_HEX8(0xA0, host->disk[0][5 * BLOCK + 1]);
    TEST_ASSERT_EQUAL_HEX8(0xFE, host->disk[0][7 * BLOCK - 2]);
    TEST_ASSERT_EQUAL(1, host->interrupts);

    const uint32_t to = 0x200000;
    r.command(0, 041, 0, {1024, 0, to & 0xFFFF, to >> 16, 0, 0, 0, 0, 5, 0});
    r.poll();
    TEST_ASSERT_EQUAL_HEX16(0241, r.rsp(2, 4));
    TEST_ASSERT_EQUAL_HEX16(0, r.rsp(2, 5));
    for (auto i = 0; i < 1024; i += 2)
        TEST_ASSERT_EQUAL_HEX16(0xA000 + i, host->readWord(to + i));
    TEST_ASSERT_EQUAL(2, host->interrupts);
}

void test_errors() {
    Rings r{*host, *mscp, 0x1000, 8, 8, 0x2000};
    r.init(0115633);
    // Unit 1 has no medium; unit 0 is not online yet.
    r.command(1, 011, 0);
    r.poll();
    TEST_ASSERT_EQUAL_HEX16(0x23, r.rsp(0, 5));
    r.command(0, 041, 0, {512, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    r.poll();
    TEST_ASSERT_EQUAL_HEX16(4, r.rsp(1, 5));
    TEST_ASSERT_EQUAL(0, r.rsp(1, 6));
    r.command(0, 011, 0);
    r.poll();
    TEST_ASSERT_EQUAL_HEX16(0, r.rsp(2, 5));
    // Past the end, odd, into the I/O page, a card failure, no such command.
    r.command(0, 041, 0, {512, 0, 0, 0, 0, 0, 0, 0, 1000, 0});
    r.poll();
    TEST_ASSERT_EQUAL_HEX16(0x1C01, r.rsp(3, 5));
    r.command(0, 041, 0, {1024, 0, 0, 0, 0, 0, 0, 0, 999, 0});
    r.poll();
    TEST_ASSERT_EQUAL_HEX16(0x0C01, r.rsp(4, 5));
    r.command(0, 041, 0, {512, 0, 1, 0, 0, 0, 0, 0, 0, 0});
    r.poll();
    TEST_ASSERT_EQUAL_HEX16(0x29, r.rsp(5, 5));
    r.command(0, 041, 0, {1024, 0, 0157600, 077, 0, 0, 0, 0, 0, 0});
    r.poll();
    TEST_ASSERT_EQUAL_HEX16(0x69, r.rsp(6, 5));
    host->fail = true;
    r.command(0, 041, 0, {512, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    r.poll();
    TEST_ASSERT_EQUAL_HEX16(11, r.rsp(7, 5));
    host->fail = false;
    r.command(0, 022, 0);
    r.poll();
    TEST_ASSERT_EQUAL_HEX16(0x0801, r.rsp(8, 5));
    TEST_ASSERT_EQUAL_HEX16(0222, r.rsp(8, 4));
    TEST_ASSERT_EQUAL(12, r.rspLength(8));
}

// Get unit status reports the geometry; available takes a unit offline.
void test_unit_status() {
    Rings r{*host, *mscp, 0x1000, 8, 8, 0x2000};
    r.init(0115633);
    r.command(0, 003, 0);
    r.poll();
    TEST_ASSERT_EQUAL_HEX16(4, r.rsp(0, 5));
    TEST_ASSERT_EQUAL(17, r.rsp(0, 18));
    TEST_ASSERT_EQUAL(15, r.rsp(0, 19));
    TEST_ASSERT_EQUAL(4, r.rsp(0, 20));
    TEST_ASSERT_EQUAL_HEX16(0x4036, r.rsp(0, 14));
    TEST_ASSERT_EQUAL_HEX16(0x2564, r.rsp(0, 15));
    TEST_ASSERT_EQUAL(48, r.rspLength(0));
    r.command(0, 011, 0);
    r.poll();
    r.command(0, 003, 0);
    r.poll();
    TEST_ASSERT_EQUAL_HEX16(0, r.rsp(2, 5));
    r.command(0, 010, 0);
    r.poll();
    TEST_ASSERT_EQUAL_HEX16(0, r.rsp(3, 5));
    TEST_ASSERT_EQUAL_HEX16(0210, r.rsp(3, 4));
    r.command(0, 003, 0);
    r.poll();
    TEST_ASSERT_EQUAL_HEX16(4, r.rsp(4, 5));
    // The next unit past the last wraps to 0.
    r.command(7, 003, 1);
    r.poll();
    TEST_ASSERT_EQUAL(0, r.rsp(5, 2));
    TEST_ASSERT_EQUAL_HEX16(4, r.rsp(5, 5));
}

// Rings of 8 wrap; a command waits while the response ring is full.
void test_ring_wrap() {
    Rings r{*host, *mscp, 0x1000, 8, 8, 0x2000};
    r.init(0115633);
    for (auto i = 0; i < 20; ++i) {
        r.command(0, 003, 0);
        r.poll();
        TEST_ASSERT_EQUAL_HEX16(0203, r.rsp(i, 4));
        TEST_ASSERT_EQUAL_HEX16(0x1234 + i % 8, r.rsp(i, 0));
        TEST_ASSERT_EQUAL_HEX32(
                FLAG | r.cmdPkt(i % 8), host->read32(r.cmdDesc(i % 8)));
    }
    // The host keeps the slots: the ninth command waits for one.
    for (auto i = 20; i < 28; ++i) {
        r.command(0, 003, 0);
        r.poll(true);
        TEST_ASSERT_EQUAL_HEX16(0203, r.rsp(i, 4));
    }
    r.command(0, 003, 0);
    r.poll(true);
    TEST_ASSERT_EQUAL_HEX32(
            OWN | FLAG | r.cmdPkt(28 % 8), host->read32(r.cmdDesc(28 % 8)));
    r.arm(28 % 8);
    r.run();
    TEST_ASSERT_EQUAL_HEX32(
            FLAG | r.cmdPkt(28 % 8), host->read32(r.cmdDesc(28 % 8)));
    TEST_ASSERT_EQUAL_HEX16(0203, r.rsp(28, 4));
}

// rauboot reads IP right after its GO: the poll waits for the service
// that brings the controller up.
void test_poll_after_go() {
    Rings r{*host, *mscp, 0x1000, 1, 1, 0x2000};
    mscp->reset();
    mscp->writeSa(0100000);
    r.run();
    mscp->writeSa(0x1000);
    r.run();
    mscp->writeSa(0);
    r.run();
    r.armResponses();
    r.command(0, 011, 0);
    mscp->writeSa(1);
    mscp->readIp();
    r.run();
    TEST_ASSERT_EQUAL_HEX16(0, mscp->sa());
    TEST_ASSERT_EQUAL_HEX16(0211, r.rsp(0, 4));
    TEST_ASSERT_EQUAL_HEX16(0, r.rsp(0, 5));
}

// Step 1 with the wrap bit echoes SA; a bus reset starts over.
void test_wrap_and_reset() {
    mscp->reset();
    mscp->writeSa(0140123);
    while (mscp->service())
        ;
    TEST_ASSERT_EQUAL_HEX16(0140123, mscp->sa());
    mscp->writeSa(012345);
    while (mscp->service())
        ;
    TEST_ASSERT_EQUAL_HEX16(012345, mscp->sa());
    TEST_ASSERT_EQUAL(1, mscp->step());
    mscp->reset();
    TEST_ASSERT_EQUAL_HEX16(0x0B40, mscp->sa());
}

}  // namespace

// Unit 0 holds 1000 blocks, each byte its block number and offset.
void setUp() {
    host = new FakeHost();
    mscp = new Mscp(*host);
    host->disk[0].resize(1000 * BLOCK);
    for (size_t i = 0; i < host->disk[0].size(); ++i)
        host->disk[0][i] = (i / BLOCK) ^ (i & 0xFF);
}

void tearDown() {
    delete mscp;
    delete host;
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_rom_boot);
    RUN_TEST(test_kernel_init);
    RUN_TEST(test_set_controller);
    RUN_TEST(test_read_write_22bit);
    RUN_TEST(test_errors);
    RUN_TEST(test_unit_status);
    RUN_TEST(test_ring_wrap);
    RUN_TEST(test_poll_after_go);
    RUN_TEST(test_wrap_and_reset);
    return UNITY_END();
}
