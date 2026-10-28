#ifndef __DEBUGGER_PWM_CLOCK_H__
#define __DEBUGGER_PWM_CLOCK_H__

#include <Arduino.h>

namespace debugger {

/**
 * The FlexPWM output behind a Teensy pin (Teensy core pwm.c): its module,
 * submodule, output (A or B), pad mux value and pad mux register. Only the
 * pins a board clocks from are listed; add a row from pwm.c for a new one.
 */
template <uint8_t PIN>
struct PwmPin {
    static constexpr bool flexpwm = false;
};

template <>
struct PwmPin<5> {  // FlexPWM2_1_A, EMC_08
    static constexpr bool flexpwm = true;
    static constexpr uint32_t module = IMXRT_FLEXPWM2_ADDRESS;
    static constexpr uint8_t submodule = 1;
    static constexpr bool outputB = false;
    static constexpr uint32_t mux = 1;
    static volatile uint32_t &pad() { return CORE_PIN5_CONFIG; }
};

template <>
struct PwmPin<6> {  // FlexPWM2_2_A, B0_10
    static constexpr bool flexpwm = true;
    static constexpr uint32_t module = IMXRT_FLEXPWM2_ADDRESS;
    static constexpr uint8_t submodule = 2;
    static constexpr bool outputB = false;
    static constexpr uint32_t mux = 2;
    static volatile uint32_t &pad() { return CORE_PIN6_CONFIG; }
};

template <>
struct PwmPin<28> {  // FlexPWM3_1_B, EMC_32
    static constexpr bool flexpwm = true;
    static constexpr uint32_t module = IMXRT_FLEXPWM3_ADDRESS;
    static constexpr uint8_t submodule = 1;
    static constexpr bool outputB = true;
    static constexpr uint32_t mux = 1;
    static volatile uint32_t &pad() { return CORE_PIN28_CONFIG; }
};

template <>
struct PwmPin<29> {  // FlexPWM3_1_A, EMC_31
    static constexpr bool flexpwm = true;
    static constexpr uint32_t module = IMXRT_FLEXPWM3_ADDRESS;
    static constexpr uint8_t submodule = 1;
    static constexpr bool outputB = false;
    static constexpr uint32_t mux = 1;
    static volatile uint32_t &pad() { return CORE_PIN29_CONFIG; }
};

/**
 * A CPU clock on |PIN| from its FlexPWM output. Every register is fixed by
 * the pin at compile time, so the calls compile to the bare register
 * accesses. The pin is handed between GPIO and the PWM with the output
 * masked, so a pulse is never cut short.
 */
template <uint8_t PIN>
struct PwmClock {
    using Pin = PwmPin<PIN>;
    static_assert(Pin::flexpwm, "not a FlexPWM pin");

    /**
     * Pulses no shorter than |halfMinNs| per half, no slower than |minHz|,
     * high for |duty|/256 of a period.
     */
    PwmClock(uint32_t halfMinNs, uint32_t minHz, uint8_t duty = 128)
        : _halfMinNs(halfMinNs), _minHz(minHz), _duty(duty) {}

    static IMXRT_FLEXPWM_t &pwm() {
        return *reinterpret_cast<IMXRT_FLEXPWM_t *>(Pin::module);
    }
    static auto &sm() { return pwm().SM[Pin::submodule]; }
    /** The period's count: high from 0, low from the duty's count. */
    static uint16_t count() { return sm().CNT; }

    /**
     * Time the PWM as near |hz| as its divider allows, any odd count going to
     * the low half. Returns the rate it runs at, or 0 for one the CPU can't
     * take, changing nothing. The pad mux is left alone.
     */
    uint32_t setRate(uint32_t hz) const {
        if (hz < _minHz)
            return 0;
        // Only an undivided count comes near the minimum half.
        const uint32_t div = (F_BUS_ACTUAL + hz / 2) / hz;
        if (uint64_t(div / 2) * 1000000000 <
                uint64_t(_halfMinNs) * F_BUS_ACTUAL)
            return 0;
        analogWriteFrequency(PIN, hz);
        auto &s = sm();
        pwm().MCTRL |= FLEXPWM_MCTRL_CLDOK(1 << Pin::submodule);
        const uint16_t high = (uint32_t(s.VAL1 + 1) * _duty) >> 8;
        if (Pin::outputB) {
            s.VAL5 = high;
        } else {
            s.VAL3 = high;
        }
        pwm().MCTRL |= FLEXPWM_MCTRL_LDOK(1 << Pin::submodule);
        const auto prescale = (s.CTRL >> 4) & 7;
        return (F_BUS_ACTUAL >> prescale) / (s.VAL1 + 1);
    }

    /** Run the PWM at |hz|, masked low, and leave the pin a GPIO driving low. */
    void begin(uint32_t hz) {
        setRate(hz);
        auto &s = sm();
        // The reload takes MASK, not just a software FORCE.
        s.CTRL2 = (s.CTRL2 & ~FLEXPWM_SMCTRL2_FORCE_SEL(7)) |
                  FLEXPWM_SMCTRL2_FORCE_SEL(2);
        mask(true);
        analogWrite(PIN, _duty);  // at the default 8-bit resolution
        pinMode(PIN, OUTPUT);
        digitalWriteFast(PIN, LOW);
        _gpioMux = Pin::pad();
    }

    /**
     * The output masked low, or let go, from the next period start: a mask
     * change waits for the reload (RT1060 RM 55.8.46), so the pulse in
     * progress is never cut short.
     */
    static void mask(bool masked) {
        const uint16_t m = Pin::outputB
                                   ? FLEXPWM_MASK_MASKB(1 << Pin::submodule)
                                   : FLEXPWM_MASK_MASKA(1 << Pin::submodule);
        auto &p = pwm();
        p.MASK = masked ? (p.MASK | m) : (p.MASK & ~m);
    }

    /** Hand the pin, low, to the PWM; the first high half is whole. */
    void start() const {
        Pin::pad() = Pin::mux;
        mask(false);
    }

    /** Take the pin back low once the mask holds it there, two periods of |hz| on. */
    void stop(uint32_t hz) const {
        mask(true);
        delayNanoseconds(2 * (1000000000 / hz));
        Pin::pad() = _gpioMux;
    }

private:
    const uint32_t _halfMinNs;
    const uint32_t _minHz;
    const uint8_t _duty;
    // The pad mux as a GPIO.
    uint32_t _gpioMux;
};

}  // namespace debugger
#endif /* __DEBUGGER_PWM_CLOCK_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
