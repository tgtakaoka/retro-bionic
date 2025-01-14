#ifndef __DEBUGGER_SIGS_H__
#define __DEBUGGER_SIGS_H__

#include <stdint.h>

namespace debugger {

template <typename SIGNALS>
struct Sigs {
    uint32_t addr;
    uint16_t data;

    virtual void clear() { _mark = 0; }
    virtual void print() const = 0;
    virtual bool fetch() const { return false; }

    SIGNALS *inject(uint16_t val) {
        data = val;
        _mark |= INJECT;
        return static_cast<SIGNALS *>(this);
    }
    SIGNALS *capture() {
        _mark |= CAPTURE;
        return static_cast<SIGNALS *>(this);
    }
    bool readMemory() const { return (_mark & INJECT) == 0; }
    bool writeMemory() const { return (_mark & CAPTURE) == 0; }

protected:
    uint8_t _mark;
    static constexpr uint8_t INJECT = 1;
    static constexpr uint8_t CAPTURE = 2;
};

}  // namespace debugger
#endif /* __SIGS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
