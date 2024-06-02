//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).
//
// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "mizar/comparator.h"

#include <stdint.h>

#include <algorithm>
#include <memory>
#include <mutex>

#include "port/port.h"
#include "mizar/convenience.h"
#include "mizar/slice.h"
#include "mizar/utilities/customizable_util.h"
#include "mizar/utilities/object_registry.h"

namespace MIZAR_NAMESPACE {

namespace {
class BytewiseComparatorImpl : public Comparator {
 public:
  BytewiseComparatorImpl() { }
  static const char* kClassName() { return "leveldb.BytewiseComparator"; }
  const char* Name() const override { return kClassName(); }

  int Compare(const Slice& a, const Slice& b) const override {
    return a.compare(b);
  }

  bool Equal(const Slice& a, const Slice& b) const override { return a == b; }

  void FindShortestSeparator(std::string* start,
                             const Slice& limit) const override {
    // Find length of common prefix
    size_t min_length = std::min(start->size(), limit.size());
    size_t diff_index = 0;
    while ((diff_index < min_length) &&
           ((*start)[diff_index] == limit[diff_index])) {
      diff_index++;
    }

    if (diff_index >= min_length) {
      // Do not shorten if one string is a prefix of the other
    } else {
      uint8_t start_byte = static_cast<uint8_t>((*start)[diff_index]);
      uint8_t limit_byte = static_cast<uint8_t>(limit[diff_index]);
      if (start_byte >= limit_byte) {
        // Cannot shorten since limit is smaller than start or start is
        // already the shortest possible.
        return;
      }
      assert(start_byte < limit_byte);

      if (diff_index < limit.size() - 1 || start_byte + 1 < limit_byte) {
        (*start)[diff_index]++;
        start->resize(diff_index + 1);
      } else {
        //     v
        // A A 1 A A A
        // A A 2
        //
        // Incrementing the current byte will make start bigger than limit, we
        // will skip this byte, and find the first non 0xFF byte in start and
        // increment it.
        diff_index++;

        while (diff_index < start->size()) {
          // Keep moving until we find the first non 0xFF byte to
          // increment it
          if (static_cast<uint8_t>((*start)[diff_index]) <
              static_cast<uint8_t>(0xff)) {
            (*start)[diff_index]++;
            start->resize(diff_index + 1);
            break;
          }
          diff_index++;
        }
      }
      assert(Compare(*start, limit) < 0);
    }
  }

  void FindShortSuccessor(std::string* key) const override {
    // Find first character that can be incremented
    size_t n = key->size();
    for (size_t i = 0; i < n; i++) {
      const uint8_t byte = (*key)[i];
      if (byte != static_cast<uint8_t>(0xff)) {
        (*key)[i] = byte + 1;
        key->resize(i+1);
        return;
      }
    }
    // *key is a run of 0xffs.  Leave it alone.
  }

  bool IsSameLengthImmediateSuccessor(const Slice& s,
                                      const Slice& t) const override {
    if (s.size() != t.size() || s.size() == 0) {
      return false;
    }
    size_t diff_ind = s.difference_offset(t);
    // same slice
    if (diff_ind >= s.size()) return false;
    uint8_t byte_s = static_cast<uint8_t>(s[diff_ind]);
    uint8_t byte_t = static_cast<uint8_t>(t[diff_ind]);
    // first different byte must be consecutive, and remaining bytes must be
    // 0xff for s and 0x00 for t
    if (byte_s != uint8_t{0xff} && byte_s + 1 == byte_t) {
      for (size_t i = diff_ind + 1; i < s.size(); ++i) {
        byte_s = static_cast<uint8_t>(s[i]);
        byte_t = static_cast<uint8_t>(t[i]);
        if (byte_s != uint8_t{0xff} || byte_t != uint8_t{0x00}) {
          return false;
        }
      }
      return true;
    } else {
      return false;
    }
  }

  bool CanKeysWithDifferentByteContentsBeEqual() const override {
    return false;
  }

  using Comparator::CompareWithoutTimestamp;
  int CompareWithoutTimestamp(const Slice& a, bool /*a_has_ts*/, const Slice& b,
                              bool /*b_has_ts*/) const override {
    return a.compare(b);
  }

  bool EqualWithoutTimestamp(const Slice& a, const Slice& b) const override {
    return a == b;
  }
};

class ReverseBytewiseComparatorImpl : public BytewiseComparatorImpl {
 public:
  ReverseBytewiseComparatorImpl() { }

  static const char* kClassName() {
    return "rocksdb.ReverseBytewiseComparator";
  }
  const char* Name() const override { return kClassName(); }

  int Compare(const Slice& a, const Slice& b) const override {
    return -a.compare(b);
  }

  void FindShortestSeparator(std::string* start,
                             const Slice& limit) const override {
    // Find length of common prefix
    size_t min_length = std::min(start->size(), limit.size());
    size_t diff_index = 0;
    while ((diff_index < min_length) &&
           ((*start)[diff_index] == limit[diff_index])) {
      diff_index++;
    }

    assert(diff_index <= min_length);
    if (diff_index == min_length) {
      // Do not shorten if one string is a prefix of the other
      //
      // We could handle cases like:
      //     V
      // A A 2 X Y
      // A A 2
      // in a similar way as BytewiseComparator::FindShortestSeparator().
      // We keep it simple by not implementing it. We can come back to it
      // later when needed.
    } else {
      uint8_t start_byte = static_cast<uint8_t>((*start)[diff_index]);
      uint8_t limit_byte = static_cast<uint8_t>(limit[diff_index]);
      if (start_byte > limit_byte && diff_index < start->size() - 1) {
        // Case like
        //     V
        // A A 3 A A
        // A A 1 B B
        //
        // or
        //     v
        // A A 2 A A
        // A A 1 B B
        // In this case "AA2" will be good.
#ifndef NDEBUG
        std::string old_start = *start;
#endif
        start->resize(diff_index + 1);
#ifndef NDEBUG
        assert(old_start >= *start);
#endif
        assert(Slice(*start).compare(limit) > 0);
      }
    }
  }

  void FindShortSuccessor(std::string* /*key*/) const override {
    // Don't do anything for simplicity.
  }

  bool CanKeysWithDifferentByteContentsBeEqual() const override {
    return false;
  }

  using Comparator::CompareWithoutTimestamp;
  int CompareWithoutTimestamp(const Slice& a, bool /*a_has_ts*/, const Slice& b,
                              bool /*b_has_ts*/) const override {
    return -a.compare(b);
  }
};
}// namespace

const Comparator* BytewiseComparator() {
  static BytewiseComparatorImpl bytewise;
  return &bytewise;
}

const Comparator* ReverseBytewiseComparator() {
  static ReverseBytewiseComparatorImpl rbytewise;
  return &rbytewise;
}

#ifndef MIZAR_LITE
static int RegisterBuiltinComparators(ObjectLibrary& library,
                                      const std::string& /*arg*/) {
  library.AddFactory<const Comparator>(
      BytewiseComparatorImpl::kClassName(),
      [](const std::string& /*uri*/,
         std::unique_ptr<const Comparator>* /*guard */,
         std::string* /* errmsg */) { return BytewiseComparator(); });
  library.AddFactory<const Comparator>(
      ReverseBytewiseComparatorImpl::kClassName(),
      [](const std::string& /*uri*/,
         std::unique_ptr<const Comparator>* /*guard */,
         std::string* /* errmsg */) { return ReverseBytewiseComparator(); });
  return 2;
}
#endif  // MIZAR_LITE

Status Comparator::CreateFromString(const ConfigOptions& config_options,
                                    const std::string& value,
                                    const Comparator** result) {
#ifndef MIZAR_LITE
  static std::once_flag once;
  std::call_once(once, [&]() {
    RegisterBuiltinComparators(*(ObjectLibrary::Default().get()), "");
  });
#endif  // MIZAR_LITE
  std::string id;
  std::unordered_map<std::string, std::string> opt_map;
  Status status = Customizable::GetOptionsMap(config_options, *result, value,
                                              &id, &opt_map);
  if (!status.ok()) {  // GetOptionsMap failed
    return status;
  }
  if (id == BytewiseComparatorImpl::kClassName()) {
    *result = BytewiseComparator();
  } else if (id == ReverseBytewiseComparatorImpl::kClassName()) {
    *result = ReverseBytewiseComparator();
  } else if (value.empty()) {
    // No Id and no options.  Clear the object
    *result = nullptr;
    return Status::OK();
  } else if (id.empty()) {  // We have no Id but have options.  Not good
    return Status::NotSupported("Cannot reset object ", id);
  } else {
#ifndef MIZAR_LITE
    status = config_options.registry->NewStaticObject(id, result);
#else
    status = Status::NotSupported("Cannot load object in LITE mode ", id);
#endif  // MIZAR_LITE
    if (!status.ok()) {
      if (config_options.ignore_unsupported_options &&
          status.IsNotSupported()) {
        return Status::OK();
      } else {
        return status;
      }
    } else if (!opt_map.empty()) {
      Comparator* comparator = const_cast<Comparator*>(*result);
      status = comparator->ConfigureFromMap(config_options, opt_map);
    }
  }
  return status;
}
}  // namespace MIZAR_NAMESPACE
