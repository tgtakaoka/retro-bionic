#include "disk_cache.h"
#include <string.h>

namespace debugger {

void DiskCache::begin(uint8_t *arena, uint32_t bytes) {
    constexpr uint32_t SET_BYTES = (BLOCK + sizeof(Slot)) * WAYS;
    constexpr uint32_t ALIGN = 16;
    _sets = arena && bytes > ALIGN ? (bytes - ALIGN) / SET_BYTES : 0;
    if (_sets == 0) {
        _slots = nullptr;
        _data = nullptr;
        return;
    }
    const auto slots = _sets * WAYS;
    _slots = reinterpret_cast<Slot *>(arena);
    _data = arena + ((slots * sizeof(Slot) + ALIGN - 1) & ~(ALIGN - 1));
    for (auto i = 0U; i < slots; ++i) {
        _slots[i].file = FREE;
        _slots[i].state = 0;
    }
    _free = slots;
    for (auto f = 0; f < FILES; ++f)
        _dirty[f] = _written[f] = 0;
    _hits = _misses = 0;
}

bool DiskCache::enable(bool on) {
    if (!on && _on) {
        if (!syncAll())
            return false;
        for (auto f = 0; f < FILES; ++f)
            drop(f);
    }
    _on = on;
    return true;
}

bool DiskCache::setPolicy(Policy policy) {
    if (policy == WRITE_THROUGH && _policy == WRITE_BACK && !syncAll())
        return false;
    _policy = policy;
    return true;
}

void DiskCache::attach(uint8_t file, BlockStore *store) {
    drop(file);
    _stores[file] = store;
    _written[file] = false;
}

bool DiskCache::detach(uint8_t file) {
    const auto ok = sync(file);
    drop(file);
    _stores[file] = nullptr;
    return ok;
}

DiskCache::Slot *DiskCache::find(
        uint8_t file, uint32_t block, bool touch) const {
    const auto set = this->set(block);
    for (auto s = set; s < set + WAYS; ++s) {
        if (s->file == file && s->block == block) {
            if (touch)
                this->touch(set, s);
            return s;
        }
    }
    return nullptr;
}

void DiskCache::touch(Slot *set, Slot *latest) {
    for (auto s = set; s < set + WAYS; ++s) {
        if (s == latest) {
            s->state &= DIRTY;
        } else if (s->file != FREE && (s->state & AGE) != AGE) {
            ++s->state;
        }
    }
}

DiskCache::Slot *DiskCache::fill(uint8_t file, uint32_t block, uint16_t len) {
    const auto set = this->set(block);
    Slot *victim = nullptr;
    for (auto s = set; s < set + WAYS; ++s) {
        if (s->file == FREE) {
            victim = s;
            break;
        }
        // The oldest clean slot, else the oldest dirty one.
        if (victim == nullptr || (victim->state & DIRTY) > (s->state & DIRTY) ||
                ((victim->state & DIRTY) == (s->state & DIRTY) &&
                        (s->state & AGE) > (victim->state & AGE)))
            victim = s;
    }
    if (victim->file == FREE) {
        --_free;
    } else if ((victim->state & DIRTY) && !writeBack(victim)) {
        return nullptr;
    }
    victim->file = file;
    victim->block = block;
    victim->len = len;
    victim->state = 0;
    if (len && !_stores[file]->read(block * BLOCK, data(victim), len)) {
        release(victim);
        return nullptr;
    }
    touch(set, victim);
    return victim;
}

// A block short of 512 bytes ends where the file did: when the file has
// grown since, the new bytes are read in behind the ones cached.
bool DiskCache::refill(Slot *s) {
    const auto start = s->block * BLOCK;
    const auto size = _stores[s->file]->size();
    if (size <= start + s->len)
        return true;
    const auto len = size - start < BLOCK ? uint16_t(size - start) : BLOCK;
    if (!_stores[s->file]->read(start + s->len, data(s) + s->len, len - s->len))
        return false;
    s->len = len;
    return true;
}

bool DiskCache::writeBack(Slot *s) {
    _written[s->file] = true;
    if (!_stores[s->file]->write(s->block * BLOCK, data(s), s->len))
        return false;
    s->state &= AGE;
    --_dirty[s->file];
    return true;
}

void DiskCache::release(Slot *s) {
    if (s->state & DIRTY)
        --_dirty[s->file];
    s->file = FREE;
    s->state = 0;
    ++_free;
}

bool DiskCache::read(
        uint8_t file, uint32_t at, uint8_t *data, uint16_t len, uint16_t &got) {
    got = 0;
    auto store = _stores[file];
    if (!enabled()) {
        const auto size = store->size();
        if (at >= size)
            return true;
        const auto n = size - at < len ? uint16_t(size - at) : len;
        if (!store->read(at, data, n))
            return false;
        got = n;
        return true;
    }
    while (len) {
        const auto block = at / BLOCK;
        const auto off = uint16_t(at % BLOCK);
        const auto n = BLOCK - off < len ? uint16_t(BLOCK - off) : len;
        auto s = find(file, block);
        if (s == nullptr) {
            ++_misses;
            const auto start = block * BLOCK;
            const auto size = store->size();
            if (size <= start)
                return true;
            const auto valid =
                    size - start < BLOCK ? uint16_t(size - start) : BLOCK;
            s = fill(file, block, valid);
            if (s == nullptr)
                return false;
        } else {
            ++_hits;
            if (s->len < off + n && !refill(s))
                return false;
        }
        if (s->len <= off)
            return true;
        const auto avail = s->len - off < n ? uint16_t(s->len - off) : n;
        memcpy(data, this->data(s) + off, avail);
        got += avail;
        if (avail < n)
            return true;
        at += n;
        data += n;
        len -= n;
    }
    return true;
}

bool DiskCache::write(
        uint8_t file, uint32_t at, const uint8_t *data, uint16_t len) {
    auto store = _stores[file];
    if (!enabled()) {
        _written[file] = true;
        return store->write(at, data, len);
    }
    const auto through = _policy == WRITE_THROUGH;
    if (through) {
        _written[file] = true;
        if (!store->write(at, data, len))
            return false;
    }
    while (len) {
        const auto block = at / BLOCK;
        const auto off = uint16_t(at % BLOCK);
        const auto n = BLOCK - off < len ? uint16_t(BLOCK - off) : len;
        auto s = find(file, block);
        if (s == nullptr) {
            ++_misses;
            if (!through) {
                // Whatever the file holds around the new bytes comes in
                // with them; a whole block needs nothing.
                const auto start = block * BLOCK;
                const auto size = store->size();
                uint16_t valid = 0;
                if (n < BLOCK && size > start)
                    valid = size - start < BLOCK ? uint16_t(size - start)
                                                 : BLOCK;
                if (valid < off)
                    return false;  // the caller has not grown the file
                s = fill(file, block, valid);
                if (s == nullptr)
                    return false;
            }
        } else {
            ++_hits;
            // A block short of the new bytes takes what the file gained
            // (the caller grew it) first, so no stale byte turns valid.
            if (s->len < off && (!refill(s) || s->len < off))
                return false;
        }
        if (s) {
            memcpy(this->data(s) + off, data, n);
            if (s->len < off + n)
                s->len = off + n;
            if (!through && (s->state & DIRTY) == 0) {
                s->state |= DIRTY;
                ++_dirty[file];
            }
        }
        at += n;
        data += n;
        len -= n;
    }
    return true;
}

bool DiskCache::flush(uint8_t file) {
    if (enabled() && _policy == WRITE_BACK)
        return true;
    _written[file] = false;
    return _stores[file]->flush();
}

// The flush covers evictions and the caller's own writes too.
bool DiskCache::sync(uint8_t file) {
    auto store = _stores[file];
    if (store == nullptr || (_dirty[file] == 0 && !_written[file]))
        return true;
    auto left = 0U;
    const auto end = _slots + _sets * WAYS;
    for (auto s = _slots; _dirty[file] > left && s < end; ++s) {
        if (s->file == file && (s->state & DIRTY) && !writeBack(s))
            ++left;
    }
    _written[file] = false;
    return store->flush() && left == 0;
}

bool DiskCache::syncAll() {
    auto ok = true;
    for (auto f = 0; f < FILES; ++f) {
        if (!sync(f))
            ok = false;
    }
    return ok;
}

uint32_t DiskCache::preload(uint8_t file, uint32_t at, uint32_t len) {
    if (!enabled() || len == 0)
        return 0;
    const auto first = at / BLOCK;
    const auto last = (at + len - 1) / BLOCK;
    const auto size = _stores[file]->size();
    uint32_t have = 0;
    for (auto block = first; block <= last && _free; ++block) {
        const auto start = block * BLOCK;
        if (size <= start)
            break;
        if (find(file, block, false) == nullptr) {
            // Only into a free slot: a preload evicts nothing.
            if (!hasFree(set(block)))
                continue;
            const auto valid =
                    size - start < BLOCK ? uint16_t(size - start) : BLOCK;
            if (fill(file, block, valid) == nullptr)
                break;
        }
        ++have;
    }
    return have;
}

bool DiskCache::hasFree(const Slot *set) {
    for (auto s = set; s < set + WAYS; ++s) {
        if (s->file == FREE)
            return true;
    }
    return false;
}

void DiskCache::invalidate(uint8_t file, uint32_t at, uint32_t len) {
    if (_sets == 0 || len == 0)
        return;
    const auto first = at / BLOCK;
    const auto last = (at + len - 1) / BLOCK;
    if (last - first >= _sets * WAYS) {
        drop(file);
        return;
    }
    for (auto block = first; block <= last; ++block) {
        if (const auto s = find(file, block, false))
            release(s);
    }
}

void DiskCache::drop(uint8_t file) {
    const auto end = _slots + _sets * WAYS;
    for (auto s = _slots; s < end; ++s) {
        if (s->file == file)
            release(s);
    }
}

uint32_t DiskCache::dirty() const {
    uint32_t n = 0;
    for (auto d : _dirty)
        n += d;
    return n;
}

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
