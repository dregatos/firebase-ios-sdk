/*
 * Copyright 2018 Google
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

#ifndef FIRESTORE_CORE_SRC_FIREBASE_FIRESTORE_REMOTE_REMOTE_OBJC_BRIDGE_H_
#define FIRESTORE_CORE_SRC_FIREBASE_FIRESTORE_REMOTE_REMOTE_OBJC_BRIDGE_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "Firestore/Protos/nanopb/google/firestore/v1/firestore.nanopb.h"
#include "Firestore/core/src/firebase/firestore/core/database_info.h"
#include "Firestore/core/src/firebase/firestore/local/query_data.h"
#include "Firestore/core/src/firebase/firestore/model/types.h"
#include "Firestore/core/src/firebase/firestore/nanopb/byte_string.h"
#include "Firestore/core/src/firebase/firestore/nanopb/message.h"
#include "Firestore/core/src/firebase/firestore/nanopb/reader.h"
#include "Firestore/core/src/firebase/firestore/remote/serializer.h"
#include "Firestore/core/src/firebase/firestore/util/status_fwd.h"
#include "grpcpp/support/byte_buffer.h"

namespace firebase {
namespace firestore {

namespace model {
class DocumentKey;
class MaybeDocument;
class SnapshotVersion;
}  // namespace model

namespace remote {

class WatchChange;

// TODO(varconst): remove this file?
//
// The original purpose of this file was to cleanly encapsulate the remaining
// Objective-C dependencies of `remote/` folder. These dependencies no longer
// exist (modulo pretty-printing), and this file makes C++ diverge from other
// platforms.
//
// On the other hand, stream classes are large, and having one easily
// separatable aspect of their implementation (serialization) refactored out is
// arguably a good thing. If this file were to stay (in some form, certainly
// under a different name), other platforms would have to follow suit.
//
// Note: return value optimization should make returning Nanopb messages from
// functions cheap (even though they may be large types that are relatively
// expensive to copy).

class WatchStreamSerializer {
 public:
  explicit WatchStreamSerializer(Serializer serializer);

  nanopb::Message<google_firestore_v1_ListenRequest> EncodeWatchRequest(
      const local::QueryData& query) const;
  nanopb::Message<google_firestore_v1_ListenRequest> EncodeUnwatchRequest(
      model::TargetId target_id) const;

  nanopb::MaybeMessage<google_firestore_v1_ListenResponse> ParseResponse(
      const grpc::ByteBuffer& buffer) const;
  std::unique_ptr<WatchChange> DecodeWatchChange(
      nanopb::Reader* reader,
      const google_firestore_v1_ListenResponse& response) const;
  model::SnapshotVersion DecodeSnapshotVersion(
      nanopb::Reader* reader,
      const google_firestore_v1_ListenResponse& response) const;

  /** Creates a pretty-printed description of the proto for debugging. */
  static std::string Describe(
      const nanopb::Message<google_firestore_v1_ListenRequest>& request);
  static std::string Describe(
      const nanopb::Message<google_firestore_v1_ListenResponse>& response);

 private:
  Serializer serializer_;
};

class WriteStreamSerializer {
 public:
  explicit WriteStreamSerializer(Serializer serializer);

  nanopb::Message<google_firestore_v1_WriteRequest> EncodeHandshake() const;
  nanopb::Message<google_firestore_v1_WriteRequest> EncodeWriteMutationsRequest(
      const std::vector<model::Mutation>& mutations,
      const nanopb::ByteString& last_stream_token) const;
  nanopb::Message<google_firestore_v1_WriteRequest> EncodeEmptyMutationsList(
      const nanopb::ByteString& last_stream_token) const {
    return EncodeWriteMutationsRequest({}, last_stream_token);
  }

  nanopb::MaybeMessage<google_firestore_v1_WriteResponse> ParseResponse(
      const grpc::ByteBuffer& buffer) const;
  model::SnapshotVersion DecodeCommitVersion(
      nanopb::Reader* reader,
      const google_firestore_v1_WriteResponse& proto) const;
  std::vector<model::MutationResult> DecodeMutationResults(
      nanopb::Reader* reader,
      const google_firestore_v1_WriteResponse& proto) const;

  /** Creates a pretty-printed description of the proto for debugging. */
  static std::string Describe(
      const nanopb::Message<google_firestore_v1_WriteRequest>& request);
  static std::string Describe(
      const nanopb::Message<google_firestore_v1_WriteResponse>& response);

 private:
  Serializer serializer_;
};

class DatastoreSerializer {
 public:
  explicit DatastoreSerializer(const core::DatabaseInfo& database_info);

  nanopb::Message<google_firestore_v1_CommitRequest> EncodeCommitRequest(
      const std::vector<model::Mutation>& mutations) const;

  nanopb::Message<google_firestore_v1_BatchGetDocumentsRequest>
  EncodeLookupRequest(const std::vector<model::DocumentKey>& keys) const;

  /**
   * Merges results of the streaming read together. The array is sorted by the
   * document key.
   */
  util::StatusOr<std::vector<model::MaybeDocument>> MergeLookupResponses(
      const std::vector<grpc::ByteBuffer>& responses) const;

  const Serializer& serializer() const {
    return serializer_;
  }

 private:
  Serializer serializer_;
};

}  // namespace remote
}  // namespace firestore
}  // namespace firebase

#endif  // FIRESTORE_CORE_SRC_FIREBASE_FIRESTORE_REMOTE_REMOTE_OBJC_BRIDGE_H_
