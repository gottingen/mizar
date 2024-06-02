//  Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#include "cache/cache_key.h"

#include <algorithm>
#include <atomic>

#include "mizar/cache.h"
#include "table/unique_id_impl.h"
#include "util/hash.h"
#include "util/math.h"

namespace MIZAR_NAMESPACE {

// Value space plan for CacheKey:
//
// session_etc64_ | offset_etc64_ | Only generated by
// ---------------+---------------+------------------------------------------
//              0 |             0 | Reserved for "empty" CacheKey()
//              0 |  > 0, < 1<<63 | CreateUniqueForCacheLifetime
//              0 |      >= 1<<63 | CreateUniqueForProcessLifetime
//            > 0 |           any | OffsetableCacheKey.WithOffset

CacheKey CacheKey::CreateUniqueForCacheLifetime(Cache *cache) {
  // +1 so that we can reserve all zeros for "unset" cache key
  uint64_t id = cache->NewId() + 1;
  // Ensure we don't collide with CreateUniqueForProcessLifetime
  assert((id >> 63) == 0U);
  return CacheKey(0, id);
}

CacheKey CacheKey::CreateUniqueForProcessLifetime() {
  // To avoid colliding with CreateUniqueForCacheLifetime, assuming
  // Cache::NewId counts up from zero, here we count down from UINT64_MAX.
  // If this ever becomes a point of contention, we could use CoreLocalArray.
  static std::atomic<uint64_t> counter{UINT64_MAX};
  uint64_t id = counter.fetch_sub(1, std::memory_order_relaxed);
  // Ensure we don't collide with CreateUniqueForCacheLifetime
  assert((id >> 63) == 1U);
  return CacheKey(0, id);
}

// Value plan for CacheKeys from OffsetableCacheKey, assuming that
// db_session_ids are generated from a base_session_id and
// session_id_counter (by SemiStructuredUniqueIdGen+EncodeSessionId
// in DBImpl::GenerateDbSessionId):
//
// Conceptual inputs:
//   db_id                   (unstructured, from GenerateRawUniqueId or equiv)
//                           * could be shared between cloned DBs but rare
//                           * could be constant, if session id suffices
//   base_session_id         (unstructured, from GenerateRawUniqueId)
//   session_id_counter      (structured)
//                           * usually much smaller than 2**24
//   file_number             (structured)
//                           * usually smaller than 2**24
//   offset_in_file          (structured, might skip lots of values)
//                           * usually smaller than 2**32
//   max_offset              determines placement of file_number to prevent
//                           overlapping with offset
//
// Outputs come from bitwise-xor of the constituent pieces, low bits on left:
//
// |------------------------- session_etc64 -------------------------|
// | +++++++++++++++ base_session_id (lower 64 bits) +++++++++++++++ |
// |-----------------------------------------------------------------|
// | session_id_counter ...|                                         |
// |-----------------------------------------------------------------|
// |                                               | ... file_number |
// |                                               | overflow & meta |
// |-----------------------------------------------------------------|
//
//
// |------------------------- offset_etc64 --------------------------|
// | hash of: ++++++++++++++++++++++++++++++++++++++++++++++++++++++ |
// |  * base_session_id (upper ~39 bits)                             |
// |  * db_id (~122 bits entropy)                                    |
// |-----------------------------------------------------------------|
// | offset_in_file ............... |                                |
// |-----------------------------------------------------------------|
// |                                              | file_number, 0-3 |
// |                                              | lower bytes      |
// |-----------------------------------------------------------------|
//
// Based on max_offset, a maximal number of bytes 0..3 is chosen for
// including from lower bits of file_number in offset_etc64. The choice
// is encoded in two bits of metadata going into session_etc64, though
// the common case of 3 bytes is encoded as 0 so that session_etc64
// is unmodified by file_number concerns in the common case.
//
// There is nothing preventing "file number overflow & meta" from meeting
// and overlapping with session_id_counter, but reaching such a case requires
// an intractable combination of large file offsets (thus at least some large
// files), large file numbers (thus large number of files generated), and
// large number of session IDs generated in a single process. A trillion each
// (2**40) of session ids, offsets, and file numbers comes to 120 bits.
// With two bits of metadata and byte granularity, this is on the verge of
// overlap, but even in the overlap case, it doesn't seem likely that
// a file from billions of files or session ids ago will still be live
// or cached.
//
// In fact, if our SST files are all < 4TB (see
// BlockBasedTable::kMaxFileSizeStandardEncoding), then SST files generated
// in a single process are guaranteed to have unique cache keys, unless/until
// number session ids * max file number = 2**86, e.g. 1 trillion DB::Open in
// a single process and 64 trillion files generated. Even at that point, to
// see a collision we would need a miraculous re-synchronization of session
// id and file number, along with a live file or stale cache entry from
// trillions of files ago.
//
// How https://github.com/pdillinger/unique_id applies here:
// Every bit of output always includes "unstructured" uniqueness bits and
// often combines with "structured" uniqueness bits. The "unstructured" bits
// change infrequently: only when we cannot guarantee our state tracking for
// "structured" uniqueness hasn't been cloned. Using a static
// SemiStructuredUniqueIdGen for db_session_ids, this means we only get an
// "all new" session id when a new process uses RocksDB. (Between processes,
// we don't know if a DB or other persistent storage has been cloned.) Within
// a process, only the session_lower of the db_session_id changes
// incrementally ("structured" uniqueness).
//
// This basically means that our offsets, counters and file numbers allow us
// to do somewhat "better than random" (birthday paradox) while in the
// degenerate case of completely new session for each tiny file, we still
// have strong uniqueness properties from the birthday paradox, with ~103
// bit session IDs or up to 128 bits entropy with different DB IDs sharing a
// cache.
//
// More collision probability analysis:
// Suppose a RocksDB host generates (generously) 2 GB/s (10TB data, 17 DWPD)
// with average process/session lifetime of (pessimistically) 4 minutes.
// In 180 days (generous allowable data lifespan), we generate 31 million GB
// of data, or 2^55 bytes, and 2^16 "all new" session IDs.
//
// First, suppose this is in a single DB (lifetime 180 days):
// 128 bits cache key size
// - 55 <- ideal size for byte offsets + file numbers
// -  2 <- bits for offsets and file numbers not exactly powers of two
// -  2 <- bits for file number encoding metadata
// +  2 <- bits saved not using byte offsets in BlockBasedTable::GetCacheKey
// ----
//   71 <- bits remaining for distinguishing session IDs
// The probability of a collision in 71 bits of session ID data is less than
// 1 in 2**(71 - (2 * 16)), or roughly 1 in a trillion. And this assumes all
// data from the last 180 days is in cache for potential collision, and that
// cache keys under each session id exhaustively cover the remaining 57 bits
// while in reality they'll only cover a small fraction of it.
//
// Although data could be transferred between hosts, each host has its own
// cache and we are already assuming a high rate of "all new" session ids.
// So this doesn't really change the collision calculation. Across a fleet
// of 1 million, each with <1 in a trillion collision possibility,
// fleetwide collision probability is <1 in a million.
//
// Now suppose we have many DBs per host, say 2**10, with same host-wide write
// rate and process/session lifetime. File numbers will be ~10 bits smaller
// and we will have 2**10 times as many session IDs because of simultaneous
// lifetimes. So now collision chance is less than 1 in 2**(81 - (2 * 26)),
// or roughly 1 in a billion.
//
// Suppose instead we generated random or hashed cache keys for each
// (compressed) block. For 1KB compressed block size, that is 2^45 cache keys
// in 180 days. Collision probability is more easily estimated at roughly
// 1 in 2**(128 - (2 * 45)) or roughly 1 in a trillion (assuming all
// data from the last 180 days is in cache, but NOT the other assumption
// for the 1 in a trillion estimate above).
//
// Conclusion: Burning through session IDs, particularly "all new" IDs that
// only arise when a new process is started, is the only way to have a
// plausible chance of cache key collision. When processes live for hours
// or days, the chance of a cache key collision seems more plausibly due
// to bad hardware than to bad luck in random session ID data.
//
OffsetableCacheKey::OffsetableCacheKey(const std::string &db_id,
                                       const std::string &db_session_id,
                                       uint64_t file_number,
                                       uint64_t max_offset) {
#ifndef NDEBUG
  max_offset_ = max_offset;
#endif
  // Closely related to GetSstInternalUniqueId, but only need 128 bits and
  // need to include an offset within the file.
  // See also https://github.com/pdillinger/unique_id for background.
  uint64_t session_upper = 0;  // Assignment to appease clang-analyze
  uint64_t session_lower = 0;  // Assignment to appease clang-analyze
  {
    Status s = DecodeSessionId(db_session_id, &session_upper, &session_lower);
    if (!s.ok()) {
      // A reasonable fallback in case malformed
      Hash2x64(db_session_id.data(), db_session_id.size(), &session_upper,
               &session_lower);
    }
  }

  // Hash the session upper (~39 bits entropy) and DB id (120+ bits entropy)
  // for more global uniqueness entropy.
  // (It is possible that many DBs descended from one common DB id are copied
  // around and proliferate, in which case session id is critical, but it is
  // more common for different DBs to have different DB ids.)
  uint64_t db_hash = Hash64(db_id.data(), db_id.size(), session_upper);

  // This establishes the db+session id part of the cache key.
  //
  // Exactly preserve (in common cases; see modifiers below) session lower to
  // ensure that session ids generated during the same process lifetime are
  // guaranteed unique.
  //
  // We put this first for CommonPrefixSlice(), so that a small-ish set of
  // cache key prefixes to cover entries relevant to any DB.
  session_etc64_ = session_lower;
  // This provides extra entopy in case of different DB id or process
  // generating a session id, but is also partly/variably obscured by
  // file_number and offset (see below).
  offset_etc64_ = db_hash;

  // Into offset_etc64_ we are (eventually) going to pack & xor in an offset and
  // a file_number, but we might need the file_number to overflow into
  // session_etc64_. (There must only be one session_etc64_ value per
  // file, and preferably shared among many files.)
  //
  // Figure out how many bytes of file_number we are going to be able to
  // pack in with max_offset, though our encoding will only support packing
  // in up to 3 bytes of file_number. (16M file numbers is enough for a new
  // file number every second for half a year.)
  int file_number_bytes_in_offset_etc =
      (63 - FloorLog2(max_offset | 0x100000000U)) / 8;
  int file_number_bits_in_offset_etc = file_number_bytes_in_offset_etc * 8;

  // Assert two bits of metadata
  assert(file_number_bytes_in_offset_etc >= 0 &&
         file_number_bytes_in_offset_etc <= 3);
  // Assert we couldn't have used a larger allowed number of bytes (shift
  // would chop off bytes).
  assert(file_number_bytes_in_offset_etc == 3 ||
         (max_offset << (file_number_bits_in_offset_etc + 8) >>
          (file_number_bits_in_offset_etc + 8)) != max_offset);

  uint64_t mask = (uint64_t{1} << (file_number_bits_in_offset_etc)) - 1;
  // Pack into high bits of etc so that offset can go in low bits of etc
  // TODO: could be EndianSwapValue?
  uint64_t offset_etc_modifier = ReverseBits(file_number & mask);
  assert(offset_etc_modifier << file_number_bits_in_offset_etc == 0U);

  // Overflow and 3 - byte count (likely both zero) go into session_id part
  uint64_t session_etc_modifier =
      (file_number >> file_number_bits_in_offset_etc << 2) |
      static_cast<uint64_t>(3 - file_number_bytes_in_offset_etc);
  // Packed into high bits to minimize interference with session id counter.
  session_etc_modifier = ReverseBits(session_etc_modifier);

  // Assert session_id part is only modified in extreme cases
  assert(session_etc_modifier == 0 || file_number > /*3 bytes*/ 0xffffffU ||
         max_offset > /*5 bytes*/ 0xffffffffffU);

  // Xor in the modifiers
  session_etc64_ ^= session_etc_modifier;
  offset_etc64_ ^= offset_etc_modifier;

  // Although DBImpl guarantees (in recent versions) that session_lower is not
  // zero, that's not entirely sufficient to guarantee that session_etc64_ is
  // not zero (so that the 0 case can be used by CacheKey::CreateUnique*)
  if (session_etc64_ == 0U) {
    session_etc64_ = session_upper | 1U;
  }
  assert(session_etc64_ != 0);
}

}  // namespace MIZAR_NAMESPACE
