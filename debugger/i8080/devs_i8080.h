#ifndef __DEVS_I8080_H__
#define __DEVS_I8080_H__

#include "devs.h"

#define USART_BASE 0x40
#define DISK_BASE 0x50

namespace debugger {

struct Mems;
struct SectorDisk;

struct DevsI8080 : Devs {
    // With |mems|, the CP/M-80 disk is there too.
    DevsI8080(const Mems *mems = nullptr);
    ~DevsI8080();

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
    Disk *disk() const override;

protected:
    // With |disk|, owned, at DISK_BASE.
    DevsI8080(SectorDisk *disk);

    Device *_usart;
    SectorDisk *_disk;
};

}  // namespace debugger
#endif /* __DEVS_I8080_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
