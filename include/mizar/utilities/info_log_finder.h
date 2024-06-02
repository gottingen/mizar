// Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#pragma once

#include <string>
#include <vector>

#include "mizar/db.h"
#include "mizar/options.h"

namespace MIZAR_NAMESPACE {

// This function can be used to list the Information logs,
// given the db pointer.
Status GetInfoLogList(DB* db, std::vector<std::string>* info_log_list);
}  // namespace MIZAR_NAMESPACE
