#ifndef __DEBUGGER_IDENTITY_H__
#define __DEBUGGER_IDENTITY_H__

#include <stdint.h>

namespace debugger {

// A target directory's code, which runs in ITCM; scripts/teensy41.ld
// keeps each in flash and gives them all one ITCM address.
struct Overlay final {
    const uint8_t *start;
    const uint8_t *stop;

    void load() const;
};

// The overlay of the target directory |dir|; empty unless the image is
// built with ITCM_OVERLAY.
#if defined(ITCM_OVERLAY)
#define TARGET_OVERLAY(dir)                                \
    extern "C" const uint8_t __load_start_ovl_##dir[];     \
    extern "C" const uint8_t __load_stop_ovl_##dir[];      \
    constexpr Overlay overlay {                            \
        __load_start_ovl_##dir, __load_stop_ovl_##dir      \
    }
#else
#define TARGET_OVERLAY(dir) \
    constexpr Overlay overlay { nullptr, nullptr }
#endif

struct Pins;
struct Target;

// Each target's static Identity adds itself to a list, which
// readIdentity() searches for the name in the identity EEPROM.
struct Identity final {
    Identity(const char *name, Pins *(*instance)(),
            Overlay overlay = Overlay{nullptr, nullptr});

    const char *name() const { return _name; }
    Target *instance() const;

    static const Identity &readIdentity();
    static void writeIdentity(const char *name);
    static void printIdentity();

    static const Identity *_head;
    static const Identity &searchIdentity(const char *name);

private:
    const char *const _name;
    Pins *(*const _instance)();
    const Overlay _overlay;
    const Identity *const _next;
};

extern const struct Identity IdentityNull;

}  // namespace debugger

#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
