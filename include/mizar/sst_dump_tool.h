// Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).
#ifndef MIZAR_LITE
#pragma once

#include "mizar/options.h"

namespace MIZAR_NAMESPACE {

class SSTDumpTool {
 public:
  int Run(int argc, char const* const* argv, Options options = Options());
};

}  // namespace MIZAR_NAMESPACE

#endif  // MIZAR_LITE
