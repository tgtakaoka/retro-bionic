// Host-side tests for the kernel data mapping the DCJ11 debugger
// translates its stack and store addresses through:
// `pio test -e native -f test_pdp11_mmu`. MMR0, MMR3, KDPAR0, KDPAR6,
// KDPAR7 and KIPAR7 are 2.11BSD's, read under simh with its kernel up.

#include <unity.h>

#include "pdp11/mmu_pdp11.h"

using namespace debugger::pdp11;

namespace {

MmuPdp11 bsd211() {
    MmuPdp11 mmu;
    mmu.mmr0 = 0000377;
    mmu.mmr3 = 0000025;
    const uint16_t kdpar[] = {0, 0200, 0400, 0600, 01000, 01200, 004534, 0177600};
    for (auto i = 0; i < MmuPdp11::PAGES; ++i)
        mmu.par[i] = kdpar[i];
    return mmu;
}

void test_off() {
    MmuPdp11 mmu;
    TEST_ASSERT_FALSE(mmu.enabled());
    TEST_ASSERT_EQUAL_UINT32(0173004, mmu.physical(0173004));
    TEST_ASSERT_EQUAL_UINT32(0000024, mmu.physical(0000024));
}

void test_kernel_d() {
    const auto mmu = bsd211();
    TEST_ASSERT_TRUE(mmu.enabled());
    TEST_ASSERT_EQUAL_UINT16(MmuPdp11::KDPAR0, mmu.parBase());
    // The vectors, the I/O page and the stack.
    TEST_ASSERT_EQUAL_UINT32(0000024, mmu.physical(0000024));
    TEST_ASSERT_EQUAL_UINT32(017777000, mmu.physical(0177000));
    TEST_ASSERT_EQUAL_UINT32(0453400 + 07000, mmu.physical(0147000));
}

void test_kernel_i_and_18bit() {
    MmuPdp11 mmu;
    mmu.mmr0 = 1;
    mmu.par[7] = 0177600;  // 18 bits keep the I/O page's low ones
    TEST_ASSERT_EQUAL_UINT16(MmuPdp11::KIPAR0, mmu.parBase());
    TEST_ASSERT_EQUAL_UINT32(0777000, mmu.physical(0177000));
    mmu.par[7] = 004123;  // 2.11BSD's kernel I page 7
    TEST_ASSERT_EQUAL_UINT32(0425304, mmu.physical(0173004));
}

}  // namespace

void setUp() {}
void tearDown() {}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_off);
    RUN_TEST(test_kernel_d);
    RUN_TEST(test_kernel_i_and_18bit);
    return UNITY_END();
}
