#ifndef __SIGNALS_Z280_H__
#define __SIGNALS_Z280_H__

#include "signals.h"

namespace debugger {
namespace z280 {

// Status Code
enum BUS_ST : uint8_t {
    ST_RESV0 = 0x0,    // Reserved
    ST_RFSH = 0x1,     // Refresh
    ST_IORQ = 0x2,     // I/O transaction
    ST_HALT = 0x3,     // Halt
    ST_INTAA = 0x4,    // Interrupt acknowledge line A
    ST_NMIA = 0x5,     // NMI acknowledge
    ST_INTAB = 0x6,    // Interrupt acknowledge line B
    ST_INTAC = 0x7,    // Interrupt acknowledge line C
    ST_MREQ = 0x8,     // Transfer between CPU and memory, cachable
    ST_MREQ_NC = 0x9,  // Transfer between CPU and memory, non-cachable
    ST_EPU_MEM = 0xA,  // Data transfer between EPU and memory
    ST_RESVB = 0xB,    // Reserved
    ST_EPU_OPR = 0xC,  // EPU instruction fetch, template, subsequent words
    ST_EPU_OPC = 0xD,  // EPU instruction fetch, template, first word
    ST_EPU_CPU = 0xE,  // Data transfer between EPU and CPU
    ST_LOCK = 0xF,     // Test and Set (data transfer)
};

struct Signals final : SignalsBase<Signals> {
    void getAddr();
    void getControl();
    void getData();
    void outData() const;
    void inputMode() const;
    void print() const;

    bool memReq() const { return st() == ST_MREQ; }
    bool ioReq() const { return st() == ST_IORQ; }
    bool read() const { return rw() != 0; }
    bool write() const { return rw() == 0; }
    bool wordAccess() const { return bw() == 0; }
    bool byteAccess() const { return bw() != 0; }
    
private:
    uint8_t rw() const { return _signals[0]; }
    uint8_t &rw() { return _signals[0]; }
    uint8_t bw() const { return _signals[1]; }
    uint8_t &bw() { return _signals[1]; }
    uint8_t st() const { return _signals[2]; }
    uint8_t &st() { return _signals[2]; }
};
}  // namespace z280
}  // namespace debugger
#endif /* __SIGNALS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
