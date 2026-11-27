#ifndef __DEBUGGER_DISK_CACHE_H__
#define __DEBUGGER_DISK_CACHE_H__

#include <stdint.h>

namespace debugger {

/**
 * The card file behind one cached file: what misses fill from and dirty
 * blocks go back to. A write may extend the file; a read never does.
 */
struct BlockStore {
    virtual ~BlockStore() {}
    virtual uint32_t size() = 0;
    virtual bool read(uint32_t at, uint8_t *data, uint16_t len) = 0;
    virtual bool write(uint32_t at, const uint8_t *data, uint16_t len) = 0;
    virtual bool flush() = 0;
};

/**
 * A cache of 512-byte file blocks in an arena the target lends, over up
 * to FILES attached stores; units sharing a file share its blocks. Blocks
 * are 8-way set-associative by block number, so a sync writes a run of
 * dirty blocks in ascending order, and the slot table sits in the arena
 * too. Each block keeps the number of bytes that are valid in it, so a
 * flush never extends a file past what the writes themselves reached.
 *
 * Under WRITE_THROUGH a write goes to the store at once and only updates
 * blocks already cached; under WRITE_BACK it allocates and leaves the
 * block dirty until a sync, an eviction, or the file's detach. A write
 * expects the store to reach |at| already: the caller grows it first.
 *
 * read(), write() and flush() run inside a held bus cycle: no output.
 */
struct DiskCache {
    static constexpr uint16_t BLOCK = 512;
    static constexpr uint8_t WAYS = 8;
    static constexpr uint8_t FILES = 8;
    enum Policy : uint8_t { WRITE_THROUGH, WRITE_BACK };

    // Takes |bytes| at |arena| for the slots and blocks; fewer than a
    // set's worth leaves the cache without memory.
    void begin(uint8_t *arena, uint32_t bytes);
    // The bytes of blocks the arena holds; 0 without memory.
    uint32_t capacity() const { return _sets * WAYS * uint32_t(BLOCK); }
    bool enabled() const { return _on && _sets != 0; }
    // Turning the cache off syncs and drops every block first; false, and
    // still on, when some block stayed dirty.
    bool enable(bool on);
    bool on() const { return _on; }
    Policy policy() const { return _policy; }
    // Back to through syncs first; false, and still back, when some
    // block stayed dirty.
    bool setPolicy(Policy policy);

    // |file| is the caller's index of the store; its blocks are dropped.
    void attach(uint8_t file, BlockStore *store);
    // Syncs and drops the file's blocks; false when some stayed dirty.
    bool detach(uint8_t file);

    // [at, at+len) of |file| into |data|: |got| is what is there, short
    // at the file's end. False on a store failure.
    bool read(uint8_t file, uint32_t at, uint8_t *data, uint16_t len,
            uint16_t &got);
    bool write(uint8_t file, uint32_t at, const uint8_t *data, uint16_t len);
    // The store's flush under write-through; nothing under write-back.
    bool flush(uint8_t file);
    // Writes the file's dirty blocks and flushes a store written since
    // its last flush; false when some block stayed dirty, to be retried
    // by the next sync.
    bool sync(uint8_t file);
    bool syncAll();
    // Reads [at, at+len) of |file| in, a block at a time, as far as free
    // slots go: the blocks of the range then cached, those already in
    // counted. For the prompt, not a bus cycle.
    uint32_t preload(uint8_t file, uint32_t at, uint32_t len);
    // The bytes [at, at+len) of |file| in the arena, for a copy that
    // never touches the card: null when one cached block does not hold
    // them all. |dirty| marks the block written, under write-back only.
    uint8_t *cached(uint8_t file, uint32_t at, uint16_t len, bool dirty);
    // Slots never filled, or freed since.
    uint32_t free() const { return _free; }
    // Forgets the blocks over [at, at+len): the caller wrote the store
    // itself.
    void invalidate(uint8_t file, uint32_t at, uint32_t len);
    // The caller wrote the store itself, past its cached blocks.
    void wrote(uint8_t file) { _written[file] = true; }
    void drop(uint8_t file);

    uint32_t dirty(uint8_t file) const { return _dirty[file]; }
    uint32_t dirty() const;
    uint32_t hits() const { return _hits; }
    uint32_t misses() const { return _misses; }

private:
    struct Slot {
        uint32_t block;
        uint16_t len;   // valid bytes from 0
        uint8_t file;   // FREE when unused
        uint8_t state;  // DIRTY | age, 0 the latest used
    };
    static constexpr uint8_t FREE = 0xFF;
    static constexpr uint8_t DIRTY = 0x80;
    static constexpr uint8_t AGE = 0x7F;

    Slot *_slots = nullptr;
    uint8_t *_data = nullptr;
    uint32_t _sets = 0;
    uint32_t _free = 0;
    BlockStore *_stores[FILES] = {};
    uint32_t _dirty[FILES] = {};
    bool _written[FILES] = {};  // since the store's last flush
    uint32_t _hits = 0;
    uint32_t _misses = 0;
    Policy _policy = WRITE_THROUGH;
    bool _on = true;

    Slot *set(uint32_t block) const { return _slots + (block % _sets) * WAYS; }
    uint8_t *data(const Slot *s) const { return _data + (s - _slots) * BLOCK; }
    // The slot holding |block|, made the latest used when |touch|.
    Slot *find(uint8_t file, uint32_t block, bool touch = true) const;
    // A slot for |block| with its first |len| bytes read from the store,
    // evicting the oldest clean, else the oldest dirty, block of the set.
    Slot *fill(uint8_t file, uint32_t block, uint16_t len);
    // Reads bytes the store gained since the block was filled.
    bool refill(Slot *s);
    bool writeBack(Slot *s);
    void release(Slot *s);
    static void touch(Slot *set, Slot *latest);
    static bool hasFree(const Slot *set);
};

}  // namespace debugger
#endif /* __DEBUGGER_DISK_CACHE_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
