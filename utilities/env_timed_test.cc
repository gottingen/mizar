// Copyright (c) 2017-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#ifndef MIZAR_LITE

#include "mizar/env.h"
#include "mizar/perf_context.h"
#include "test_util/testharness.h"

namespace MIZAR_NAMESPACE {

class TimedEnvTest : public testing::Test {
};

TEST_F(TimedEnvTest, BasicTest) {
  SetPerfLevel(PerfLevel::kEnableTime);
  ASSERT_EQ(0, get_perf_context()->env_new_writable_file_nanos);

  std::unique_ptr<Env> mem_env(NewMemEnv(Env::Default()));
  std::unique_ptr<Env> timed_env(NewTimedEnv(mem_env.get()));
  std::unique_ptr<WritableFile> writable_file;
  ASSERT_OK(timed_env->NewWritableFile("f", &writable_file, EnvOptions()));

  ASSERT_GT(get_perf_context()->env_new_writable_file_nanos, 0);
}

}  // namespace MIZAR_NAMESPACE

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

#else  // MIZAR_LITE
#include <stdio.h>

int main(int /*argc*/, char** /*argv*/) {
  fprintf(stderr, "SKIPPED as TimedEnv is not supported in MIZAR_LITE\n");
  return 0;
}

#endif  // MIZAR_LITE
