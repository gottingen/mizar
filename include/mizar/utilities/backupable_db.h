//  Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

// This is a DEPRECATED header for API backward compatibility. Please
// use backup_engine.h.

#pragma once
#ifndef MIZAR_LITE

// A legacy unnecessary include
#include <cinttypes>

#include "mizar/utilities/backup_engine.h"

// A legacy unnecessary include
#include "mizar/utilities/stackable_db.h"

namespace MIZAR_NAMESPACE {

using BackupableDBOptions = BackupEngineOptions;

}  // namespace MIZAR_NAMESPACE

#endif  // MIZAR_LITE
