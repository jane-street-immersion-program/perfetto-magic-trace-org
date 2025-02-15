/*
 * Copyright (C) 2019 The Android Open Source Project
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

#ifndef SRC_TRACE_PROCESSOR_UTIL_DESCRIPTORS_H_
#define SRC_TRACE_PROCESSOR_UTIL_DESCRIPTORS_H_

#include <algorithm>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "perfetto/base/status.h"
#include "perfetto/ext/base/optional.h"
#include "protos/perfetto/common/descriptor.pbzero.h"

namespace protozero {
struct ConstBytes;
}

namespace perfetto {
namespace trace_processor {

class FieldDescriptor {
 public:
  FieldDescriptor(std::string name,
                  uint32_t number,
                  uint32_t type,
                  std::string raw_type_name,
                  bool is_repeated,
                  bool is_extension = false);

  const std::string& name() const { return name_; }
  uint32_t number() const { return number_; }
  uint32_t type() const { return type_; }
  const std::string& raw_type_name() const { return raw_type_name_; }
  const std::string& resolved_type_name() const { return resolved_type_name_; }
  bool is_repeated() const { return is_repeated_; }
  bool is_extension() const { return is_extension_; }

  void set_resolved_type_name(const std::string& resolved_type_name) {
    resolved_type_name_ = resolved_type_name;
  }

 private:
  std::string name_;
  uint32_t number_;
  uint32_t type_;
  std::string raw_type_name_;
  std::string resolved_type_name_;
  bool is_repeated_;
  bool is_extension_;
};

FieldDescriptor CreateFieldFromDecoder(
    const protos::pbzero::FieldDescriptorProto::Decoder& f_decoder,
    bool is_extension);

class ProtoDescriptor {
 public:
  enum class Type { kEnum = 0, kMessage = 1 };

  ProtoDescriptor(std::string file_name,
                  std::string package_name,
                  std::string full_name,
                  Type type,
                  base::Optional<uint32_t> parent_id);

  void AddField(FieldDescriptor descriptor) {
    PERFETTO_DCHECK(type_ == Type::kMessage);
    fields_.emplace(descriptor.number(), std::move(descriptor));
  }

  void AddEnumValue(int32_t integer_representation,
                    std::string string_representation) {
    PERFETTO_DCHECK(type_ == Type::kEnum);
    enum_values_by_name_[string_representation] = integer_representation;
    enum_names_by_value_[integer_representation] =
        std::move(string_representation);
  }

  const FieldDescriptor* FindFieldByName(const std::string& name) const {
    PERFETTO_DCHECK(type_ == Type::kMessage);
    auto it =
        std::find_if(fields_.begin(), fields_.end(),
                     [name](std::pair<int32_t, const FieldDescriptor&> p) {
                       return p.second.name() == name;
                     });
    if (it == fields_.end()) {
      return nullptr;
    }
    return &it->second;
  }

  const FieldDescriptor* FindFieldByTag(const uint32_t tag_number) const {
    PERFETTO_DCHECK(type_ == Type::kMessage);
    auto it = fields_.find(tag_number);
    if (it == fields_.end()) {
      return nullptr;
    }
    return &it->second;
  }

  base::Optional<std::string> FindEnumString(const int32_t value) const {
    PERFETTO_DCHECK(type_ == Type::kEnum);
    auto it = enum_names_by_value_.find(value);
    return it == enum_names_by_value_.end() ? base::nullopt
                                            : base::make_optional(it->second);
  }

  base::Optional<int32_t> FindEnumValue(const std::string& value) const {
    PERFETTO_DCHECK(type_ == Type::kEnum);
    auto it = enum_values_by_name_.find(value);
    return it == enum_values_by_name_.end() ? base::nullopt
                                            : base::make_optional(it->second);
  }

  const std::string& file_name() const { return file_name_; }

  const std::string& package_name() const { return package_name_; }

  const std::string& full_name() const { return full_name_; }

  Type type() const { return type_; }

  const std::unordered_map<uint32_t, FieldDescriptor>& fields() const {
    return fields_;
  }
  std::unordered_map<uint32_t, FieldDescriptor>* mutable_fields() {
    return &fields_;
  }

 private:
  std::string file_name_;  // File in which descriptor was originally defined.
  std::string package_name_;
  std::string full_name_;
  const Type type_;
  base::Optional<uint32_t> parent_id_;
  std::unordered_map<uint32_t, FieldDescriptor> fields_;
  std::unordered_map<int32_t, std::string> enum_names_by_value_;
  std::unordered_map<std::string, int32_t> enum_values_by_name_;
};

using ExtensionInfo = std::pair<std::string, protozero::ConstBytes>;

class DescriptorPool {
 public:
  // Adds Descriptors from file_descriptor_set_proto. Ignores any FileDescriptor
  // with name matching a prefix in |skip_prefixes|.
  base::Status AddFromFileDescriptorSet(
      const uint8_t* file_descriptor_set_proto,
      size_t size,
      const std::vector<std::string>& skip_prefixes = {},
      bool merge_existing_messages = false);

  base::Optional<uint32_t> FindDescriptorIdx(
      const std::string& full_name) const;

  std::vector<uint8_t> SerializeAsDescriptorSet();

  void AddProtoDescriptorForTesting(ProtoDescriptor descriptor) {
    AddProtoDescriptor(std::move(descriptor));
  }

  const std::vector<ProtoDescriptor>& descriptors() const {
    return descriptors_;
  }

 private:
  base::Status AddNestedProtoDescriptors(const std::string& file_name,
                                         const std::string& package_name,
                                         base::Optional<uint32_t> parent_idx,
                                         protozero::ConstBytes descriptor_proto,
                                         std::vector<ExtensionInfo>* extensions,
                                         bool merge_existing_messages);
  base::Status AddEnumProtoDescriptors(const std::string& file_name,
                                       const std::string& package_name,
                                       base::Optional<uint32_t> parent_idx,
                                       protozero::ConstBytes descriptor_proto,
                                       bool merge_existing_messages);

  base::Status AddExtensionField(const std::string& package_name,
                                 protozero::ConstBytes field_desc_proto);

  // Recursively searches for the given short type in all parent messages
  // and packages.
  base::Optional<uint32_t> ResolveShortType(const std::string& parent_path,
                                            const std::string& short_type);

  // Adds a new descriptor to the pool and returns its index. There must not be
  // already a descriptor with the same full_name in the pool.
  uint32_t AddProtoDescriptor(ProtoDescriptor descriptor);

  std::vector<ProtoDescriptor> descriptors_;
  // full_name -> index in the descriptors_ vector.
  std::unordered_map<std::string, uint32_t> full_name_to_descriptor_index_;
  std::set<std::string> processed_files_;
};

}  // namespace trace_processor
}  // namespace perfetto

#endif  // SRC_TRACE_PROCESSOR_UTIL_DESCRIPTORS_H_
