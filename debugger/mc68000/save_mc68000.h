#ifndef __SAVE_MC68000_H__
#define __SAVE_MC68000_H__

#include <stdint.h>

namespace debugger {
namespace mc68000 {

// The register save the debugger injects, and where its pushes land.
// MOVEM stacks D0-D7/A0-A6 from SSP down, D0 lowest; USP, on the MC68010
// VBR, SFC and DFC, and SR follow. BRA back to the origin fetches it again
// once all have run.
struct SaveMc68000 {
    // clang-format off
    static constexpr uint8_t SAVE[] = {
        0x48, 0xE7, 0xFF, 0xFE,  // MOVEM.L D0-D7/A0-A6,-(A7)
        0x4E, 0x68,              // MOVE    USP,A0
        0x2F, 0x08,              // MOVE.L  A0,-(A7)
        0x40, 0xE7,              // MOVE    SR,-(A7)
        0x60, 0xF4,              // BRA.S   org
    };
    static constexpr uint8_t SAVE10[] = {
        0x48, 0xE7, 0xFF, 0xFE,  // MOVEM.L D0-D7/A0-A6,-(A7)
        0x4E, 0x68,              // MOVE    USP,A0
        0x2F, 0x08,              // MOVE.L  A0,-(A7)
        0x4E, 0x7A, 0x88, 0x01,  // MOVEC   VBR,A0
        0x2F, 0x08,              // MOVE.L  A0,-(A7)
        0x4E, 0x7A, 0x80, 0x00,  // MOVEC   SFC,A0
        0x2F, 0x08,              // MOVE.L  A0,-(A7)
        0x4E, 0x7A, 0x80, 0x01,  // MOVEC   DFC,A0
        0x2F, 0x08,              // MOVE.L  A0,-(A7)
        0x40, 0xE7,              // MOVE    SR,-(A7)
        0x60, 0xE2,              // BRA.S   org
    };
    // clang-format on
    static_assert(SAVE[sizeof(SAVE) - 1] == uint8_t(-sizeof(SAVE)),
            "BRA.S org: the displacement is from the BRA plus 2");
    static_assert(SAVE10[sizeof(SAVE10) - 1] == uint8_t(-sizeof(SAVE10)),
            "BRA.S org: the displacement is from the BRA plus 2");

    static constexpr uint_fast8_t MOVEM_BYTES = 15 * 4;
    static constexpr uint_fast8_t MAX_BYTES = MOVEM_BYTES + 4 + 3 * 4 + 2;
    static constexpr uint_fast8_t bytes(bool mc68010) {
        return MOVEM_BYTES + 4 + (mc68010 ? 3 * 4 : 0) + 2;
    }

    struct Pushed {
        uint32_t sp;  // SSP as the save began
        uint16_t sr;
        uint32_t usp;
        uint32_t d[8];
        uint32_t a[7];
        uint32_t vbr;
        uint8_t sfc;
        uint8_t dfc;
    };

    // The registers in the |n| bytes captured at |addrs|, whatever order
    // the bus moved them in; addresses wrap at |mask|. False when a byte
    // is missing.
    static bool decode(const uint8_t *buf, const uint32_t *addrs,
            uint_fast8_t n, uint32_t mask, bool mc68010, Pushed &p) {
        const Captured c{buf, addrs, n, mask};
        if (n < bytes(mc68010))
            return false;
        // SR went last, as one word: its address is the lower of the last
        // two.
        const auto low =
                addrs[n - 2] < addrs[n - 1] ? addrs[n - 2] : addrs[n - 1];
        const auto sp = (low + bytes(mc68010)) & mask;
        const auto base = sp - MOVEM_BYTES;
        auto ok = c.readWord(low, p.sr);
        for (uint_fast8_t i = 0; i < 8; ++i)
            ok &= c.readLong(base + 4 * i, p.d[i]);
        for (uint_fast8_t i = 0; i < 7; ++i)
            ok &= c.readLong(base + 32 + 4 * i, p.a[i]);
        ok &= c.readLong(base - 4, p.usp);
        if (mc68010) {
            uint32_t sfc, dfc;
            ok &= c.readLong(base - 8, p.vbr);
            ok &= c.readLong(base - 12, sfc);
            ok &= c.readLong(base - 16, dfc);
            p.sfc = sfc & 7;
            p.dfc = dfc & 7;
        }
        p.sp = sp;
        return ok;
    }

private:
    struct Captured {
        const uint8_t *buf;
        const uint32_t *addrs;
        uint_fast8_t n;
        uint32_t mask;

        bool readByte(uint32_t addr, uint8_t &v) const {
            for (uint_fast8_t i = 0; i < n; ++i) {
                if (addrs[i] == (addr & mask)) {
                    v = buf[i];
                    return true;
                }
            }
            return false;
        }
        bool readWord(uint32_t addr, uint16_t &v) const {
            uint8_t h, l;
            if (!readByte(addr, h) || !readByte(addr + 1, l))
                return false;
            v = static_cast<uint16_t>(h) << 8 | l;
            return true;
        }
        bool readLong(uint32_t addr, uint32_t &v) const {
            uint16_t h, l;
            if (!readWord(addr, h) || !readWord(addr + 2, l))
                return false;
            v = static_cast<uint32_t>(h) << 16 | l;
            return true;
        }
    };
};

}  // namespace mc68000
}  // namespace debugger
#endif /* __SAVE_MC68000_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
