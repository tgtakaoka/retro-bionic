#ifndef __DEVS_Z8000_H__
#define __DEVS_Z8000_H__

#include "devs.h"

namespace debugger {

struct DevsZ80;

namespace z8000 {

struct MemsZ8000;
struct SplitIo;

// The Z80's USART and CP/M-8000 disk, and the Z8002's split
// instruction/data control.
struct DevsZ8000 final : Devs {
    static constexpr uint16_t USART = 0x0140;
    static constexpr uint16_t SPLIT_BASE = 0x0160;

    explicit DevsZ8000(MemsZ8000 *mems);
    ~DevsZ8000();

    void begin() override;
    void reset() override;
    bool isSelected(uint32_t addr) const override;
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;
    uint16_t vector() const override;

    Device *parseDevice(const char *name) const override;
    void enableDevice(Device *dev) override;
    void disableDevice(Device *dev) override;
    void printDevices() const override;
    Disk *disk() const override;

protected:
    bool loopDevices() override;

private:
    DevsZ80 *const _devs;
    SplitIo *const _split;
};

}  // namespace z8000
}  // namespace debugger
#endif /* __DEVS_Z8000_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
