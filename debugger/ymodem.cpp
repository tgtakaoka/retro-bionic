#include "ymodem.h"
#include <stdlib.h>
#include <string.h>

namespace debugger {

uint16_t Ymodem::crc16(uint16_t crc, const uint8_t *data, size_t len) {
    while (len--) {
        crc ^= uint16_t(*data++) << 8;
        for (auto bit = 0; bit < 8; ++bit)
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
    }
    return crc;
}

const char *Ymodem::name(Result result) {
    switch (result) {
    case OK:
        return "done";
    case TIMEOUT:
        return "timeout";
    case CANCELLED:
        return "cancelled";
    case REFUSED:
        return "refused";
    default:
        return "failed";
    }
}

void Ymodem::cancel() {
    static constexpr uint8_t CANCELS[] = {CAN, CAN, CAN, CAN, CAN};
    _link.put(CANCELS, sizeof(CANCELS));
}

int Ymodem::getBlock(uint8_t &seq, uint16_t &len, uint32_t firstMs) {
    const auto head = _link.get(firstMs);
    if (head < 0)
        return -1;
    if (head == CAN || head == CTRL_C)
        return -2;
    if (head == EOT)
        return EOT;
    if (head != SOH && head != STX)
        return -3;
    len = head == STX ? 1024 : 128;
    uint8_t header[2];
    for (auto &h : header) {
        const auto c = _link.get(BYTE_MS);
        if (c < 0)
            return -1;
        h = c;
    }
    for (uint16_t i = 0; i < len; ++i) {
        const auto c = _link.get(BYTE_MS);
        if (c < 0)
            return -1;
        _block[i] = c;
    }
    uint16_t crc = 0;
    for (auto shift = 8; shift >= 0; shift -= 8) {
        const auto c = _link.get(BYTE_MS);
        if (c < 0)
            return -1;
        crc |= uint16_t(c) << shift;
    }
    if (header[0] != uint8_t(~header[1]) || crc16(0, _block, len) != crc)
        return -3;
    seq = header[0];
    return head;
}

Ymodem::Result Ymodem::receive(YmodemSink &sink, uint32_t &received) {
    received = 0;
    uint32_t size = 0;
    bool open = false;  // the file's header has arrived
    uint8_t expect = 0;
    uint8_t retries = 0;
    auto eot = 0;
    _link.put('C');
    for (uint32_t waited = 0;;) {
        uint8_t seq;
        uint16_t len;
        // Until the first block the sender may still be picking the file.
        const auto head = getBlock(seq, len, open ? REPLY_MS : 3000);
        if (head == -2) {
            cancel();
            return CANCELLED;
        }
        if (head == -1 && !open && retries == 0) {
            if ((waited += 3000) >= WAIT_MS)
                return TIMEOUT;
            _link.put('C');
            continue;
        }
        if (head < 0) {
            if (++retries > RETRIES) {
                cancel();
                return head == -1 ? TIMEOUT : FAILED;
            }
            // The rest of a block that lost a byte is not more blocks: let
            // it pass before the one NAK that asks for it again.
            if (head == -3 && !settle(QUIET_MS)) {
                cancel();
                return CANCELLED;
            }
            _link.put(NAK);
            continue;
        }
        retries = 0;
        if (head == EOT) {
            // The first EOT is doubted, the second ends the file. Then
            // 'C' asks for the next header, which an empty one answers.
            _link.put(++eot == 1 ? NAK : ACK);
            if (eot == 2)
                _link.put('C');
            continue;
        }
        if (seq == 0 && (!open || eot == 2)) {
            // A header: the name, then the size in decimal, each NUL ended.
            if (_block[0] == 0 || eot == 2) {
                // The end of the batch. One file is all we take: a second
                // is cancelled, but the first stays.
                if (_block[0] != 0)
                    cancel();
                else
                    _link.put(ACK);
                return OK;
            }
            _block[len - 1] = 0;
            const auto name = reinterpret_cast<const char *>(_block);
            size = strtoul(name + strlen(name) + 1, nullptr, 10);
            if (!sink.open(name, size)) {
                cancel();
                return REFUSED;
            }
            open = true;
            expect = 1;
            _link.put(ACK);
            _link.put('C');
            continue;
        }
        if (seq != expect) {
            // A block sent again after its ACK was lost.
            _link.put(seq == uint8_t(expect - 1) ? ACK : NAK);
            continue;
        }
        // Data, less the padding past the size the header gave.
        auto take = len;
        if (size && received + take > size)
            take = size - received;
        if (take && !sink.write(_block, take)) {
            cancel();
            return FAILED;
        }
        received += take;
        ++expect;
        _link.put(ACK);
    }
}

void Ymodem::putBlock(uint8_t seq, uint16_t len) {
    const uint8_t header[] = {uint8_t(len == 1024 ? STX : SOH), seq, uint8_t(~seq)};
    _link.put(header, sizeof(header));
    _link.put(_block, len);
    const auto crc = crc16(0, _block, len);
    const uint8_t trailer[] = {uint8_t(crc >> 8), uint8_t(crc)};
    _link.put(trailer, sizeof(trailer));
}

int Ymodem::getReply() {
    const auto c = _link.get(REPLY_MS);
    if (c == CAN || c == CTRL_C)
        return -2;
    return c;
}

bool Ymodem::settle(uint32_t ms) {
    // Two CANs in a row cancel: lrzsz sends a run of them, and the tail
    // of a damaged block may hold a single one.
    auto cans = 0;
    for (int c; (c = _link.get(ms)) >= 0;) {
        if (c == CAN && ++cans == 2)
            return false;
        if (c != CAN)
            cans = 0;
    }
    return true;
}

bool Ymodem::waitForC(Result &result) {
    for (uint32_t waited = 0; waited < WAIT_MS; waited += 1000) {
        const auto c = _link.get(1000);
        if (c == 'C') {
            if (settle(SETTLE_MS))
                return true;
            result = CANCELLED;
            return false;
        }
        if (c == CAN || c == CTRL_C) {
            result = CANCELLED;
            return false;
        }
    }
    result = TIMEOUT;
    return false;
}

Ymodem::Result Ymodem::send(const char *name, uint32_t size, YmodemSource &source) {
    Result result;
    if (!waitForC(result))
        return result;
    // The header, then the data in 1K blocks, the last padded.
    memset(_block, 0, sizeof(_block));
    strncpy(reinterpret_cast<char *>(_block), name, 100);
    char decimal[11];
    auto d = decimal + sizeof(decimal) - 1;
    *d = 0;
    for (auto n = size;; n /= 10) {
        *--d = '0' + n % 10;
        if (n < 10)
            break;
    }
    // The size, then a modification time of 0: the board keeps no clock,
    // and lrzsz stamps a file without one with garbage.
    auto p = reinterpret_cast<char *>(_block) + strlen(name) + 1;
    strcpy(p, d);
    strcat(p, " 0");
    uint8_t seq = 0;
    uint16_t len = 128;
    uint32_t sent = 0;
    bool header = true;  // the block out is the header, not data
    bool more = true;
    for (uint8_t retries = 0;;) {
        putBlock(seq, len);
        const auto reply = getReply();
        if (reply == -2) {
            cancel();
            return CANCELLED;
        }
        if (reply != ACK) {
            if (++retries > RETRIES) {
                cancel();
                return reply == -1 ? TIMEOUT : FAILED;
            }
            // lrzsz answers a bad block with NAK (or 'C', for the first
            // data block), then takes a second of quiet to clear the line;
            // a resend before that is noise to it.
            if (!settle(RETRY_MS)) {
                cancel();
                return CANCELLED;
            }
            continue;
        }
        retries = 0;
        if (header) {
            // The receiver asks for the data with another 'C'; an empty
            // file has none.
            header = false;
            if (!waitForC(result))
                return result;
            if (size == 0)
                break;
        } else if (!settle(GAP_MS)) {
            cancel();
            return CANCELLED;
        }
        if (!more)
            break;
        ++seq;  // wraps past block 255, as the protocol has it
        len = size - sent > 128 ? 1024 : 128;
        const auto got = source.read(_block, len);
        sent += got;
        if (got < len) {
            memset(_block + got, PAD, len - got);
            more = false;
        } else if (sent == size) {
            more = false;
        }
    }
    // EOT until it is acknowledged (the receiver doubts the first, and
    // now and then the second), then the empty header that ends the batch.
    for (uint8_t retries = 0;; ++retries) {
        _link.put(EOT);
        const auto reply = getReply();
        if (reply == ACK)
            break;
        if (reply == -2 || retries == RETRIES || !settle(RETRY_MS)) {
            cancel();
            return reply == -1 ? TIMEOUT : reply == -2 ? CANCELLED : FAILED;
        }
    }
    if (!waitForC(result))
        return result;
    memset(_block, 0, 128);
    for (uint8_t retries = 0;; ++retries) {
        putBlock(0, 128);
        const auto reply = _link.get(CLOSE_MS);
        // lrzsz's receiver flushes its output as it exits, which can take
        // this ACK with it: the file was acknowledged before, so a silence
        // here is as good.
        if (reply == ACK || reply == -1 || retries == RETRIES)
            return OK;
        if (reply == -2)
            return CANCELLED;
    }
}

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
