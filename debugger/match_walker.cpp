#include "match_walker.h"

#ifndef ARDUINO
#include <stdio.h>
#include <string.h>
#endif

namespace debugger {

#ifndef ARDUINO
bool MatchWalker::trace = false;
#define TRACE(...)               \
    do {                         \
        if (trace)               \
            printf(__VA_ARGS__); \
    } while (0)
#else
#define TRACE(...)
#endif

namespace {

// Walks tried, at most, before a ring is given up.
constexpr uint32_t BUDGET = 20000;

// The end of the alternative starting at |seq|.
const char *alternativeEnd(const char *seq) {
    while (*seq && *seq != '@' && *seq != '/')
        ++seq;
    return seq;
}

// The ']' closing the unordered group from |p|.
const char *unorderedEnd(const char *p, const char *end) {
    while (p < end && *p != ']')
        ++p;
    return p;
}

// The '}' closing the group whose '{' is just before |p|.
const char *groupEnd(const char *p, const char *end) {
    for (auto depth = 1; p < end; ++p) {
        if (*p == '{')
            ++depth;
        if (*p == '}' && --depth == 0)
            return p;
    }
    return end;
}

// An address step after a data token in a group: |step| is the bytes,
// |any| any address; returns the step's characters.
uint_fast8_t addressStep(
        const char *p, const char *end, int8_t &step, bool &any) {
    any = false;
    step = 0;
    if (p >= end)
        return 0;
    if (*p == '.') {
        any = true;
        return 1;
    }
    if (*p == '0') {
        step = 0;
        return 1;
    }
    if ((*p == '+' || *p == '-') && p + 1 < end && p[1] >= '1' && p[1] <= '9') {
        step = (*p == '+' ? 1 : -1) * (p[1] - '0');
        return 2;
    }
    return 0;
}

// where a transfer the ring doesn't show goes: wherever an interrupt's
// push says
constexpr uint32_t ANY_PC = UINT32_MAX;

bool isNextFetch(char c) {
    return c == 'N' || c == 'J' || c == 'P' || c == '?';
}

bool isData(char c) {
    return c == 'R' || c == 'A' || c == 'V' || c == 'r' || c == 'W' ||
           c == 'B' || c == 'w' || c == 'I' || c == 'O';
}

// The token a data token's second byte meets.
char continuation(char c) {
    switch (c) {
    case 'W':
    case 'B':
    case 'w':
        return 'w';
    case 'I':
    case 'O':
        return c;
    default:
        return 'r';
    }
}

}  // namespace

// What a match went through: the cycle after it, where it goes next, and
// what it moved.
struct MatchWalker::Match {
    uint_fast8_t i;
    uint32_t fetch;      // the next instruction byte's address
    uint32_t value;      // what its reads put together
    uint8_t valueBytes;  // how many
    bool hasRead;
    uint32_t readAt;    // the last read cycle's address
    uint32_t lastRead;  // the last byte it read
    bool hasWrite;
    uint32_t writeAt;        // the last write cycle's address
    uint32_t lastWrite;      // the last byte it wrote
    bool interrupt;          // its first write pushes the PC
    const SignalsImpl *ack;  // its acknowledge
    bool moved;              // it matched a data or transfer token
    bool executed;           // it matched a token past its bytes
    bool ended;              // the ring ended in it
    bool hasNext;            // it knows where the next instruction is
    uint32_t next;           // where that is
    uint8_t fetchAt;         // the cycle that fetched its first byte
    bool biased;             // it knows where its fetches are on the bus
    uint32_t bias;           // that: + its PC
    Queue queue;             // its bytes fetched ahead, then the next one's
    bool sawNext;            // its next fetch is in the ring
    bool sawVector;          // it read the vector table
};

uint_fast8_t MatchWalker::instructionBytes(const char *seq) {
    const auto end = alternativeEnd(seq);
    uint_fast8_t bytes = 0;
    for (auto p = seq; p < end; ++p) {
        if (*p == '+' || *p == '-') {
            // an address step's digit isn't an instruction byte
            if (p + 1 < end && p[1] >= '1' && p[1] <= '9')
                ++p;
        } else if (*p == '1' || *p == '2') {
            ++bytes;
        }
    }
    return bytes;
}

#ifndef ARDUINO
const char *MatchWalker::illFormed(
        const char *seq, const char *own, bool instruction) {
    auto bytes = -1;  // each alternative's, which all of a variant's share
    for (auto p = seq; *p;) {
        const auto end = alternativeEnd(p);
        const char *group = nullptr;    // the open {
        const char *bracket = nullptr;  // the open [
        auto fetches = 0;               // next fetches in [ ]
        auto n = 0;
        for (auto q = p; q < end; ++q) {
            const auto c = *q;
            if (group && q > p && isData(q[-1])) {
                int8_t step;
                bool any;
                if (const auto k = addressStep(q, end, step, any)) {
                    q += k - 1;
                    continue;
                }
            }
            if (c == '{') {
                if (group || bracket)
                    return "a group inside a group or [ ]";
                group = q;
            } else if (c == '}') {
                if (group == nullptr)
                    return "a } without its {";
                if (q - group - 1 > 16)
                    return "a group longer than its 16 slots";
                group = nullptr;
            } else if (c == '[') {
                if (group || bracket)
                    return "a [ inside a group or [ ]";
                bracket = q;
                fetches = 0;
            } else if (c == ']') {
                if (bracket == nullptr)
                    return "a ] without its [";
                bracket = nullptr;
            } else if (bracket) {
                if (isNextFetch(c)) {
                    if (++fetches > 1)
                        return "two next fetches in [ ]";
                } else if (!isData(c)) {
                    return "a token [ ] can't hold";
                }
            } else if (c == '1' || c == '2') {
                ++n;
            } else if (strchr("nNJP?RrWwABVIOXx!h-~", c) == nullptr &&
                       strchr(own, c) == nullptr) {
                return "a character no token is";
            }
        }
        if (group)
            return "a { without its }";
        if (bracket)
            return "a [ without its ]";
        if (instruction && bytes >= 0 && n != bytes &&
                strchr(own, '#') == nullptr)
            return "alternatives of different lengths";
        bytes = n;
        if (*end == '/')
            bytes = -1;  // a variant: its own length
        p = *end ? end + 1 : end;
    }
    return nullptr;
}
#endif

// A walk that reaches the ring's end there, the fetch stream at |stream|:
// at the stop PC when it's known, or before instructions that go on to it,
// all but the last fetched by the stream, and confirmed. A start past the
// ring's first cycle is a guess, as is the first where its last is a next
// fetch: a next fetch seen inside the ring must confirm it.
bool MatchWalker::endsAt(uint32_t pc, uint32_t stream) const {
    const auto mask = _arch->traits.addressMask;
    auto fetched = true;
    while (_stop != NO_STOP && !_arch->sameAddress(pc, _stop)) {
        Decoded inst;
        if (!fetched || !_arch->decode(pc, inst) || inst.length == 0)
            return false;
        const auto ahead = (stream - pc) & mask;
        fetched = ahead != 0 && ahead <= Queue::MAX;
        pc = (pc + inst.length) & mask;
    }
    return confirmed();
}

// The walk ends at |pc|, the queue holding its bytes |fetched|: the
// instructions there the stream fetched ran to the stop with no cycles of
// their own, and the one at the stop is where the CPU is. Always true.
bool MatchWalker::ranTo(uint32_t pc, const Queue &fetched) {
    const auto mask = _arch->traits.addressMask;
    const auto base = pc;
    while (_stop != NO_STOP && _steps < MAX_STEPS) {
        const auto k = (pc - base) & mask;
        if (k >= fetched.n)
            break;
        auto &step = _step[_steps++];
        step.pc = pc;
        step.first = _size;
        step.cycles = 0;
        step.fetchAt = fetched.at[k];
        step.sawNext = false;
        step.interrupt = false;
        step.moved = false;
        Decoded inst;
        if (_arch->sameAddress(pc, _stop) || !_arch->decode(pc, inst) ||
                inst.length == 0)
            break;
        pc = (pc + inst.length) & mask;
    }
    return true;
}

// Whether the ring may end inside an instruction fetched at |fetchAt| that
// took |cycles| from its first: where its last cycle is a next fetch, only
// before it took any of its own past that fetch.
bool MatchWalker::cutAt(uint8_t fetchAt, uint_fast8_t cycles) const {
    return !_arch->traits.lastIsNextFetch || fetchAt + 1u == _size ||
           cycles == 0;
}

// Whether an instruction the ring cuts is the one the CPU stopped at.
bool MatchWalker::atStop(uint32_t pc) const {
    return _stop != NO_STOP && _arch->sameAddress(pc, _stop) && confirmed();
}

bool MatchWalker::confirmed() const {
    if (_from == 0 && !_before && !_arch->traits.lastIsNextFetch)
        return true;
    for (auto n = 0u; n < _steps; ++n) {
        if (_step[n].sawNext)
            return true;
    }
    TRACE("    nothing confirms the walk from %u\n", (unsigned)_from);
    return false;
}

// One alternative, [seq, end), of |inst| from cycle |m.i|; its first
// bytes already fetched at the cycles |queued| holds.
bool MatchWalker::matchAlternative(const Decoded &inst, const char *seq,
        const char *end, const Queue &queued, Match &m) {
    const auto &arch = *_arch;
    const auto bigEndian = arch.traits.bigEndian;
    const auto mask = arch.traits.addressMask;
    m.fetchAt = NO_FETCH;
    m.fetch = inst.pc;
    m.queue = queued;
    // the first fetch tells where an MMU maps the instruction's page: its
    // other fetches follow from there
    m.biased = queued.n != 0;
    m.bias = queued.bias;
    // which of a fetch's bytes onStream() found |addr| at: its first, but
    // where a word fetch may be read again for its later byte
    uint_fast8_t onAt = 0;
    const auto words = arch.traits.refetchWord;
    auto onStream = [&](const SignalsImpl *q, uint32_t addr) {
        if (_kind[m.i] != K_READ)
            return false;
        const auto bytes = words ? arch.fetchBytes(q) : 1;
        if (m.biased) {
            const uint32_t on = (addr + m.bias) & mask;
            if (on < q->addr || on - q->addr >= bytes)
                return false;
            onAt = on - q->addr;
            return true;
        }
        for (onAt = 0; onAt < bytes; ++onAt) {
            if (arch.sameAddress(q->addr + onAt, addr & mask)) {
                m.biased = true;
                m.bias = q->addr + onAt - addr;
                return true;
            }
        }
        return false;
    };
    // a fetch's bytes after the |used| first: the queue's
    auto queueRest = [&](const SignalsImpl *q, uint_fast8_t used) {
        for (auto k = used; k < arch.fetchBytes(q) && m.queue.n < Queue::MAX;
                ++k)
            m.queue.push(m.i);
        m.queue.bias = m.bias;
    };
    // a stall fetches again what the stream fetched of the instruction
    auto refetched = [&](const SignalsImpl *q, uint32_t stream) {
        const auto from = inst.pc;
        for (auto k = 1u; k <= arch.traits.refetch && k <= stream - from; ++k) {
            if (onStream(q, stream - k))
                return true;
        }
        return false;
    };
    // past ~, the queue fetches ahead between its tokens
    bool queueing = false;
    const uint8_t capacity =
            arch.traits.queue < Queue::MAX ? arch.traits.queue : Queue::MAX;
    // the last address each token of a group had, by its position
    uint32_t last[16];
    bool seen[16];
    const char *group = nullptr;  // the open group's first token
    const char *groupClose = nullptr;
    uint_fast8_t groupStart = 0;  // the cycle its iteration began at
    Match saved;                  // the state its iteration began with
    // an iteration not done: its cycles back, unmarked
    auto giveBack = [&](const Match &from) {
        for (auto j = from.i; j < m.i; ++j)
            _role[j] = R_NONE;
        m = from;
    };
    auto p = seq;
    auto fail = [&](const char *why) {
        TRACE("    %X: %c at cycle %u %s\n", (unsigned)inst.pc, *p,
                (unsigned)m.i, why);
        return false;
    };
    // the last word fetch read again, a few cycles on at most, once the
    // stream has taken all of it
    auto repeated = [&](const SignalsImpl *q, uint32_t stream) {
        if (!words || arch.fetchBytes(q) < 2 ||
                q->addr + arch.fetchBytes(q) > ((stream + m.bias) & mask))
            return false;
        for (auto j = m.i; j > 0 && j + 3 > m.i; --j) {
            const auto role = _role[j - 1];
            if (role == R_BYTE || role == R_FETCH)
                return _kind[j - 1] == K_READ && at(j - 1)->addr == q->addr;
        }
        return false;
    };
    // where the token fails at it, a word fetch ahead the queue had too
    // little room for: what it took is read again, or lost
    auto dropFetch = [&]() {
        if (!words || !queueing || m.i >= _size)
            return false;
        const auto q = at(m.i);
        if (arch.fetchBytes(q) < 2 || !onStream(q, m.fetch + m.queue.n))
            return false;
        _role[m.i++] = R_BYTE;
        if (m.i > _written)
            _written = m.i;
        return true;
    };
    // past ~, the queue fetches ahead; a stall fetches again what the
    // stream has already fetched
    auto fetchAhead = [&]() {
        while (m.i < _size) {
            const auto q = at(m.i);
            const auto stream = m.fetch + m.queue.n;
            if (queueing &&
                    (words || m.queue.n + arch.fetchBytes(q) <= capacity) &&
                    onStream(q, stream) &&
                    m.queue.n + arch.fetchBytes(q) - onAt <= capacity) {
                queueRest(q, onAt);
            } else if (!refetched(q, stream) && !repeated(q, stream)) {
                break;
            }
            _role[m.i++] = R_BYTE;
            if (m.i > _written)
                _written = m.i;
        }
    };
    // |p|'s token against the cycle at m.i, and the cycles it takes;
    // |after| is the token past it, or past both where a cycle met two
    auto matchToken = [&](const char *p, const char *&after,
                              bool exact = false) {
        const auto c = *p;
        const auto s = at(m.i);
        const auto kind = _kind[m.i];
        int8_t step = 0;
        bool any = false;
        const auto stepChars = group ? addressStep(p + 1, end, step, any) : 0;
        const auto slot = group ? (p - group) & 15 : 0;
        // inside a group, a stepped token's address from its last: then
        // the step alone decides it
        const auto byStep = group && stepChars && !any && seen[slot];
        auto stepped = [&](uint32_t addr) {
            if (!byStep)
                return true;
            return addr == ((last[slot] + step) & mask);
        };
        // a data cycle that moves two bytes meets two tokens where the
        // next goes on from this one; else only the first byte is its
        const auto pair = p + 1 + stepChars;
        const uint_fast8_t width = isData(c) && arch.dataBytes(s) > 1 &&
                                                   pair < end &&
                                                   *pair == continuation(c)
                                           ? 2
                                           : 1;
        if (exact && isData(c) && arch.dataBytes(s) > width)
            return false;  // moves more than its tokens
        const auto lastByte = (s->addr + width - 1) & mask;
        auto read = [&](bool starts) {
            for (auto k = 0u; k < width; ++k) {
                const uint32_t b = arch.dataByte(s, k);
                if (starts && k == 0) {
                    m.value = b;
                    m.valueBytes = 1;
                } else if (bigEndian) {
                    m.value = (m.value << 8) | b;
                } else if (m.valueBytes < sizeof(m.value)) {
                    m.value |= b << (8 * m.valueBytes++);
                }
            }
            m.hasRead = true;
            m.readAt = s->addr;
            m.lastRead = lastByte;
        };
        auto write = [&]() {
            if (m.interrupt && !m.hasWrite &&
                    !arch.pushesPc(s, inst.pc,
                            _steps ? _step[_steps - 1].pc : inst.pc))
                return false;
            m.hasWrite = true;
            m.writeAt = s->addr;
            m.lastWrite = lastByte;
            m.moved = true;
            return true;
        };
        Role role = R_DATA;
        bool ok = false;
        switch (c) {
        case '1': {
            // the fetch may bring it after a byte, and more after it
            auto k = 0u;
            while (k < arch.fetchBytes(s) &&
                    !arch.sameAddress(s->addr + k, inst.pc & mask))
                ++k;
            ok = kind == K_READ && k < arch.fetchBytes(s);
            m.biased = ok;
            m.bias = s->addr + k - inst.pc;
            m.fetch = inst.pc + 1;
            m.fetchAt = m.i;
            role = R_FETCH;
            if (ok)
                queueRest(s, k + 1);
            break;
        }
        case '2':
            ok = onStream(s, m.fetch);
            ++m.fetch;
            role = R_BYTE;
            if (ok)
                queueRest(s, onAt + 1);
            break;
        case 'n':
            ok = onStream(s, m.fetch);
            role = R_BYTE;
            break;
        case 'R':
        case 'A':
        case 'V':
            ok = kind == K_READ &&
                 (c != 'A' || !inst.hasEa || s->addr == inst.ea) &&
                 (c != 'V' || arch.isVectorTable(s->addr)) && stepped(s->addr);
            if (ok) {
                read(true);
                m.moved = true;
                if (c == 'V')
                    m.sawVector = true;
            }
            break;
        case 'r':
            ok = kind == K_READ &&
                 (byStep ||
                         (m.hasRead && s->addr == ((m.lastRead + 1) & mask))) &&
                 stepped(s->addr);
            if (ok)
                read(false);
            break;
        case 'W':
        case 'B': {
            auto placed = [&]() {
                if (byStep)
                    return true;
                if (c == 'B' && inst.hasEa)
                    return s->addr == inst.ea;
                return !m.hasWrite || s->addr == ((m.writeAt - width) & mask) ||
                       (m.hasRead && s->addr == m.readAt);
            };
            ok = kind == K_WRITE && placed() && stepped(s->addr) && write();
            break;
        }
        case 'w':
            ok = kind == K_WRITE &&
                 (byStep || !m.hasWrite ||
                         s->addr == ((m.lastWrite + 1) & mask)) &&
                 stepped(s->addr) && write();
            break;
        case 'I':
            ok = kind == K_IO_READ && stepped(s->addr);
            m.moved |= ok;
            break;
        case 'O':
            ok = kind == K_IO_WRITE && stepped(s->addr);
            m.moved |= ok;
            break;
        case '!':
            ok = kind == K_ACK;
            m.ack = s;
            break;
        case 'h':
            ok = kind == K_HALT;
            break;
        case 'X':
            ok = kind == K_READ;
            break;
        case 'x':
            ok = kind == K_READ && arch.isDummy(s);
            break;
        case '-':
            ok = kind == K_NONE;
            role = R_NONE;
            break;
        default:
            return fail("is no token");
        }
        if (!ok)
            return false;
        if (group) {
            last[slot] = s->addr;
            seen[slot] = true;
        }
        if (c != '1' && c != '2')
            m.executed = true;
        _role[m.i++] = role;
        if (m.i > _written)
            _written = m.i;
        after = width > 1 ? pair + 1 : pair;
        return true;
    };
    // [from, close): each kind's tokens in order, but the kinds and the
    // transfer in any, the fetch stream going on between them
    auto matchUnordered = [&](const char *from, const char *close) {
        enum { MEM_READ, MEM_WRITE, IO_READ, IO_WRITE, LANES };
        const char *head[LANES] = {};
        const char *tail[LANES] = {};
        char transfer = 0;
        for (auto q = from; q < close; ++q) {
            const auto c = *q;
            const int lane = (c == 'R' || c == 'r' || c == 'A' || c == 'V')
                                     ? MEM_READ
                             : (c == 'W' || c == 'w' || c == 'B') ? MEM_WRITE
                             : c == 'I'                           ? IO_READ
                             : c == 'O'                           ? IO_WRITE
                                                                  : LANES;
            if (lane == LANES) {
                if (!isNextFetch(c))
                    return fail("is no token in [ ]");
                transfer = c;
                continue;
            }
            if (head[lane] == nullptr)
                head[lane] = q;
            tail[lane] = q + 1;
        }
        uint32_t target = 0;
        bool known = transfer == 'J' || transfer == 'P';
        if (transfer == 'J') {
            if (!inst.hasTarget)
                return fail("has no target");
            target = inst.target;
        }
        auto busy = [&]() {
            if (transfer)
                return true;
            for (auto k = 0u; k < LANES; ++k) {
                if (head[k] && head[k] < tail[k])
                    return true;
            }
            return false;
        };
        // the transfer's fetch: the next instruction's, which the queue
        // holds from there
        auto transferred = [&](const SignalsImpl *q) {
            m.queue.n = 0;
            m.fetch = q->addr;
            m.hasNext = true;
            m.next = q->addr;
            m.sawNext = true;
            m.moved = m.executed = true;
            transfer = 0;
            queueRest(q, 0);
            _role[m.i++] = R_BYTE;
            if (m.i > _written)
                _written = m.i;
        };
        // the rest of the group from here; a read at the target that a
        // data token could take too is the transfer first, then the data
        auto rest = [&](auto &self) -> bool {
            while (busy()) {
                fetchAhead();
                if (m.i >= _size) {  // cut: where it goes, if it knows
                    m.ended = true;
                    m.hasNext = known && transfer;
                    m.next = target;
                    return true;
                }
                const auto q = at(m.i);
                if (transfer == 'P')
                    target = (m.value + arch.traits.targetBias) & mask;
                if (transfer && known && _kind[m.i] == K_READ &&
                        arch.sameAddress(q->addr, target)) {
                    const auto was = m;
                    const auto lanes = head[MEM_READ];
                    const auto wasTransfer = transfer;
                    transferred(q);
                    if (lanes == nullptr || lanes >= tail[MEM_READ])
                        continue;  // nothing else it could be
                    if (self(self))
                        return true;
                    m = was;
                    head[MEM_READ] = lanes;
                    transfer = wasTransfer;
                }
                auto matched = false;
                for (auto k = 0u; k < LANES && !matched; ++k) {
                    const char *after;
                    if (head[k] && head[k] < tail[k] &&
                            matchToken(head[k], after, true)) {
                        head[k] = after;
                        matched = true;
                    }
                }
                if (matched)
                    continue;
                if (transfer == '?' && _kind[m.i] == K_READ &&
                        (!m.interrupt || arch.entersInterrupt(q, m.ack))) {
                    transferred(q);
                    continue;
                }
                // an interrupt before the transfer's fetch: it pushes where
                // that would have been
                const auto kind = _kind[m.i];
                auto data = false;
                for (auto k = 0u; k < LANES; ++k)
                    data |= head[k] && head[k] < tail[k];
                if (transfer && !data && !m.interrupt &&
                        (kind == K_WRITE || kind == K_ACK)) {
                    m.queue.n = 0;
                    m.hasNext = true;
                    m.next = known ? target : ANY_PC;
                    m.moved = m.executed = true;
                    return true;
                }
                return fail("isn't wanted in [ ]");
            }
            return true;
        };
        return rest(rest);
    };
    while (true) {
        if (p >= end || (group && p == groupClose)) {
            if (group == nullptr)
                return true;
            // an iteration done: another, from its first token
            if (m.i == groupStart) {  // matched nothing: no more
                p = groupClose + 1;
                group = nullptr;
                continue;
            }
            groupStart = m.i;
            saved = m;
            p = group;
            continue;
        }
        const auto c = *p;
        if (c == '~') {
            queueing = true;
            ++p;
            continue;
        }
        fetchAhead();
        if (c == '{') {
            group = p + 1;
            groupClose = groupEnd(group, end);
            if (groupClose - group > 16)
                return fail("opens a group longer than its 16 slots");
            for (auto q = group; q < groupClose; ++q) {
                if (*q == '{')
                    return fail("nests a group");
            }
            groupStart = m.i;
            saved = m;
            for (auto k = 0u; k < 16; ++k)
                seen[k] = false;
            p = group;
            continue;
        }
        if (isNextFetch(c)) {
            uint32_t expect = 0;
            bool known = true;
            switch (c) {
            case 'N':
                expect = (inst.pc + inst.length) & mask;
                break;
            case 'J':
                if (!inst.hasTarget)
                    return fail("has no target");
                expect = inst.target;
                m.moved = true;
                break;
            case 'P':
                expect = (m.value + arch.traits.targetBias) & mask;
                m.moved = true;
                break;
            default:
                known = false;
                break;
            }
            m.executed = true;
            if (m.queue.n && c == 'N') {
                // the queue fetched it ahead: its head
                if ((m.fetch & mask) != expect)
                    return fail("is not where the queue is");
                m.hasNext = true;
                m.next = expect;
                m.sawNext = true;
                if (p + 1 == end)
                    return true;
                ++p;
                continue;
            }
            if (known && m.i < _size && _kind[m.i] == K_READ &&
                    !arch.sameAddress(at(m.i)->addr, expect) && dropFetch())
                continue;
            if (m.queue.n) {  // a transfer: what the queue fetched is lost
                m.queue.n = 0;
                queueing = false;
            }
            if (m.i >= _size) {
                m.ended = true;
                m.hasNext = known;
                m.next = expect;
                return true;
            }
            const auto s = at(m.i);
            const auto kind = _kind[m.i];
            if (known && p + 1 == end && !m.interrupt &&
                    (kind == K_WRITE || kind == K_ACK)) {
                // an interrupt before the fetch: it pushes where that is
                m.hasNext = true;
                m.next = expect;
                return true;
            }
            if (kind != K_READ || (known && !arch.sameAddress(s->addr, expect)))
                return fail("is not the next fetch");
            m.hasNext = true;
            m.next = s->addr;  // where the bus has it
            m.sawNext = true;
            if (p + 1 == end)
                return true;  // the next instruction's cycle, looked at
            // fetched ahead: this one's cycle, the next one's first byte
            m.queue.push(m.i);
            m.queue.bias = s->addr - m.next;
            _role[m.i++] = R_BYTE;
            if (m.i > _written)
                _written = m.i;
            ++p;
            continue;
        }
        if (c == '1' && m.queue.n) {
            // fetched ahead by the one before
            m.fetch = inst.pc + 1;
            m.fetchAt = m.queue.pop();
            ++p;
            continue;
        }
        if (c == '2' && m.queue.n) {
            m.queue.pop();
            ++m.fetch;
            ++p;
            continue;
        }
        // a token that takes a cycle: the ring may end before it, inside
        // the instruction it stopped in
        if (m.i >= _size) {
            // inside an iteration: its cycles are the instruction's
            m.ended = true;
            return true;
        }
        if (c == '[') {
            const auto close = unorderedEnd(p + 1, end);
            if (!matchUnordered(p + 1, close))
                return false;
            if (m.ended)
                return true;
            p = close + 1;
            continue;
        }
        const char *after;
        // a read of the stream is its fetch, not the data
        if ((c == 'R' || c == 'A' || c == 'X') && dropFetch())
            continue;
        if (!matchToken(p, after)) {
            if (dropFetch())
                continue;
            if (group) {  // the iteration stops: on after the group
                giveBack(saved);
                p = groupClose + 1;
                group = nullptr;
                continue;
            }
            return fail("doesn't match");
        }
        p = after;
    }
}

// The walk on from cycle |i|, the instruction at |pc|, its first bytes
// already fetched at the cycles |queued| holds.
bool MatchWalker::walkFrom(uint_fast8_t i, uint32_t pc, const Queue &queued) {
    if (i >= _size)
        return pc != ANY_PC &&
               cutAt(queued.n ? queued.at[0] : NO_FETCH, queued.n == 0) &&
               endsAt(pc, pc + queued.n) && ranTo(pc, queued);
    if (_arch->traits.lastIsNextFetch && i + 1 == _size && queued.n == 0)
        return endsAt(pc);
    if (_budget == 0 || --_budget == 0)
        return false;
    _arch->idle();
    Decoded inst;
    if (pc != ANY_PC && _arch->decode(pc, inst) &&
            walkOn(inst, i, queued, false))
        return true;
    const auto intr = _arch->interruptSequence();
    if (intr == nullptr)
        return false;
    inst.pc = pc;
    inst.seq = intr;
    inst.length = 0;
    inst.hasTarget = inst.hasEa = false;
    return walkOn(inst, i, queued, true);
}

// Each alternative of |inst| from cycle |i|, and the walk on after it.
bool MatchWalker::walkOn(const Decoded &inst, uint_fast8_t i,
        const Queue &queued, bool interrupt) {
    // an alternative the ring cuts in its bytes, where none walks
    const char *cut = nullptr;
    const char *cutEnd = nullptr;
    for (auto seq = inst.seq; *seq;) {
        const auto end = alternativeEnd(seq);
        Match m{};
        m.i = i;
        m.interrupt = interrupt;
        _written = i;
        const auto matched = matchAlternative(inst, seq, end, queued, m);
        // what it marked, a walk that fails leaves unmarked
        const auto written = _written;
        if (matched && m.ended && _arch->traits.queue > 1 && !m.executed) {
            // cut before anything of its own but its bytes, which the
            // queue fetched: not walked
            TRACE("    %X: cut in its bytes at cycle %u\n", (unsigned)inst.pc,
                    (unsigned)i);
            if (cut == nullptr && endsAt(inst.pc, m.fetch + m.queue.n)) {
                cut = seq;
                cutEnd = end;
            }
        } else if (matched && _steps < MAX_STEPS) {
            auto &step = _step[_steps++];
            step.pc = inst.pc;
            step.first = i;
            step.cycles = m.i - i;
            step.fetchAt = m.fetchAt;
            step.sawNext = m.sawNext;
            step.interrupt = interrupt;
            step.moved = m.moved || interrupt;
            if (m.ended) {
                // an interrupt cut before its vector is only any reads
                if (interrupt && !m.sawVector)
                    TRACE("    %X: interrupt cut before its vector\n",
                            (unsigned)inst.pc);
                else if (!cutAt(m.fetchAt, m.i - i))
                    TRACE("    %X: the ring can't end inside it\n",
                            (unsigned)inst.pc);
                else if (atStop(inst.pc) ||
                         (m.hasNext ? endsAt(m.next) : confirmed()))
                    return true;
            } else if (m.hasNext) {
                if (walkFrom(m.i, m.next, m.queue))
                    return true;
            } else if (m.i >= _size) {
                if (cutAt(NO_FETCH, m.i - i) && confirmed())
                    return true;  // where it went, the ring doesn't say
            } else if (_kind[m.i] == K_READ &&
                       walkFrom(m.i, at(m.i)->addr, Queue{})) {
                return true;  // no next fetch named: wherever it reads
            }
            --_steps;
        }
        for (auto j = i; j < written; ++j)
            _role[j] = R_NONE;
        if (*end != '@')
            break;
        seq = end + 1;
    }
    if (cut) {  // its cycles marked again
        Match m{};
        m.i = i;
        m.interrupt = interrupt;
        matchAlternative(inst, cut, cutEnd, queued, m);
        // it, then what the queue holds past it
        Queue fetched{};
        fetched.push(m.fetchAt);
        for (auto k = inst.pc + 1; k != m.fetch && fetched.n < Queue::MAX; ++k)
            fetched.push(NO_FETCH);
        for (auto k = 0u; k < m.queue.n && fetched.n < Queue::MAX; ++k)
            fetched.push(m.queue.at[k]);
        return ranTo(inst.pc, fetched);
    }
    TRACE("    %X: no %s fits at cycle %u\n", (unsigned)inst.pc,
            interrupt ? "interrupt" : "alternative", (unsigned)i);
    return false;
}

uint_fast16_t MatchWalker::owner(uint_fast8_t i) const {
    for (auto n = 0u; n < _steps; ++n) {
        if (i >= _step[n].first && i < _step[n].first + _step[n].cycles)
            return n;
    }
    // past the last: what its stream fetched ahead
    if (_steps && i >= _start && i < _size && _role[i] != R_NONE)
        return _steps - 1;
    return NOBODY;
}

MatchWalker &MatchWalker::shared() {
    static MatchWalker walker;
    return walker;
}

// The walk from cycle |i| found: its cycles marked.
bool MatchWalker::walked(uint_fast8_t i) {
    TRACE("  walked from %u:", (unsigned)i);
    for (auto n = 0u; n < _steps; ++n)
        TRACE(" %X@%d", (unsigned)_step[n].pc,
                _step[n].fetchAt == NO_FETCH ? -1 : _step[n].fetchAt);
    TRACE("\n");
    _start = i;
    for (auto j = 0u; j < i; ++j)
        _role[j] = R_NONE;
    uint8_t span[Cycles::MAX_CYCLES];
    for (auto j = 0u; j < _size; ++j) {
        span[j] = 0;
        if (_role[j] == R_FETCH)
            _role[j] = R_BYTE;
    }
    for (auto n = 0u; n < _steps; ++n) {
        const auto f = _step[n].fetchAt;
        if (f == NO_FETCH)
            continue;
        _role[f] = R_FETCH;
        span[f] = _step[n].cycles;
    }
    for (auto j = 0u; j < _size; ++j)
        _arch->markCycle(at(j), _role[j], span[j]);
    return true;
}

void MatchWalker::classify() {
    for (auto i = 0u; i < _size; ++i)
        _kind[i] = _arch->cycleKind(at(i));
}

uint_fast8_t MatchWalker::matchInstruction(
        const Arch &arch, SignalsImpl *begin, const SignalsImpl *end) {
    _arch = &arch;
    _begin = begin;
    _size = begin->diff(end);
    _stop = NO_STOP;
    _steps = 0;
    _written = 0;
    classify();
    Decoded inst;
    if (_size == 0 || _kind[0] != K_READ || !arch.decode(begin->addr, inst))
        return 0;
    for (auto seq = inst.seq; *seq;) {
        const auto alt = alternativeEnd(seq);
        Match m{};
        m.i = 0;
        if (matchAlternative(inst, seq, alt, Queue{}, m) && !m.ended)
            return m.i;
        if (*alt != '@')
            break;
        seq = alt + 1;
    }
    return 0;
}

bool MatchWalker::walk(const Arch &arch, SignalsImpl *begin,
        const SignalsImpl *end, uint32_t stop) {
    _arch = &arch;
    _begin = begin;
    _size = begin->diff(end);
    _stop = stop;
    _budget = BUDGET;
    _steps = 0;
    classify();
    const auto starts = arch.traits.maxStart && arch.traits.maxStart < _size
                                ? arch.traits.maxStart
                                : _size;
    // the ring begun inside a wait: its end, an interrupt's entry
    if (const auto resume = starts ? arch.resumeSequence() : nullptr) {
        TRACE("  from inside a wait\n");
        for (auto j = 0u; j < _size; ++j)
            _role[j] = R_NONE;
        _steps = 0;
        _from = 0;
        _before = true;
        Decoded wait;
        wait.pc = 0;
        wait.seq = resume;
        wait.length = 0;
        wait.hasTarget = wait.hasEa = false;
        if (walkOn(wait, 0, Queue{}, true))
            return walked(0);
    }
    for (auto i = 0u; i < starts; ++i) {
        if (_kind[i] != K_READ)
            continue;
        // at the ring's first cycle, also where it began inside an
        // instruction: its first bytes fetched before it
        const auto before = i == 0 ? arch.traits.cutStart : 0;
        auto walked = false;
        // or any byte the fetch brings, after its first
        const int bytes = arch.fetchBytes(at(i));
        for (int n = 0; n < bytes + int(before) && !walked; ++n) {
            const int k = n < bytes ? -n : n - bytes + 1;
            if (k >= Queue::MAX)
                break;
            const auto pc = (at(i)->addr - k) & arch.traits.addressMask;
            TRACE("  from %u at %X\n", i, (unsigned)pc);
            Queue fetched{};
            for (auto b = 0; b < k; ++b)
                fetched.push(NO_FETCH);
            for (auto j = 0u; j < _size; ++j)
                _role[j] = R_NONE;
            _steps = 0;
            _from = i;
            _before = k > 0;
            walked = walkFrom(i, pc, fetched);
        }
        if (walked && arch.traits.leadUnproven) {
            // the instructions ahead of the first with evidence: the
            // ring's, as is
            auto n = 0u;
            while (n < _steps && !_step[n].moved)
                ++n;
            if (n < _steps) {
                for (auto k = n; k < _steps; ++k)
                    _step[k - n] = _step[k];
                _steps -= n;
            }
        }
        if (walked)
            return this->walked(i);
        if (_budget == 0)
            break;
    }
    // where no walk reaches the stop, one that reaches the ring's end,
    // with a budget of its own
    if (stop != NO_STOP) {
        TRACE("  nothing reaches the stop %X\n", (unsigned)stop);
        return walk(arch, begin, end, NO_STOP);
    }
    _start = _size;
    _steps = 0;
    for (auto j = 0u; j < _size; ++j)
        arch.markCycle(at(j), R_NONE, 0);
    return false;
}

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
