#ifndef __SIGNALS_Z280_H__
#define __SIGNALS_Z280_H__

#include "signals.h"

// -D Z280_LOG_MATCH traces the matcher on the console.
#ifdef Z280_LOG_MATCH
#define LOG_MATCH(e) e
#else
#define LOG_MATCH(e)
#endif

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

    // Transactions that strobe #DS and move data (Table 13-1).
    // Test and Set is a locked read-modify-write on a memory
    // location, so it must be answered like any other memory
    // transaction. EPU transfers are left out: no EPU on this board.
    bool memReq() const {
        const auto status = st();
        return status == ST_MREQ || status == ST_MREQ_NC || status == ST_LOCK;
    }
    bool ioReq() const { return st() == ST_IORQ; }
    // A maskable acknowledge, which the device answers with a vector.
    // ST_NMIA sits inside the ST_INTAA..ST_INTAC range but must not be
    // included: #NMI vectors to 0066H on its own and asks for no vector,
    // so answering its acknowledge with a device vector corrupts the
    // sequence that suspend() is driving.
    bool intAck() const {
        const auto status = st();
        return status == ST_INTAA || status == ST_INTAB ||
               status == ST_INTAC;
    }
    bool nmiAck() const { return st() == ST_NMIA; }
    bool halt() const { return st() == ST_HALT; }
    bool refresh() const { return st() == ST_RFSH; }
    bool read() const { return rw() != 0; }
    bool write() const { return rw() == 0; }
    bool wordAccess() const { return bw() == 0; }
    bool byteAccess() const { return bw() != 0; }

    // What InstZ280::match() made of this cycle.
    void markFetch() { mark() = FETCH; }
    void markByte() { mark() = BYTE; }
    void markOperand() { mark() = OPERAND; }
    void clearMark() { mark() = 0; }
    bool fetch() const { return mark() == FETCH; }
    bool isByte() const { return mark() == BYTE; }
    bool isOperand() const { return mark() == OPERAND; }

private:
    enum : uint8_t { FETCH = 1, BYTE = 2, OPERAND = 3 };
    uint8_t mark() const { return _signals[3]; }
    uint8_t &mark() { return _signals[3]; }
    uint8_t rw() const { return _signals[0]; }
    uint8_t &rw() { return _signals[0]; }
    uint8_t bw() const { return _signals[1]; }
    uint8_t &bw() { return _signals[1]; }
    uint8_t st() const { return _signals[2]; }
    uint8_t &st() { return _signals[2]; }
};
}  // namespace z280
}  // namespace debugger
#endif /* __SIGNALS_Z280_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
