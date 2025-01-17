/*
 * Copyright 2019 Google
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#import "Firestore/core/test/firebase/firestore/local/persistence_testing.h"

#include <utility>

#import "Firestore/Source/Local/FSTLocalSerializer.h"
#import "Firestore/Source/Remote/FSTSerializerBeta.h"

#include "Firestore/core/src/firebase/firestore/local/leveldb_persistence.h"
#include "Firestore/core/src/firebase/firestore/local/lru_garbage_collector.h"
#include "Firestore/core/src/firebase/firestore/local/memory_persistence.h"
#include "Firestore/core/src/firebase/firestore/local/proto_sizer.h"
#include "Firestore/core/src/firebase/firestore/model/database_id.h"
#include "Firestore/core/src/firebase/firestore/util/exception.h"
#include "Firestore/core/src/firebase/firestore/util/filesystem.h"
#include "Firestore/core/src/firebase/firestore/util/path.h"
#include "Firestore/core/src/firebase/firestore/util/status.h"
#include "Firestore/core/src/firebase/firestore/util/string_apple.h"

namespace firebase {
namespace firestore {
namespace local {
namespace {

using model::DatabaseId;
using util::Path;
using util::Status;

FSTLocalSerializer* MakeLocalSerializer() {
  auto remoteSerializer =
      [[FSTSerializerBeta alloc] initWithDatabaseID:DatabaseId("p", "d")];
  return [[FSTLocalSerializer alloc] initWithRemoteSerializer:remoteSerializer];
}

}  // namespace

Path LevelDbDir() {
  Path dir = util::TempDir().AppendUtf8("PersistenceTesting");

  // Delete the directory first to ensure isolation between runs.
  Status status = util::RecursivelyDelete(dir);
  if (!status.ok()) {
    util::ThrowIllegalState("Failed to clean up leveldb in dir %s: %s",
                            dir.ToUtf8String(), status.ToString());
  }

  return dir;
}

std::unique_ptr<LevelDbPersistence> LevelDbPersistenceForTesting(
    Path dir, LruParams lru_params) {
  FSTLocalSerializer* serializer = MakeLocalSerializer();

  auto created =
      LevelDbPersistence::Create(std::move(dir), serializer, lru_params);
  if (!created.ok()) {
    util::ThrowIllegalState("Failed to open leveldb in dir %s: %s",
                            dir.ToUtf8String(), created.status().ToString());
  }
  return std::move(created).ValueOrDie();
}

std::unique_ptr<LevelDbPersistence> LevelDbPersistenceForTesting(Path dir) {
  return LevelDbPersistenceForTesting(std::move(dir), LruParams::Default());
}

std::unique_ptr<LevelDbPersistence> LevelDbPersistenceForTesting(
    LruParams lru_params) {
  return LevelDbPersistenceForTesting(LevelDbDir(), lru_params);
}

std::unique_ptr<LevelDbPersistence> LevelDbPersistenceForTesting() {
  return LevelDbPersistenceForTesting(LevelDbDir());
}

std::unique_ptr<MemoryPersistence> MemoryPersistenceWithEagerGcForTesting() {
  return MemoryPersistence::WithEagerGarbageCollector();
}

std::unique_ptr<MemoryPersistence> MemoryPersistenceWithLruGcForTesting() {
  return MemoryPersistenceWithLruGcForTesting(LruParams::Default());
}

std::unique_ptr<MemoryPersistence> MemoryPersistenceWithLruGcForTesting(
    LruParams lru_params) {
  FSTLocalSerializer* serializer = MakeLocalSerializer();
  auto sizer = absl::make_unique<ProtoSizer>(serializer);
  return MemoryPersistence::WithLruGarbageCollector(lru_params,
                                                    std::move(sizer));
}

}  // namespace local
}  // namespace firestore
}  // namespace firebase
