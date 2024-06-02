// Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).
//
// This file implements the "bridge" between Java and C++ for
// MIZAR_NAMESPACE::ColumnFamilyHandle.

#include <jni.h>
#include <stdio.h>
#include <stdlib.h>

#include "include/org_rocksdb_ColumnFamilyHandle.h"
#include "rocksjni/portal.h"

/*
 * Class:     org_rocksdb_ColumnFamilyHandle
 * Method:    getName
 * Signature: (J)[B
 */
jbyteArray Java_org_rocksdb_ColumnFamilyHandle_getName(JNIEnv* env,
                                                       jobject /*jobj*/,
                                                       jlong jhandle) {
  auto* cfh = reinterpret_cast<MIZAR_NAMESPACE::ColumnFamilyHandle*>(jhandle);
  std::string cf_name = cfh->GetName();
  return MIZAR_NAMESPACE::JniUtil::copyBytes(env, cf_name);
}

/*
 * Class:     org_rocksdb_ColumnFamilyHandle
 * Method:    getID
 * Signature: (J)I
 */
jint Java_org_rocksdb_ColumnFamilyHandle_getID(JNIEnv* /*env*/,
                                               jobject /*jobj*/,
                                               jlong jhandle) {
  auto* cfh = reinterpret_cast<MIZAR_NAMESPACE::ColumnFamilyHandle*>(jhandle);
  const int32_t id = cfh->GetID();
  return static_cast<jint>(id);
}

/*
 * Class:     org_rocksdb_ColumnFamilyHandle
 * Method:    getDescriptor
 * Signature: (J)Lorg/rocksdb/ColumnFamilyDescriptor;
 */
jobject Java_org_rocksdb_ColumnFamilyHandle_getDescriptor(JNIEnv* env,
                                                          jobject /*jobj*/,
                                                          jlong jhandle) {
  auto* cfh = reinterpret_cast<MIZAR_NAMESPACE::ColumnFamilyHandle*>(jhandle);
  MIZAR_NAMESPACE::ColumnFamilyDescriptor desc;
  MIZAR_NAMESPACE::Status s = cfh->GetDescriptor(&desc);
  if (s.ok()) {
    return MIZAR_NAMESPACE::ColumnFamilyDescriptorJni::construct(env, &desc);
  } else {
    MIZAR_NAMESPACE::RocksDBExceptionJni::ThrowNew(env, s);
    return nullptr;
  }
}

/*
 * Class:     org_rocksdb_ColumnFamilyHandle
 * Method:    disposeInternal
 * Signature: (J)V
 */
void Java_org_rocksdb_ColumnFamilyHandle_disposeInternal(JNIEnv* /*env*/,
                                                         jobject /*jobj*/,
                                                         jlong jhandle) {
  auto* cfh = reinterpret_cast<MIZAR_NAMESPACE::ColumnFamilyHandle*>(jhandle);
  assert(cfh != nullptr);
  delete cfh;
}
