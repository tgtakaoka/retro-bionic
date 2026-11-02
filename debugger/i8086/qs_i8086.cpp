#include "qs_i8086.h"
#include <DMAChannel.h>
#include "debugger.h"
#include "pins_i8086.h"

namespace debugger {
namespace i8086 {

namespace {

// QS0 is P7.16 and QS1 P9.06 on the fast ports, which DMA can't read;
// their slow ports are GPIO2 and GPIO4. One DMA request reads QS1, then
// QS0, 0x8000 below it, into a slot of the ring; two channels would take
// two requests, too many for a CLK period.
constexpr uint32_t QS0_BIT = 16;
constexpr uint32_t QS1_BIT = 6;
static_assert(IMXRT_GPIO4_ADDRESS - IMXRT_GPIO2_ADDRESS == 0x8000,
        "GPIO4 at GPIO2+0x8000");

// The sample, in bus clocks after CLK's rising edge: QS changes some 100 ns
// after it, and the two reads take some 50 ns.
constexpr uint16_t DELAY = 12;

// Each slot holds QS1's port, then QS0's; circular addressing wants the
// ring aligned to its size.
constexpr uint32_t RING_BYTES = QsTracker::SAMPLES * 8;
DMAMEM uint32_t ring[QsTracker::SAMPLES * 2] __attribute__((aligned(RING_BYTES)));

DMAChannel dma;

// FlexPWM2 submodule 0 is the delay timer: XBAR1 restarts its count at
// CLK's rising edge, and its VAL2 compare asks for a sample.
auto &delay_sm = IMXRT_FLEXPWM2.SM[0];

void xbar_connect(uint8_t out, uint8_t in) {
    auto sel = &XBARA1_SEL0 + out / 2;
    const auto shift = (out & 1) * 8;
    *sel = (*sel & ~(0xFF << shift)) | (in << shift);
}

uint32_t index_of() {
    const auto p = static_cast<const uint8_t *>(dma.destinationAddress());
    return ((p - reinterpret_cast<const uint8_t *>(ring)) / 8) %
           QsTracker::SAMPLES;
}

// Each request is a whole major loop, so the source comes back through
// SLAST; the destination goes on round the ring.
void set_tcd() {
    auto tcd = dma.TCD;
    tcd->ATTR_SRC = 2;  // 32-bit
    tcd->ATTR_DST = ((31 - __builtin_clz(RING_BYTES)) << 3) | 2;
    tcd->SADDR = &GPIO4_PSR;
    tcd->SOFF = -0x8000;
    tcd->NBYTES = 8;
    tcd->SLAST = 0x10000;
    tcd->DADDR = ring;
    tcd->DOFF = 4;
    tcd->CITER = 1;
    tcd->BITER = 1;
    tcd->DLASTSGA = 0;
    tcd->CSR = 0;
}

}  // namespace

// FlexPWM3 submodule 1, CLK's, triggers when it counts VAL2, its rising
// edge; set again on every reset, which sets the PWM up anew.
void QsTracker::begin() {
    IMXRT_FLEXPWM3.SM[1].TCTRL |= FLEXPWM_SMTCTRL_OUT_TRIG_EN(1 << 2);
    if (_started)
        return;
    _started = true;
    CCM_CCGR0 |= CCM_CCGR0_GPIO2(CCM_CCGR_ON);
    CCM_CCGR3 |= CCM_CCGR3_GPIO4(CCM_CCGR_ON);
    CCM_CCGR2 |= CCM_CCGR2_XBAR1(CCM_CCGR_ON);
    CCM_CCGR4 |= CCM_CCGR4_PWM2(CCM_CCGR_ON);
    // The pads go to their slow ports; nothing else reads them.
    IOMUXC_GPR_GPR27 &= ~(1U << QS0_BIT);
    IOMUXC_GPR_GPR29 &= ~(1U << QS1_BIT);
    // The delay timer restarts from 0 at each rising edge and never wraps
    // within a CLK period, so it never reloads: values load at once.
    IMXRT_FLEXPWM2.MCTRL &= ~FLEXPWM_MCTRL_RUN(1);
    delay_sm.CTRL2 = FLEXPWM_SMCTRL2_INIT_SEL(3);  // EXT_SYNC
    delay_sm.CTRL = FLEXPWM_SMCTRL_FULL | FLEXPWM_SMCTRL_LDMOD;
    delay_sm.INIT = 0;
    delay_sm.VAL1 = 0x7FFF;
    delay_sm.VAL2 = DELAY;
    delay_sm.TCTRL = FLEXPWM_SMTCTRL_OUT_TRIG_EN(1 << 2);
    IMXRT_FLEXPWM2.MCTRL |= FLEXPWM_MCTRL_LDOK(1);
    IMXRT_FLEXPWM2.MCTRL |= FLEXPWM_MCTRL_RUN(1);
    // XBAR1 restarts the delay timer at each rising trigger, and output 0
    // turns each delayed one into a DMA request.
    xbar_connect(XBARA1_OUT_FLEXPWM2_PWM0_EXT_SYNC,
            XBARA1_IN_FLEXPWM3_PWM2_OUT_TRIG0);
    xbar_connect(XBARA1_OUT_DMA_CH_MUX_REQ30, XBARA1_IN_FLEXPWM2_PWM1_OUT_TRIG0);
    XBARA1_CTRL0 = (XBARA1_CTRL0 & 0xFF00) | XBARA_CTRL_EDGE0(1) |
                   XBARA_CTRL_DEN0;
    dma.begin(true);
    set_tcd();
    dma.triggerAtHardwareEvent(DMAMUX_SOURCE_XBAR1_0);
    dma.enable();
}

void QsTracker::freeze(uint32_t last) {
    if (_frozen)
        return;
    dma.disable();
    _stopCycle = last;
    _stopIndex = index_of();
    _frozen = true;
    arm_dcache_delete(ring, sizeof(ring));
}

void QsTracker::thaw() {
    if (!_frozen)
        return;
    _frozen = false;
    dma.enable();
}

uint8_t QsTracker::sampleAt(uint32_t back) const {
    const auto i = (_stopIndex - 1 - back) % SAMPLES;
    return ((ring[i * 2 + 1] >> QS0_BIT) & 1) |
           (((ring[i * 2] >> QS1_BIT) & 1) << 1);
}

uint16_t QsTracker::delay() {
    return DELAY;
}

}  // namespace i8086
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
