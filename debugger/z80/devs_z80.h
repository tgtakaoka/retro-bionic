#ifndef __DEVS_Z80_H__
#define __DEVS_Z80_H__

#include "devs.h"

#define USART_BASE 0x40  // I/O port

namespace debugger {

struct Mems;
struct SectorDisk;

struct DevsZ80 : Devs {
    // With |mems|, the CP/M disk is there too, 10H above the USART.
    DevsZ80(uint16_t addr = USART_BASE, const Mems *mems = nullptr);
    ~DevsZ80();

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
    // With |disk|, owned, 10H above the USART: a target with its own
    // sector disk, such as the Z8000.
    DevsZ80(uint16_t addr, SectorDisk *disk);

    Disk *disk() const override;

private:
    Device *_usart;
    SectorDisk *_disk;
};

}  // namespace debugger
#endif /* __DEVS_Z80_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
