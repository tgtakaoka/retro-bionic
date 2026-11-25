#ifndef __DEVS_I8086_H__
#define __DEVS_I8086_H__

#include "i8080/devs_i8080.h"

#define TIMER_BASE 0x58

namespace debugger {
namespace i8086 {

struct Timer;

// The 8080's USART at I/O 40H, the PC disk (pc_disk.h) at 50H and the
// PC's tick as a timer at 58H; an odd register rides AD8-AD15.
struct DevsI8086 : DevsI8080 {
    DevsI8086(const Mems *mems);
    ~DevsI8086();

    void begin() override;
    void reset() override;
    bool loopDevices() override;
    bool isSelected(uint32_t addr) const override;
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;
    uint16_t vector() const override;

    Device *parseDevice(const char *name) const override;
    void enableDevice(Device *dev) override;
    void printDevices() const override;

protected:
    Timer *_timer;
};

}  // namespace i8086
}  // namespace debugger
#endif /* __DEVS_I8086_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
