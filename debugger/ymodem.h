#ifndef __DEBUGGER_YMODEM_H__
#define __DEBUGGER_YMODEM_H__

#include <stddef.h>
#include <stdint.h>

namespace debugger {

/** The byte stream a YMODEM session runs on. */
struct YmodemLink {
    virtual ~YmodemLink() {}
    // The next byte, or -1 once |ms| pass without one; keeps the rest of
    // the firmware alive while it waits.
    virtual int get(uint32_t ms) = 0;
    virtual void put(const uint8_t *data, size_t len) = 0;
    void put(uint8_t c) { put(&c, 1); }
};

/** Where a received file goes. */
struct YmodemSink {
    virtual ~YmodemSink() {}
    // The sender's name and size for the file (0 when it gave no size);
    // false refuses the file.
    virtual bool open(const char *name, uint32_t size) = 0;
    // False when the data did not get there, which ends the session.
    virtual bool write(const uint8_t *data, uint16_t len) = 0;
};

/** Where a sent file comes from. */
struct YmodemSource {
    virtual ~YmodemSource() {}
    // The bytes read, fewer than |len| only at the end.
    virtual uint16_t read(uint8_t *data, uint16_t len) = 0;
};

/**
 * One file each way by YMODEM, in CRC mode with 1K blocks, as lrzsz and
 * minicom speak it. A session waits WAIT_MS for the other end to start,
 * which gives a person time to pick the file.
 */
struct Ymodem {
    enum Result : uint8_t {
        OK,
        TIMEOUT,    // the other end stopped answering
        CANCELLED,  // by the other end, or ^C while waiting for it
        REFUSED,    // the sink would not take the file
        FAILED,     // too many bad blocks, or the sink's write failed
    };
    static constexpr uint32_t WAIT_MS = 60000;

    Ymodem(YmodemLink &link) : _link(link) {}

    // Receives one file into |sink|; |received| counts its bytes. OK
    // with none received when the sender had no file.
    Result receive(YmodemSink &sink, uint32_t &received);
    // Sends |size| bytes of |source| under |name|.
    Result send(const char *name, uint32_t size, YmodemSource &source);

    static const char *name(Result result);
    // CRC-16/XMODEM, continued from |crc|.
    static uint16_t crc16(uint16_t crc, const uint8_t *data, size_t len);

private:
    enum : uint8_t {
        SOH = 0x01,
        STX = 0x02,
        EOT = 0x04,
        ACK = 0x06,
        NAK = 0x15,
        CAN = 0x18,
        CTRL_C = 0x03,
        PAD = 0x1A,
    };
    static constexpr uint32_t BYTE_MS = 1000;  // between the bytes of a block
    static constexpr uint32_t REPLY_MS = 10000;  // for the other end's answer
    // lrzsz's receiver flushes its input right after each 'C' and ACK it
    // sends: a block answered at once, as a pty or USB can, would be lost
    // to it. The sender lets the line settle first.
    static constexpr uint32_t SETTLE_MS = 20;  // after a 'C'
    static constexpr uint32_t GAP_MS = 2;  // after an ACK
    static constexpr uint32_t RETRY_MS = 1200;  // after a NAK: lrzsz clears the line
    static constexpr uint32_t QUIET_MS = 100;  // before a NAK for a bad block
    static constexpr uint32_t CLOSE_MS = 2000;  // for the closing header's ACK
    static constexpr uint8_t RETRIES = 10;

    YmodemLink &_link;
    uint8_t _block[1024];

    // The next block: its first byte (SOH, STX or EOT), or -1 on a
    // timeout, -2 on a cancel, -3 on a bad block. |seq| and |len| come
    // from a good one.
    int getBlock(uint8_t &seq, uint16_t &len, uint32_t firstMs);
    void putBlock(uint8_t seq, uint16_t len);
    // ACK or NAK, -1 on a timeout, -2 on a cancel, or the byte itself.
    int getReply();
    // Waits for the receiver's 'C'; false on a timeout or cancel.
    bool waitForC(Result &result);
    // Eats what arrives until |ms| pass quietly; false on a cancel.
    bool settle(uint32_t ms);
    void cancel();
};

}  // namespace debugger
#endif /* __DEBUGGER_YMODEM_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
