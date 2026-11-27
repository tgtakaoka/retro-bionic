// Host-side tests for the disk block cache over a memory-backed store
// that counts its reads and writes: `pio test -e native -f test_disk_cache`.

#include <unity.h>

#include <string.h>
#include <vector>

#include "disk_cache.h"

using namespace debugger;

namespace {

constexpr auto BLOCK = DiskCache::BLOCK;

struct MemStore : BlockStore {
    std::vector<uint8_t> bytes;
    unsigned reads = 0, writes = 0, flushes = 0;
    std::vector<uint32_t> writtenAt;  // the offset of each write
    unsigned failWrites = 0;          // writes left to fail
    bool failReads = false;

    MemStore(uint32_t size = 0, uint8_t fill = 0) : bytes(size, fill) {}
    uint32_t size() override { return bytes.size(); }
    bool read(uint32_t at, uint8_t *data, uint16_t len) override {
        ++reads;
        if (failReads || at + len > bytes.size())
            return false;
        memcpy(data, &bytes[at], len);
        return true;
    }
    bool write(uint32_t at, const uint8_t *data, uint16_t len) override {
        ++writes;
        if (failWrites) {
            --failWrites;
            return false;
        }
        writtenAt.push_back(at);
        if (at + len > bytes.size())
            bytes.resize(at + len);
        memcpy(&bytes[at], data, len);
        return true;
    }
    bool flush() override {
        ++flushes;
        return true;
    }
};

// Sixteen sets of eight blocks, plus the slot table.
constexpr uint32_t SETS = 16;
constexpr uint32_t ARENA = (BLOCK + 8) * 8 * SETS + 16;
alignas(16) uint8_t arena[ARENA];

DiskCache cache;
MemStore *store0, *store1;
uint8_t buf[2 * BLOCK];

void expectRead(uint8_t file, uint32_t at, uint16_t len, uint16_t want,
        const uint8_t *expect) {
    uint16_t got = 0xFFFF;
    TEST_ASSERT_TRUE(cache.read(file, at, buf, len, got));
    TEST_ASSERT_EQUAL_UINT16(want, got);
    if (want && expect)
        TEST_ASSERT_EQUAL_UINT8_ARRAY(expect, buf, want);
}

void test_begin_sizes_sets() {
    TEST_ASSERT_TRUE(cache.enabled());
    TEST_ASSERT_EQUAL_UINT32(SETS * 8 * BLOCK, cache.capacity());
    DiskCache small;
    small.begin(arena, 100);
    TEST_ASSERT_FALSE(small.enabled());
    TEST_ASSERT_EQUAL_UINT32(0, small.capacity());
    small.begin(nullptr, 1 << 20);
    TEST_ASSERT_FALSE(small.enabled());
}

void test_read_hits_after_one_fill() {
    expectRead(0, 128, 128, 128, &store0->bytes[128]);
    TEST_ASSERT_EQUAL_UINT(1, store0->reads);
    TEST_ASSERT_EQUAL_UINT32(1, cache.misses());
    expectRead(0, 0, 128, 128, &store0->bytes[0]);
    expectRead(0, 384, 128, 128, &store0->bytes[384]);
    TEST_ASSERT_EQUAL_UINT(1, store0->reads);
    TEST_ASSERT_EQUAL_UINT32(2, cache.hits());
}

void test_read_spans_blocks_and_ends_at_eof() {
    // 256 bytes astride blocks 0 and 1.
    expectRead(0, 384, 256, 256, &store0->bytes[384]);
    TEST_ASSERT_EQUAL_UINT(2, store0->reads);
    // The file ends inside the last sector asked for.
    expectRead(0, 4 * BLOCK - 100, 256, 100, &store0->bytes[4 * BLOCK - 100]);
    // Wholly past the end: nothing, and no store access.
    const auto reads = store0->reads;
    expectRead(0, 4 * BLOCK, 128, 0, nullptr);
    expectRead(0, 9 * BLOCK, 512, 0, nullptr);
    TEST_ASSERT_EQUAL_UINT(reads, store0->reads);
}

void test_write_through_writes_at_once_and_updates_hits() {
    uint8_t data[128];
    memset(data, 0xAA, sizeof(data));
    // A miss: the store takes it and the cache does not allocate.
    TEST_ASSERT_TRUE(cache.write(0, 128, data, 128));
    TEST_ASSERT_EQUAL_UINT(1, store0->writes);
    TEST_ASSERT_EQUAL_UINT32(0, cache.dirty());
    TEST_ASSERT_EQUAL_UINT8(0xAA, store0->bytes[128]);
    expectRead(0, 128, 128, 128, data);  // fills from the store
    TEST_ASSERT_EQUAL_UINT(1, store0->reads);
    // A hit: the store takes it and the cached block follows.
    memset(data, 0x55, sizeof(data));
    TEST_ASSERT_TRUE(cache.write(0, 256, data, 128));
    TEST_ASSERT_EQUAL_UINT(2, store0->writes);
    expectRead(0, 256, 128, 128, data);
    TEST_ASSERT_EQUAL_UINT(1, store0->reads);
    // flush() reaches the store under write-through.
    TEST_ASSERT_TRUE(cache.flush(0));
    TEST_ASSERT_EQUAL_UINT(1, store0->flushes);
}

void test_write_back_keeps_dirty_until_sync() {
    cache.setPolicy(DiskCache::WRITE_BACK);
    uint8_t data[128];
    memset(data, 0xAA, sizeof(data));
    TEST_ASSERT_TRUE(cache.write(0, 128, data, 128));
    TEST_ASSERT_EQUAL_UINT(0, store0->writes);
    TEST_ASSERT_EQUAL_UINT(1, store0->reads);  // the rest of the block
    TEST_ASSERT_EQUAL_UINT32(1, cache.dirty(0));
    TEST_ASSERT_EQUAL_UINT8(128, store0->bytes[128]);  // still the old
    expectRead(0, 0, 512, 512, nullptr);
    TEST_ASSERT_EQUAL_UINT8(0, buf[0]);
    TEST_ASSERT_EQUAL_UINT8(0xAA, buf[128]);
    TEST_ASSERT_EQUAL_UINT8(0xAA, buf[255]);
    TEST_ASSERT_EQUAL_UINT8(44, buf[300]);
    // flush() is nothing under write-back.
    TEST_ASSERT_TRUE(cache.flush(0));
    TEST_ASSERT_EQUAL_UINT(0, store0->flushes);
    TEST_ASSERT_EQUAL_UINT(0, store0->writes);
    // A second write to the same block stays one dirty block.
    TEST_ASSERT_TRUE(cache.write(0, 0, data, 128));
    TEST_ASSERT_EQUAL_UINT32(1, cache.dirty());
    TEST_ASSERT_TRUE(cache.sync(0));
    TEST_ASSERT_EQUAL_UINT(1, store0->writes);
    TEST_ASSERT_EQUAL_UINT(1, store0->flushes);
    TEST_ASSERT_EQUAL_UINT32(0, cache.dirty());
    TEST_ASSERT_EQUAL_UINT8(0xAA, store0->bytes[0]);
    TEST_ASSERT_EQUAL_UINT8(0xAA, store0->bytes[255]);
    TEST_ASSERT_EQUAL_UINT8(44, store0->bytes[300]);
    TEST_ASSERT_EQUAL_UINT32(4 * BLOCK, store0->bytes.size());
}

void test_write_back_preserves_file_length() {
    // An ibm-3740 image is 256256 bytes: 500 blocks and 256 bytes.
    store0->bytes.assign(256256, 0xE5);
    cache.setPolicy(DiskCache::WRITE_BACK);
    uint8_t data[128];
    memset(data, 1, sizeof(data));
    // The last sector of the image, in the short block.
    TEST_ASSERT_TRUE(cache.write(0, 256256 - 128, data, 128));
    TEST_ASSERT_TRUE(cache.sync(0));
    TEST_ASSERT_EQUAL_UINT32(256256, store0->bytes.size());
    TEST_ASSERT_EQUAL_UINT8(1, store0->bytes[256256 - 1]);
    TEST_ASSERT_EQUAL_UINT8(0xE5, store0->bytes[256256 - 129]);
    // A sector appended right at the end extends the file by its size.
    TEST_ASSERT_TRUE(cache.write(0, 256256, data, 128));
    TEST_ASSERT_TRUE(cache.sync(0));
    TEST_ASSERT_EQUAL_UINT32(256256 + 128, store0->bytes.size());
}

void test_write_back_needs_the_file_grown() {
    cache.setPolicy(DiskCache::WRITE_BACK);
    uint8_t data[128];
    // Block 6 starts past the 4-block file: a whole block is fine...
    TEST_ASSERT_TRUE(cache.write(0, 6 * BLOCK, data, BLOCK));
    // ... and so is a sector at the file's very end...
    TEST_ASSERT_TRUE(cache.write(0, 4 * BLOCK, data, 128));
    // ... but a sector past it needs the gap filled first.
    TEST_ASSERT_FALSE(cache.write(0, 5 * BLOCK + 128, data, 128));
}

void test_write_hit_on_a_short_block_takes_the_file_first() {
    // The file ends 128 bytes into block 4, which is cached short.
    store0->bytes.resize(4 * BLOCK + 128, 0x33);
    expectRead(0, 4 * BLOCK, 128, 128, &store0->bytes[4 * BLOCK]);
    cache.setPolicy(DiskCache::WRITE_BACK);
    uint8_t data[128];
    memset(data, 0x55, sizeof(data));
    // Not grown: the write past the block's end is refused...
    TEST_ASSERT_FALSE(cache.write(0, 4 * BLOCK + 384, data, 128));
    // ... and grown with fill, the fill comes in behind the cached bytes.
    store0->bytes.resize(4 * BLOCK + 384, 0xE5);
    TEST_ASSERT_TRUE(cache.write(0, 4 * BLOCK + 384, data, 128));
    expectRead(0, 4 * BLOCK, 512, 512, nullptr);
    TEST_ASSERT_EQUAL_UINT8(0x33, buf[127]);
    TEST_ASSERT_EQUAL_UINT8(0xE5, buf[128]);
    TEST_ASSERT_EQUAL_UINT8(0xE5, buf[383]);
    TEST_ASSERT_EQUAL_UINT8(0x55, buf[384]);
    TEST_ASSERT_TRUE(cache.sync(0));
    TEST_ASSERT_EQUAL_UINT8(0xE5, store0->bytes[4 * BLOCK + 200]);
    TEST_ASSERT_EQUAL_UINT8(0x55, store0->bytes[4 * BLOCK + 511]);
    TEST_ASSERT_EQUAL_UINT32(5 * BLOCK, store0->bytes.size());
}

void test_sync_flushes_a_store_written_by_evictions() {
    store0->bytes.assign(10 * SETS * BLOCK, 0);
    cache.setPolicy(DiskCache::WRITE_BACK);
    uint8_t data[BLOCK];
    for (auto i = 0; i < 8; ++i)
        TEST_ASSERT_TRUE(cache.write(0, i * SETS * BLOCK, data, BLOCK));
    TEST_ASSERT_TRUE(cache.sync(0));
    TEST_ASSERT_EQUAL_UINT(1, store0->flushes);
    // Nothing written since: a sync is free.
    TEST_ASSERT_TRUE(cache.sync(0));
    TEST_ASSERT_EQUAL_UINT(1, store0->flushes);
    // Eight dirty blocks and a ninth evicts one to the card...
    for (auto i = 0; i < 8; ++i)
        TEST_ASSERT_TRUE(cache.write(0, i * SETS * BLOCK, data, BLOCK));
    expectRead(0, 9 * SETS * BLOCK, 16, 16, nullptr);
    TEST_ASSERT_EQUAL_UINT(9, store0->writes);
    // ... then every one of them is flushed at the next sync.
    TEST_ASSERT_TRUE(cache.sync(0));
    TEST_ASSERT_EQUAL_UINT(2, store0->flushes);
    TEST_ASSERT_EQUAL_UINT32(0, cache.dirty());
    // So is a file the caller grew itself.
    cache.wrote(0);
    TEST_ASSERT_TRUE(cache.sync(0));
    TEST_ASSERT_EQUAL_UINT(3, store0->flushes);
}

void test_failed_sync_keeps_the_cache_on_and_back() {
    cache.setPolicy(DiskCache::WRITE_BACK);
    uint8_t data[BLOCK];
    TEST_ASSERT_TRUE(cache.write(1, 0, data, BLOCK));
    store1->failWrites = 1;
    TEST_ASSERT_FALSE(cache.setPolicy(DiskCache::WRITE_THROUGH));
    TEST_ASSERT_EQUAL(DiskCache::WRITE_BACK, cache.policy());
    store1->failWrites = 1;
    TEST_ASSERT_FALSE(cache.enable(false));
    TEST_ASSERT_TRUE(cache.enabled());
    TEST_ASSERT_EQUAL_UINT32(1, cache.dirty());
    TEST_ASSERT_TRUE(cache.enable(false));
    TEST_ASSERT_EQUAL_UINT32(0, cache.dirty());
}

void test_sync_writes_in_ascending_order_and_once() {
    cache.setPolicy(DiskCache::WRITE_BACK);
    uint8_t data[BLOCK];
    for (auto b = 7; b >= 0; --b) {
        memset(data, b, sizeof(data));
        TEST_ASSERT_TRUE(cache.write(1, b * BLOCK, data, BLOCK));
    }
    TEST_ASSERT_EQUAL_UINT32(8, cache.dirty(1));
    TEST_ASSERT_EQUAL_UINT(0, store1->writes);
    TEST_ASSERT_TRUE(cache.sync(1));
    TEST_ASSERT_EQUAL_UINT(8, store1->writes);
    for (auto b = 0U; b < 8; ++b)
        TEST_ASSERT_EQUAL_UINT32(b * BLOCK, store1->writtenAt[b]);
    TEST_ASSERT_EQUAL_UINT32(8 * BLOCK, store1->bytes.size());
    TEST_ASSERT_EQUAL_UINT8(3, store1->bytes[3 * BLOCK + 7]);
    // Nothing left: a sync touches only the flush.
    TEST_ASSERT_TRUE(cache.sync(1));
    TEST_ASSERT_EQUAL_UINT(8, store1->writes);
}

void test_two_offsets_share_blocks() {
    // 128-byte sectors at 0 and a 512-byte unit at 1 K, in one file.
    uint8_t data[128];
    memset(data, 0x11, sizeof(data));
    cache.setPolicy(DiskCache::WRITE_BACK);
    TEST_ASSERT_TRUE(cache.write(0, 1024 + 128, data, 128));
    // The 512-byte unit reads block 2 through the same slot.
    expectRead(0, 1024, 512, 512, nullptr);
    TEST_ASSERT_EQUAL_UINT8(0x11, buf[128]);
    TEST_ASSERT_EQUAL_UINT8(store0->bytes[1024 + 300], buf[300]);
    TEST_ASSERT_EQUAL_UINT(1, store0->reads);
    TEST_ASSERT_EQUAL_UINT32(1, cache.dirty());
}

void test_eviction_prefers_clean_and_writes_dirty_back() {
    // Blocks b, b+16, b+32 ... fall in one set; nine of them overflow it.
    store0->bytes.assign(10 * SETS * BLOCK, 0);
    cache.setPolicy(DiskCache::WRITE_BACK);
    uint8_t data[BLOCK];
    memset(data, 0xD1, sizeof(data));
    TEST_ASSERT_TRUE(cache.write(0, 0, data, BLOCK));  // block 0 dirty
    for (auto i = 1; i <= 7; ++i)
        expectRead(0, i * SETS * BLOCK, 16, 16, nullptr);  // 7 clean
    TEST_ASSERT_EQUAL_UINT(0, store0->writes);
    // The eighth miss evicts the oldest clean one, block 16, not block 0.
    expectRead(0, 8 * SETS * BLOCK, 16, 16, nullptr);
    TEST_ASSERT_EQUAL_UINT(0, store0->writes);
    expectRead(0, 0, 16, 16, data);  // still a hit
    const auto hits = cache.hits();
    expectRead(0, 1 * SETS * BLOCK, 16, 16, nullptr);  // refilled
    TEST_ASSERT_EQUAL_UINT32(hits, cache.hits());
    // Make every slot of the set dirty, then one more forces a write.
    for (auto i = 1; i <= 7; ++i)
        TEST_ASSERT_TRUE(cache.write(0, i * SETS * BLOCK, data, BLOCK));
    TEST_ASSERT_EQUAL_UINT(0, store0->writes);
    TEST_ASSERT_EQUAL_UINT32(8, cache.dirty());
    expectRead(0, 9 * SETS * BLOCK, 16, 16, nullptr);
    TEST_ASSERT_EQUAL_UINT(1, store0->writes);
    TEST_ASSERT_EQUAL_UINT32(7, cache.dirty());
}

void test_failed_sync_keeps_blocks_dirty_for_retry() {
    cache.setPolicy(DiskCache::WRITE_BACK);
    uint8_t data[BLOCK];
    memset(data, 0x77, sizeof(data));
    for (auto b = 0; b < 3; ++b)
        TEST_ASSERT_TRUE(cache.write(1, b * BLOCK, data, BLOCK));
    store1->failWrites = 2;
    TEST_ASSERT_FALSE(cache.sync(1));
    TEST_ASSERT_EQUAL_UINT32(2, cache.dirty(1));
    TEST_ASSERT_EQUAL_UINT(3, store1->writes);
    TEST_ASSERT_TRUE(cache.sync(1));
    TEST_ASSERT_EQUAL_UINT32(0, cache.dirty(1));
    TEST_ASSERT_EQUAL_UINT(5, store1->writes);
    TEST_ASSERT_EQUAL_UINT32(3 * BLOCK, store1->bytes.size());
}

void test_failed_eviction_fails_the_access_and_keeps_the_block() {
    store0->bytes.assign(10 * SETS * BLOCK, 0);
    cache.setPolicy(DiskCache::WRITE_BACK);
    uint8_t data[BLOCK];
    for (auto i = 0; i < 8; ++i)
        TEST_ASSERT_TRUE(cache.write(0, i * SETS * BLOCK, data, BLOCK));
    store0->failWrites = 1;
    uint16_t got;
    TEST_ASSERT_FALSE(cache.read(0, 8 * SETS * BLOCK, buf, 16, got));
    TEST_ASSERT_EQUAL_UINT32(8, cache.dirty());
    TEST_ASSERT_TRUE(cache.read(0, 8 * SETS * BLOCK, buf, 16, got));
    TEST_ASSERT_EQUAL_UINT32(7, cache.dirty());
}

void test_invalidate_and_drop_forget_blocks() {
    expectRead(0, 0, 16, 16, nullptr);
    expectRead(0, BLOCK, 16, 16, nullptr);
    TEST_ASSERT_EQUAL_UINT(2, store0->reads);
    // The caller wrote the store itself.
    store0->bytes[5] = 0xBB;
    cache.invalidate(0, 0, 8);
    expectRead(0, 0, 16, 16, nullptr);
    TEST_ASSERT_EQUAL_UINT8(0xBB, buf[5]);
    TEST_ASSERT_EQUAL_UINT(3, store0->reads);
    expectRead(0, BLOCK, 16, 16, nullptr);  // untouched, still cached
    TEST_ASSERT_EQUAL_UINT(3, store0->reads);
    cache.drop(0);
    expectRead(0, BLOCK, 16, 16, nullptr);
    TEST_ASSERT_EQUAL_UINT(4, store0->reads);
    // A range wider than the cache drops the file.
    cache.invalidate(0, 0, 1u << 30);
    expectRead(0, BLOCK, 16, 16, nullptr);
    TEST_ASSERT_EQUAL_UINT(5, store0->reads);
}

void test_refill_sees_a_file_grown_behind_a_short_block() {
    // The 4-block file: block 3 is full; make a 5th, short block.
    store0->bytes.resize(4 * BLOCK + 100, 0x33);
    expectRead(0, 4 * BLOCK, 100, 100, &store0->bytes[4 * BLOCK]);
    // The caller grows the file (as DiskUnit::grow does) past the cache.
    store0->bytes.resize(4 * BLOCK + 300, 0x44);
    expectRead(0, 4 * BLOCK + 100, 200, 200, &store0->bytes[4 * BLOCK + 100]);
    TEST_ASSERT_EQUAL_UINT8(0x44, buf[0]);
}

void test_detach_syncs_and_drops() {
    cache.setPolicy(DiskCache::WRITE_BACK);
    uint8_t data[BLOCK] = {9};
    TEST_ASSERT_TRUE(cache.write(1, 0, data, BLOCK));
    TEST_ASSERT_TRUE(cache.detach(1));
    TEST_ASSERT_EQUAL_UINT(1, store1->writes);
    TEST_ASSERT_EQUAL_UINT32(0, cache.dirty());
    // Attaching another store to the slot sees none of the old blocks.
    MemStore other(BLOCK, 0x5A);
    cache.attach(1, &other);
    expectRead(1, 0, 16, 16, &other.bytes[0]);
    cache.detach(1);
}

void test_policy_back_to_through_syncs() {
    cache.setPolicy(DiskCache::WRITE_BACK);
    uint8_t data[BLOCK] = {9};
    TEST_ASSERT_TRUE(cache.write(1, 0, data, BLOCK));
    TEST_ASSERT_TRUE(cache.setPolicy(DiskCache::WRITE_THROUGH));
    TEST_ASSERT_EQUAL_UINT32(0, cache.dirty());
    TEST_ASSERT_EQUAL_UINT(1, store1->writes);
}

void test_preload_reads_an_image_in_once() {
    store0->bytes.resize(4 * BLOCK + 100, 0x42);
    TEST_ASSERT_EQUAL_UINT32(SETS * 8, cache.free());
    // A unit at 1 K: three blocks, the last short.
    TEST_ASSERT_EQUAL_UINT32(3, cache.preload(0, 1024, 3 * BLOCK));
    TEST_ASSERT_EQUAL_UINT(3, store0->reads);
    TEST_ASSERT_EQUAL_UINT32(SETS * 8 - 3, cache.free());
    expectRead(0, 1024, 512, 512, &store0->bytes[1024]);
    expectRead(0, 4 * BLOCK, 128, 100, &store0->bytes[4 * BLOCK]);
    TEST_ASSERT_EQUAL_UINT(3, store0->reads);
    TEST_ASSERT_EQUAL_UINT32(0, cache.misses());
    // Blocks already in count but are not read again; two are new.
    TEST_ASSERT_EQUAL_UINT32(5, cache.preload(0, 0, 5 * BLOCK));
    TEST_ASSERT_EQUAL_UINT(5, store0->reads);
    // A file shorter than asked ends the preload at its end.
    TEST_ASSERT_EQUAL_UINT32(5, cache.preload(0, 0, 9 * BLOCK));
    TEST_ASSERT_EQUAL_UINT(5, store0->reads);
}

void test_preload_takes_what_fits_and_evicts_nothing() {
    store0->bytes.assign(20 * SETS * BLOCK, 0);
    // More blocks than slots: the head of the image, up to the last free
    // slot, and no eviction.
    TEST_ASSERT_EQUAL_UINT32(SETS * 8, cache.preload(0, 0, 20 * SETS * BLOCK));
    TEST_ASSERT_EQUAL_UINT32(0, cache.free());
    TEST_ASSERT_EQUAL_UINT(SETS * 8, store0->reads);
    expectRead(0, 0, 16, 16, nullptr);
    expectRead(0, (SETS * 8 - 1) * BLOCK, 16, 16, nullptr);
    TEST_ASSERT_EQUAL_UINT32(0, cache.misses());
    // Now a second image finds no room.
    TEST_ASSERT_EQUAL_UINT32(0, cache.preload(1, 0, BLOCK));
    // A set left full by other blocks is skipped, not evicted.
    cache.drop(0);
    store1->bytes.assign(9 * SETS * BLOCK, 0x5A);
    for (auto i = 0; i < 8; ++i)
        expectRead(1, i * SETS * BLOCK, 16, 16, &store1->bytes[0]);
    TEST_ASSERT_EQUAL_UINT32(SETS * 8 - 8, cache.free());
    TEST_ASSERT_EQUAL_UINT32(
            SETS * 8 - 8, cache.preload(0, 0, 20 * SETS * BLOCK));
    TEST_ASSERT_EQUAL_UINT32(0, cache.free());
    expectRead(1, 0, 16, 16, &store1->bytes[0]);  // still there
    // When the cache is off there is nothing to read into.
    cache.drop(0);
    cache.enable(false);
    TEST_ASSERT_EQUAL_UINT32(0, cache.preload(0, 0, BLOCK));
}

void test_disabled_cache_passes_through() {
    cache.setPolicy(DiskCache::WRITE_BACK);
    uint8_t data[128];
    memset(data, 0xCC, sizeof(data));
    TEST_ASSERT_TRUE(cache.write(0, 0, data, 128));
    TEST_ASSERT_TRUE(cache.enable(false));  // syncs first
    TEST_ASSERT_FALSE(cache.enabled());
    const auto counted = cache.hits() + cache.misses();
    TEST_ASSERT_EQUAL_UINT(1, store0->writes);
    TEST_ASSERT_EQUAL_UINT8(0xCC, store0->bytes[0]);
    TEST_ASSERT_TRUE(cache.write(0, 128, data, 128));
    TEST_ASSERT_EQUAL_UINT(2, store0->writes);
    expectRead(0, 4 * BLOCK - 64, 128, 64, &store0->bytes[4 * BLOCK - 64]);
    expectRead(0, 0, 128, 128, data);
    TEST_ASSERT_EQUAL_UINT(3, store0->reads);
    TEST_ASSERT_EQUAL_UINT32(counted, cache.hits() + cache.misses());
    TEST_ASSERT_TRUE(cache.flush(0));  // the sync flushed once already
    TEST_ASSERT_EQUAL_UINT(2, store0->flushes);
    TEST_ASSERT_TRUE(cache.enable(true));
    TEST_ASSERT_TRUE(cache.enabled());
}

}  // namespace

// cached() hands out a block's bytes for a copy the card never sees.
void test_cached_points_into_a_whole_block_only() {
    store0->bytes.resize(4 * BLOCK);
    // Nothing is in yet: a miss, and no fill.
    TEST_ASSERT_NULL(cache.cached(0, 256, 256, false));
    TEST_ASSERT_EQUAL_UINT32(1, cache.misses());
    TEST_ASSERT_EQUAL_UINT(0, store0->reads);
    TEST_ASSERT_EQUAL_UINT32(2, cache.preload(0, 0, 2 * BLOCK));
    auto p = cache.cached(0, 256, 256, false);
    TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(&store0->bytes[256], p, 256);
    // Both sectors of one block at once, but never across blocks.
    TEST_ASSERT_NOT_NULL(cache.cached(0, 0, 512, false));
    TEST_ASSERT_NULL(cache.cached(0, 256, 512, false));
    // A short block does not reach bytes past its end.
    store0->bytes.resize(2 * BLOCK + 100);
    TEST_ASSERT_EQUAL_UINT32(1, cache.preload(0, 2 * BLOCK, BLOCK));
    TEST_ASSERT_NULL(cache.cached(0, 2 * BLOCK, 256, false));
    TEST_ASSERT_NOT_NULL(cache.cached(0, 2 * BLOCK, 100, false));
    TEST_ASSERT_EQUAL_UINT(3, store0->reads);
}

void test_cached_dirty_needs_write_back() {
    cache.preload(0, 0, BLOCK);
    TEST_ASSERT_NULL(cache.cached(0, 0, 256, true));
    TEST_ASSERT_TRUE(cache.setPolicy(DiskCache::WRITE_BACK));
    auto p = cache.cached(0, 256, 256, true);
    TEST_ASSERT_NOT_NULL(p);
    memset(p, 0x5A, 256);
    TEST_ASSERT_EQUAL_UINT32(1, cache.dirty(0));
    TEST_ASSERT_EQUAL_UINT(0, store0->writes);
    // Dirty once: a second hand-out does not count the block again.
    TEST_ASSERT_NOT_NULL(cache.cached(0, 0, 256, true));
    TEST_ASSERT_EQUAL_UINT32(1, cache.dirty(0));
    TEST_ASSERT_TRUE(cache.sync(0));
    TEST_ASSERT_EQUAL_UINT(1, store0->writes);
    TEST_ASSERT_EQUAL_UINT8(0x5A, store0->bytes[256]);
    TEST_ASSERT_EQUAL_UINT8(0x5A, store0->bytes[511]);
    TEST_ASSERT_EQUAL_UINT8(255, store0->bytes[255]);
    // A block not holding the file's bytes up to the sector cannot take
    // a write, the file has not grown there.
    store0->bytes.resize(2 * BLOCK + 100);
    cache.preload(0, 2 * BLOCK, BLOCK);
    TEST_ASSERT_NULL(cache.cached(0, 2 * BLOCK + 256, 256, true));
    // Writing up to a short block's end extends what is valid in it.
    TEST_ASSERT_NOT_NULL(cache.cached(0, 2 * BLOCK, 256, true));
    TEST_ASSERT_NOT_NULL(cache.cached(0, 2 * BLOCK + 256, 256, true));
    TEST_ASSERT_TRUE(cache.sync(0));
    TEST_ASSERT_EQUAL_UINT32(3 * BLOCK, store0->bytes.size());
}

void setUp() {
    cache = DiskCache();
    cache.begin(arena, ARENA);
    store0 = new MemStore(4 * BLOCK);
    store1 = new MemStore(0);
    for (auto i = 0U; i < store0->bytes.size(); ++i)
        store0->bytes[i] = i;
    cache.attach(0, store0);
    cache.attach(1, store1);
}

void tearDown() {
    delete store0;
    delete store1;
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_begin_sizes_sets);
    RUN_TEST(test_read_hits_after_one_fill);
    RUN_TEST(test_read_spans_blocks_and_ends_at_eof);
    RUN_TEST(test_write_through_writes_at_once_and_updates_hits);
    RUN_TEST(test_write_back_keeps_dirty_until_sync);
    RUN_TEST(test_write_back_preserves_file_length);
    RUN_TEST(test_write_back_needs_the_file_grown);
    RUN_TEST(test_write_hit_on_a_short_block_takes_the_file_first);
    RUN_TEST(test_sync_flushes_a_store_written_by_evictions);
    RUN_TEST(test_failed_sync_keeps_the_cache_on_and_back);
    RUN_TEST(test_sync_writes_in_ascending_order_and_once);
    RUN_TEST(test_two_offsets_share_blocks);
    RUN_TEST(test_eviction_prefers_clean_and_writes_dirty_back);
    RUN_TEST(test_failed_sync_keeps_blocks_dirty_for_retry);
    RUN_TEST(test_failed_eviction_fails_the_access_and_keeps_the_block);
    RUN_TEST(test_invalidate_and_drop_forget_blocks);
    RUN_TEST(test_refill_sees_a_file_grown_behind_a_short_block);
    RUN_TEST(test_detach_syncs_and_drops);
    RUN_TEST(test_policy_back_to_through_syncs);
    RUN_TEST(test_preload_reads_an_image_in_once);
    RUN_TEST(test_preload_takes_what_fits_and_evicts_nothing);
    RUN_TEST(test_disabled_cache_passes_through);
    RUN_TEST(test_cached_points_into_a_whole_block_only);
    RUN_TEST(test_cached_dirty_needs_write_back);
    return UNITY_END();
}

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
