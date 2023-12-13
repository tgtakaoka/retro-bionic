#ifndef __DIGITAL_BUS_H__
#define __DIGITAL_BUS_H__

#define __concat2__(a, b) a##b

#define __GPIO__(port) __concat2__(GPIO, port)
#define __DDR__(port) __concat2__(port, _GDIR)
#define __OUT__(port) __concat2__(port, _DR)
#define __SET__(port) __concat2__(port, _DR_SET)
#define __CLR__(port) __concat2__(port, _DR_CLEAR)
#define __IN__(port) __concat2__(port, _PSR)
#define GPIO(name) __GPIO__(PORT_##name)
#define PDDR(name) __DDR__(GPIO(name))
#define POUT(name) __OUT__(GPIO(name))
#define PSET(name) __SET__(GPIO(name))
#define PCLR(name) __CLR__(GPIO(name))
#define PIN(name) __IN__(GPIO(name))

/**
 * BUS_gp: least significant bit position in PORT.
 * BUS_gm: bit mask in PORT starting from BUS_gp.
 * BUS_vp: least significant bit position in Value.
 */
#define BUS_gp(name) (name##_gp)
#define BUS_gm(name) (name##_gm)
#define BUS_vp(name) (name##_vp)
#define PORT_gm(name) (BUS_gm(name) << BUS_gp(name))
#define VALUE_gm(name) (BUS_gm(name) << BUS_vp(name))

#define busMode(name, mode)               \
    do {                                  \
        if ((mode) == OUTPUT) {           \
            PDDR(name) |= PORT_gm(name);  \
        } else {                          \
            PDDR(name) &= ~PORT_gm(name); \
        }                                 \
    } while (0)
#define readPort(name, val)                                                  \
    (BUS_gp(name) >= BUS_vp(name) ? ((val) >> (BUS_gp(name) - BUS_vp(name))) \
                                  : ((val) << (BUS_vp(name) - BUS_gp(name))))
#define writePort(name, val)                                                 \
    (BUS_gp(name) >= BUS_vp(name) ? ((val) << (BUS_gp(name) - BUS_vp(name))) \
                                  : ((val) >> (BUS_vp(name) - BUS_gp(name))))
#define busRead(name) (readPort(name, PIN(name)) & VALUE_gm(name))
#define busWrite(name, val)                                            \
    do {                                                               \
        const uint32_t v = static_cast<uint32_t>(val) << BUS_vp(name); \
        const uint32_t o = POUT(name) & ~PORT_gm(name);                \
        POUT(name) = o | (writePort(name, v) & PORT_gm(name));         \
    } while (0)

#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
