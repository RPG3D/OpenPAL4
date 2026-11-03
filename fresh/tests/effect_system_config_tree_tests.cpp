#include "pal4/effect_system_config_tree.h"
#include "pal4/utilities_text_record_copy.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IEffectSystemConfigTreeRuntime {
    std::uint32_t header = 0x3000u;
    std::uint32_t candidate = 0x3000u;
    std::int32_t comparison = 0;
    std::uint8_t insertion_result = 1u;
    std::vector<std::string> events;

    std::uint32_t ReadWord(std::uint32_t address) override {
        events.push_back("head:" + std::to_string(address));
        return header;
    }
    std::uint32_t FindLowerBound(std::uint32_t tree,
                                 std::uint32_t key) override {
        events.push_back("find:" + std::to_string(tree) + ":" +
                         std::to_string(key));
        return candidate;
    }
    std::int32_t CompareText(std::uint32_t left,
                             std::uint32_t right) override {
        events.push_back("compare:" + std::to_string(left) + ":" +
                         std::to_string(right));
        return comparison;
    }
    std::uint32_t CopyKey(std::uint32_t key) override {
        events.push_back("copy-key:" + std::to_string(key));
        return 0x5000u;
    }
    std::uint32_t CopyConfig(std::uint32_t config) override {
        events.push_back("copy-config:" + std::to_string(config));
        return 0x6000u;
    }
    void CopyNameField(std::uint32_t copy,
                       std::uint32_t source) override {
        events.push_back("copy-name:" + std::to_string(copy) + ":" +
                         std::to_string(source));
    }
    void CopyTailField(std::uint32_t copy,
                       std::uint32_t source) override {
        events.push_back("copy-tail:" + std::to_string(copy) + ":" +
                         std::to_string(source));
    }
    std::uint8_t Insert(std::uint32_t tree, std::uint32_t key_copy,
                        std::uint32_t config_copy) override {
        events.push_back("insert:" + std::to_string(tree) + ":" +
                         std::to_string(key_copy) + ":" +
                         std::to_string(config_copy));
        return insertion_result;
    }
    void DestroyConfigCopy(std::uint32_t copy) override {
        events.push_back("destroy-config:" + std::to_string(copy));
    }
    void ReleaseKeyCopy(std::uint32_t copy) override {
        events.push_back("release-key:" + std::to_string(copy));
    }
};

struct CleanupRuntime final : pal4::IEffectSystemConfigCleanupRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;
    void DestroyConfig(std::uint32_t address) override {
        events.push_back("destroy:" + std::to_string(address));
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        events.push_back("read-word:" + std::to_string(address));
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        events.push_back("read-byte:" + std::to_string(address));
        return bytes.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        events.push_back("write-word:" + std::to_string(address));
        words[address] = value;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        events.push_back("write-byte:" + std::to_string(address));
        bytes[address] = value;
    }
    std::uint32_t Free(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
        return 0x9000u;
    }
};

struct LookupRuntime final : pal4::IEffectSystemConfigLookupRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::string> node_names;
    std::vector<std::string> events;
    std::int32_t comparison = 0;
    std::uint32_t ReadWord(std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    std::uint32_t SharedNil() const override { return 0x9000u; }
    std::int32_t CompareText(std::string_view name,
                             std::uint32_t node_text) override {
        events.push_back("compare:" + std::string(name) + ":" +
                         std::to_string(node_text));
        return comparison;
    }
    std::int32_t CompareNodeToName(std::uint32_t node_text,
                                   std::string_view name) override {
        const auto& node_name = node_names.at(node_text);
        return node_name < name ? -1 : node_name > name ? 1 : 0;
    }
};

struct BufferRuntime final : pal4::IEffectSystemRecordBufferRuntime {
    std::vector<std::string> events;
    std::uint32_t Allocate(std::uint32_t bytes) override {
        events.push_back("allocate:" + std::to_string(bytes));
        return 0x7000u;
    }
    void CopyRecord(std::uint32_t destination,
                    std::uint32_t source) override {
        events.push_back("copy:" + std::to_string(destination) + ":" +
                         std::to_string(source));
    }
    void ReleaseText(std::uint32_t record) override {
        events.push_back("text:" + std::to_string(record));
    }
    void CleanupProperty(std::uint32_t record) override {
        events.push_back("property:" + std::to_string(record));
    }
    void Free(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
    }
};

struct RecordCopyRuntime final : pal4::IUtilitiesTextRecordCopyRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::uint32_t next_data = 0x6001u;
    std::uint32_t allocations = 0u;
    std::uint32_t detach_calls = 0u;
    std::uint32_t shares = 0u;
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    void Copy(std::uint32_t destination, std::uint32_t source,
              std::uint32_t length) override {
        for (auto i = 0u; i < length; ++i)
            bytes[destination + i] = bytes[source + i];
    }
    void ThrowRange() override { throw std::out_of_range("text range"); }
    void ThrowLength() override { throw std::length_error("text length"); }
    void DetachAndReset(pal4::StringBufferState&) override {
        ++detach_calls;
    }
    bool AllocateText(pal4::StringBufferState& state,
                      std::uint32_t length,
                      std::uint8_t mode) override {
        Check(mode == 1u, "record copy requests owned text mode");
        if (length == 0u) return false;
        state.data = next_data;
        state.capacity = length;
        bytes[next_data - 1u] = 0u;
        next_data += 0x100u;
        ++allocations;
        return true;
    }
    void SetTextSize(pal4::StringBufferState& state,
                     std::uint32_t length) override {
        state.length = length;
        bytes[state.data + length] = 0u;
    }
    void SetErrorFlag(pal4::StringBufferState&,
                      std::uint8_t flag) override {
        Check(flag == 1u, "shared text sets error flag one");
        ++shares;
    }
    void Reallocate(pal4::StringBufferState& state,
                    std::uint32_t length) override {
        state.data = next_data;
        state.capacity = length;
        next_data += 0x100u;
    }
    void Free(std::uint32_t) override {}

    void Put(std::uint32_t record, std::uint32_t key,
             std::uint32_t data, std::string_view text,
             std::uint8_t references, std::uint32_t tail) {
        words[record] = key;
        bytes[record + 4u] = 0x5au;
        words[record + 8u] = data;
        words[record + 12u] = static_cast<std::uint32_t>(text.size());
        words[record + 16u] = static_cast<std::uint32_t>(text.size() + 4u);
        words[record + 20u] = tail;
        if (data != 0u) {
            bytes[data - 1u] = references;
            for (std::size_t i = 0; i < text.size(); ++i)
                bytes[data + static_cast<std::uint32_t>(i)] =
                    static_cast<std::uint8_t>(text[i]);
            bytes[data + static_cast<std::uint32_t>(text.size())] = 0u;
        }
    }
};

struct CompositeRuntime final : pal4::IEffectSystemCompositeCopyRuntime {
    RecordCopyRuntime records;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::string> text_fields;
    std::vector<std::string> events;
    std::uint32_t next = 0xa000u;
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        events.push_back("byte:" + std::to_string(address));
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        events.push_back("word:" + std::to_string(address));
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        events.push_back("allocate:" + std::to_string(size));
        const auto address = next;
        next += 0x100u;
        return address;
    }
    void AssignText(std::uint32_t destination,
                    std::uint32_t source) override {
        text_fields[destination] = text_fields.at(source);
        events.push_back("text:" + std::to_string(destination));
    }
    void SetErrorFlag(std::uint32_t field,
                      std::uint8_t flag) override {
        Check(flag == 0u, "composite middle text reset flag");
        events.push_back("flag:" + std::to_string(field));
    }
    void CopyRecord24(std::uint32_t destination,
                      std::uint32_t source) override {
        events.push_back("record:" + std::to_string(destination) + ":" +
                         std::to_string(source));
        pal4::EffectSystemRecordBuffer::CopyRecord24(
            destination, source, records);
    }
};

struct NodeRuntime final : pal4::IEffectSystemConfigNodeRuntime,
                           pal4::IEffectSystemConfigCleanupRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::string> payloads;
    std::uint32_t next = 0xa000u;
    std::uint32_t allocations = 0u;
    std::uint32_t copies = 0u;
    std::vector<std::uint32_t> freed_nodes;
    std::vector<std::uint32_t> destroyed_configs;

    explicit NodeRuntime(std::uint8_t mode) {
        words[0x1004u] = 0x2000u;
        words[0x100cu] = 0u;
        bytes[0x1008u] = mode;
        words[0x2000u] = 0x2000u;
        words[0x2004u] = 0x9000u;
        words[0x2008u] = 0x2000u;
        words[0x9000u] = 0x9000u;
        words[0x9004u] = 0x9000u;
        words[0x9008u] = 0x9000u;
        words[0x904cu] = 1u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    std::uint32_t SharedNil() const override { return 0x9000u; }
    static std::int32_t Compare(std::string_view left,
                                 std::string_view right) {
        return left < right ? -1 : left > right ? 1 : 0;
    }
    std::int32_t CompareSearch(std::uint32_t key,
                               std::uint32_t node_payload) override {
        return Compare(payloads.at(key), payloads.at(node_payload));
    }
    std::int32_t CompareText(std::uint32_t left,
                             std::uint32_t right) override {
        return Compare(payloads.at(left), payloads.at(right));
    }
    std::uint32_t AllocateNode(std::uint32_t bytes_count) override {
        Check(bytes_count == 80u, "config tree allocates 80-byte nodes");
        const auto address = next;
        next += 0x100u;
        ++allocations;
        return address;
    }
    void CopyPayload(std::uint32_t destination,
                     std::uint32_t source) override {
        payloads[destination] = payloads.at(source);
        words[destination + 4u] = 0u;
        words[destination + 8u] = 0u;
        words[destination + 12u] = 0u;
        ++copies;
    }
    void FreeNode(std::uint32_t node) override {
        freed_nodes.push_back(node);
    }
    void DestroyConfig(std::uint32_t address) override {
        destroyed_configs.push_back(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t Free(std::uint32_t) override { return 0u; }
    std::uint32_t FindNode(std::string_view key) const {
        for (auto node = 0xa000u; node < next; node += 0x100u) {
            if (payloads.at(node + 12u) == key &&
                std::find(freed_nodes.begin(), freed_nodes.end(), node) ==
                    freed_nodes.end())
                return node;
        }
        return 0u;
    }

    void CheckInvariants(std::uint32_t expected_count,
                         bool allow_duplicates) const {
        const auto root = words.at(0x2004u);
        Check(words.at(0x100cu) == expected_count,
              "tree count follows successful insertions");
        if (expected_count == 0u) {
            Check(root == 0x9000u && words.at(0x2000u) == 0x2000u &&
                      words.at(0x2008u) == 0x2000u,
                  "empty tree head is self linked");
            return;
        }
        Check(words.at(root + 4u) == 0x2000u &&
                  words.at(root + 76u) == 1u,
              "root has header parent and black color");
        std::vector<std::string> ordered;
        std::set<std::uint32_t> visited;
        std::function<int(std::uint32_t, std::uint32_t)> visit =
            [&](std::uint32_t node, std::uint32_t parent) -> int {
            if (node == 0x9000u) return 1;
            Check(visited.insert(node).second, "tree has no repeated nodes");
            Check(words.at(node + 4u) == parent,
                  "child parent pointer is reciprocal");
            const auto left = words.at(node);
            const auto right = words.at(node + 8u);
            if (words.at(node + 76u) == 0u)
                Check(words.at(left + 76u) == 1u &&
                          words.at(right + 76u) == 1u,
                      "red node has black children");
            const auto left_height = visit(left, node);
            ordered.push_back(payloads.at(node + 12u));
            const auto right_height = visit(right, node);
            Check(left_height == right_height,
                  "all paths have equal black height");
            return left_height + (words.at(node + 76u) == 1u ? 1 : 0);
        };
        (void)visit(root, 0x2000u);
        Check(ordered.size() == expected_count,
              "traversal visits every allocated node");
        for (std::size_t i = 1u; i < ordered.size(); ++i)
            Check(allow_duplicates ? ordered[i - 1u] <= ordered[i]
                                   : ordered[i - 1u] < ordered[i],
                  "tree is in key order");
        Check(payloads.at(words.at(0x2000u) + 12u) == ordered.front() &&
                  payloads.at(words.at(0x2008u) + 12u) == ordered.back(),
              "head tracks leftmost and rightmost nodes");
    }
};

}  // namespace

int main() {
    Runtime runtime;
    constexpr std::uint32_t owner = 0x1000u;
    constexpr std::uint32_t config = 0x2000u;

    Check(pal4::EffectSystemConfigTree::InsertIfAbsent(
              owner, config, runtime) == 1u &&
              runtime.events == std::vector<std::string>{
                  "head:4116", "find:4112:8208", "copy-key:8208",
                  "copy-config:8192", "copy-name:24576:8208",
                  "copy-tail:24576:8224", "insert:4112:20480:24576",
                  "destroy-config:24576", "release-key:20480"},
          "empty tree copies, inserts and releases in native order");

    runtime.events.clear();
    runtime.candidate = 0x4000u;
    runtime.comparison = 0;
    Check(pal4::EffectSystemConfigTree::InsertIfAbsent(
              owner, config, runtime) == 0u &&
              runtime.events == std::vector<std::string>{
                  "head:4116", "find:4112:8208", "compare:8208:16396"},
          "equal candidate blocks duplicate without copying");

    runtime.events.clear();
    runtime.comparison = 1;
    Check(pal4::EffectSystemConfigTree::InsertIfAbsent(
              owner, config, runtime) == 0u &&
              runtime.events.size() == 3u,
          "candidate ordered before input blocks insertion");

    runtime.events.clear();
    runtime.comparison = -1;
    runtime.insertion_result = 0u;
    Check(pal4::EffectSystemConfigTree::InsertIfAbsent(
              owner, config, runtime) == 0u &&
              runtime.events.size() == 10u &&
              runtime.events.back() == "release-key:20480",
          "later insertion rejection still releases both copies");

    CleanupRuntime cleanup;
    cleanup.words[0x1004u] = 0u;
    Check(pal4::EffectSystemConfigTree::CleanupValue(
              owner, cleanup) == 0u &&
              cleanup.events == std::vector<std::string>{
                  "destroy:4112", "read-word:4100", "write-word:4100",
                  "write-word:4104", "write-word:4108"},
          "cleanup without name destroys nested config and clears fields");

    cleanup.events.clear();
    cleanup.words[0x1004u] = 0x5001u;
    cleanup.bytes[0x5000u] = 3u;
    Check(pal4::EffectSystemConfigTree::CleanupValue(
              owner, cleanup) == 0x5000u &&
              cleanup.bytes[0x5000u] == 2u &&
              cleanup.words[0x1004u] == 0u &&
              cleanup.events == std::vector<std::string>{
                  "destroy:4112", "read-word:4100", "read-byte:20480",
                  "write-byte:20480", "write-word:4100",
                  "write-word:4104", "write-word:4108"},
          "shared name decrements reference count without freeing");

    for (const auto count : {0u, 255u}) {
        cleanup.events.clear();
        cleanup.words[0x1004u] = 0x5001u;
        cleanup.bytes[0x5000u] = static_cast<std::uint8_t>(count);
        Check(pal4::EffectSystemConfigTree::CleanupValue(
                  owner, cleanup) == 0x9000u &&
                  cleanup.events == std::vector<std::string>{
                      "destroy:4112", "read-word:4100", "read-byte:20480",
                      "free:20480", "write-word:4100",
                      "write-word:4104", "write-word:4108"},
              "unshared or saturated name buffer is freed");
    }

    cleanup.events.clear();
    cleanup.words[0x1004u] = 0u;
    Check(pal4::EffectSystemConfigTree::DeleteValue(
              owner, 0u, cleanup) == owner &&
              cleanup.events == std::vector<std::string>{
                  "destroy:4112", "read-word:4100", "write-word:4100",
                  "write-word:4104", "write-word:4108"},
          "nondeleting destructor cleans nested value and returns object");
    cleanup.events.clear();
    Check(pal4::EffectSystemConfigTree::DeleteValue(
              owner, 3u, cleanup) == owner &&
              cleanup.events.back() == "free:4096" &&
              cleanup.events.front() == "destroy:4112" &&
              cleanup.events.size() == 6u,
          "deleting destructor frees object after value cleanup");

    cleanup.events.clear();
    cleanup.words[0x1008u] = 0u;
    Check(pal4::EffectSystemConfigTree::CleanupTextField(
              owner, cleanup) == 0u &&
              cleanup.events == std::vector<std::string>{
                  "read-word:4104", "write-word:4104", "write-word:4108",
                  "write-word:4112"},
          "empty text field clears its three words");
    cleanup.events.clear();
    cleanup.words[0x1008u] = 0x5001u;
    cleanup.bytes[0x5000u] = 3u;
    Check(pal4::EffectSystemConfigTree::CleanupTextField(
              owner, cleanup) == 0x5000u &&
              cleanup.bytes[0x5000u] == 2u &&
              cleanup.words[0x1008u] == 0u &&
              cleanup.events == std::vector<std::string>{
                  "read-word:4104", "read-byte:20480", "write-byte:20480",
                  "write-word:4104", "write-word:4108", "write-word:4112"},
          "shared text field decrements its buffer reference");
    for (const auto count : {0u, 255u}) {
        cleanup.events.clear();
        cleanup.words[0x1008u] = 0x5001u;
        cleanup.bytes[0x5000u] = static_cast<std::uint8_t>(count);
        Check(pal4::EffectSystemConfigTree::CleanupTextField(
                  owner, cleanup) == 0x9000u &&
                  cleanup.events == std::vector<std::string>{
                      "read-word:4104", "read-byte:20480", "free:20480",
                      "write-word:4104", "write-word:4108", "write-word:4112"},
              "unshared or saturated text buffer is freed");
    }

    LookupRuntime lookup;
    lookup.words[0x1014u] = 0x3000u;
    lookup.words[0x101cu] = 0u;
    lookup.words[0x3004u] = 0x9000u;
    Check(pal4::EffectSystemConfigTree::FindProperty(
              owner, "absent", lookup) == 0u &&
              lookup.events == std::vector<std::string>{
                  "read:4116", "read:4116", "read:12292"},
          "missing property does not compare header text");
    lookup.events.clear();
    lookup.words[0x3004u] = 0x4000u;
    lookup.words[0x4000u] = 0x9000u;
    lookup.words[0x4008u] = 0x9000u;
    lookup.node_names[0x400cu] = "exact";
    lookup.comparison = -1;
    Check(pal4::EffectSystemConfigTree::FindProperty(
              owner, "before", lookup) == 0u &&
              lookup.events.back() == "compare:before:16396",
          "query before lower-bound candidate is absent");
    lookup.events.clear();
    lookup.comparison = 0;
    Check(pal4::EffectSystemConfigTree::FindProperty(
              owner, "exact", lookup) == 0x401cu &&
              lookup.events.back() == "compare:exact:16396",
          "exact property returns node payload at plus 28");

    std::uint32_t cursor = 5u;
    Check(pal4::EffectSystemConfigTree::FirstProperty(
              owner, cursor, lookup) == 0u && cursor == 0u,
          "empty property tree resets caller cursor");
    lookup.words[0x101cu] = 3u;
    lookup.words[0x3000u] = 0x4000u;
    lookup.words[0x4000u] = 0x9000u;
    lookup.words[0x4004u] = 0x5000u;
    lookup.words[0x4008u] = 0x9000u;
    lookup.words[0x5000u] = 0x4000u;
    lookup.words[0x5004u] = 0x3000u;
    lookup.words[0x5008u] = 0x6000u;
    lookup.words[0x6000u] = 0x9000u;
    lookup.words[0x6004u] = 0x5000u;
    lookup.words[0x6008u] = 0x9000u;
    Check(pal4::EffectSystemConfigTree::FirstProperty(
              owner, cursor, lookup) == 0x401cu && cursor == 0u &&
              pal4::EffectSystemConfigTree::NextProperty(
                  owner, cursor, lookup) == 0x501cu && cursor == 1u &&
              pal4::EffectSystemConfigTree::NextProperty(
                  owner, cursor, lookup) == 0x601cu && cursor == 2u &&
              pal4::EffectSystemConfigTree::NextProperty(
                  owner, cursor, lookup) == 0u && cursor == 3u,
          "property cursor follows leftmost, parent and right-child order");

    lookup.words[0x101cu] = 4u;
    lookup.words[0x5008u] = 0x7000u;
    lookup.words[0x7000u] = 0x6000u;
    lookup.words[0x7004u] = 0x5000u;
    lookup.words[0x7008u] = 0x9000u;
    lookup.words[0x6004u] = 0x7000u;
    Check(pal4::EffectSystemConfigTree::NextProperty(
              owner, cursor = 1u, lookup) == 0x601cu && cursor == 2u,
          "successor takes leftmost node of the right subtree");

    lookup.words[0x101cu] = 5u;
    lookup.words[0x5008u] = 0x6000u;
    lookup.words[0x6004u] = 0x5000u;
    lookup.words[0x6008u] = 0x9000u;
    lookup.words[0x5004u] = 0x7000u;
    lookup.words[0x7000u] = 0x5000u;
    lookup.words[0x7004u] = 0x3000u;
    lookup.words[0x7008u] = 0x8000u;
    lookup.words[0x8000u] = 0x9000u;
    lookup.words[0x8004u] = 0x7000u;
    lookup.words[0x8008u] = 0x9000u;
    Check(pal4::EffectSystemConfigTree::NextProperty(
              owner, cursor = 2u, lookup) == 0x701cu && cursor == 3u,
          "successor climbs past a right child to its ancestor");

    BufferRuntime buffer_runtime;
    pal4::EffectSystemRecordBufferState source;
    source.flag = 7u;
    source.begin = 0x4000u;
    source.end = 0x4030u;
    source.capacity = 0x4060u;
    pal4::EffectSystemRecordBufferState copy;
    Check(&pal4::EffectSystemRecordBuffer::Copy(
              copy, source, buffer_runtime) == &copy &&
              copy.flag == 7u && copy.begin == 0x7000u &&
              copy.end == 0x7030u && copy.capacity == 0x7030u &&
              buffer_runtime.events == std::vector<std::string>{
                  "allocate:48", "copy:28672:16384",
                  "copy:28696:16408"},
          "record copy allocates exact size and copies two 24-byte slots");
    buffer_runtime.events.clear();
    Check(pal4::EffectSystemRecordBuffer::CleanupText(
              copy, buffer_runtime) == 0 &&
              copy.flag == 7u && copy.begin == 0u && copy.end == 0u &&
              copy.capacity == 0u &&
              buffer_runtime.events == std::vector<std::string>{
                  "text:28672", "text:28696", "free:28672"},
          "text cleanup releases each slot then the allocation");
    buffer_runtime.events.clear();
    Check(pal4::EffectSystemRecordBuffer::CleanupProperties(
              source, buffer_runtime) == 0 &&
              source.begin == 0u && source.end == 0u &&
              source.capacity == 0u &&
              buffer_runtime.events == std::vector<std::string>{
                  "property:16384", "property:16408", "free:16384"},
          "property cleanup uses its distinct element destructor");
    buffer_runtime.events.clear();
    pal4::EffectSystemRecordBufferState empty_source;
    Check(&pal4::EffectSystemRecordBuffer::Copy(
              copy, empty_source, buffer_runtime) == &copy &&
              copy.begin == 0x7000u && copy.end == 0x7000u &&
              buffer_runtime.events ==
                  std::vector<std::string>{"allocate:0"},
          "empty copy requests a zero-byte native allocation");

    RecordCopyRuntime record_copy;
    record_copy.Put(0x2000u, 17u, 0x3001u, "abc", 3u, 0x77u);
    pal4::EffectSystemRecordBuffer::CopyRecord24(
        0x1000u, 0x2000u, record_copy);
    Check(record_copy.words[0x1000u] == 17u &&
              record_copy.bytes[0x1004u] == 0x5au &&
              record_copy.words[0x1008u] == 0x3001u &&
              record_copy.words[0x100cu] == 3u &&
              record_copy.words[0x1010u] == 7u &&
              record_copy.words[0x1014u] == 0x77u &&
              record_copy.bytes[0x3000u] == 4u &&
              record_copy.shares == 1u && record_copy.allocations == 0u,
          "24-byte copy shares eligible text and copies the tail word");
    record_copy.Put(0x2100u, 29u, 0x3101u, "xy", 0xfeu, 0x99u);
    pal4::EffectSystemRecordBuffer::CopyRecord24(
        0x1100u, 0x2100u, record_copy);
    Check(record_copy.words[0x1108u] == 0x6001u &&
              record_copy.words[0x110cu] == 2u &&
              record_copy.words[0x1114u] == 0x99u &&
              record_copy.bytes[0x6001u] == 'x' &&
              record_copy.bytes[0x6002u] == 'y' &&
              record_copy.bytes[0x6003u] == 0u &&
              record_copy.bytes[0x3100u] == 0xfeu &&
              record_copy.allocations == 1u,
          "saturated text is deep-copied and terminated");
    record_copy.Put(0x2200u, 37u, 0u, "", 0u, 0xaau);
    pal4::EffectSystemRecordBuffer::CopyRecord24(
        0x1200u, 0x2200u, record_copy);
    Check(record_copy.words[0x1208u] == 0u &&
              record_copy.words[0x120cu] == 0u &&
              record_copy.words[0x1214u] == 0xaau,
          "empty text still copies the trailing word");
    record_copy.Put(0x2300u, 41u, 0u, "", 0u, 0xbbu);
    pal4::EffectSystemRecordBuffer::CopyRecord24(
        0x2300u, 0x2300u, record_copy);
    Check(record_copy.detach_calls == 2u &&
              record_copy.words[0x2314u] == 0xbbu,
          "self-copy follows the native detach path");
    const auto old_allocations = record_copy.allocations;
    pal4::EffectSystemRecordBuffer::CopyRecord24(
        0u, 0x2000u, record_copy);
    Check(record_copy.allocations == old_allocations,
          "null destination returns without reading source");

    CompositeRuntime composite;
    composite.bytes[0x2000u] = 1u;
    composite.bytes[0x2010u] = 2u;
    composite.bytes[0x2020u] = 3u;
    composite.bytes[0x2030u] = 4u;
    composite.words[0x2014u] = 0x3000u;
    composite.words[0x2018u] = 0x3030u;
    composite.text_fields[0x2000u] = "name";
    composite.text_fields[0x2020u] = "path";
    composite.text_fields[0x2030u] = "tail";
    composite.records.Put(0x3000u, 1u, 0x5001u, "one", 1u, 11u);
    composite.records.Put(0x3018u, 2u, 0x5101u, "two", 1u, 22u);
    pal4::EffectSystemConfigTree::CopyCompositePayload(
        0x1000u, 0x2000u, composite);
    Check(composite.bytes[0x1000u] == 1u &&
              composite.bytes[0x1010u] == 2u &&
              composite.bytes[0x1020u] == 3u &&
              composite.bytes[0x1030u] == 4u &&
              composite.words[0x1014u] == 0xa000u &&
              composite.words[0x1018u] == 0xa030u &&
              composite.words[0x101cu] == 0xa030u &&
              composite.words[0x1034u] == 0u &&
              composite.words[0x1038u] == 0u &&
              composite.words[0x103cu] == 0u &&
              composite.text_fields[0x1000u] == "name" &&
              composite.text_fields[0x1020u] == "path" &&
              composite.text_fields[0x1030u] == "tail" &&
              composite.records.words[0xa014u] == 11u &&
              composite.records.words[0xa02cu] == 22u &&
              composite.events == std::vector<std::string>{
                  "byte:4096", "word:4100", "word:4104", "word:4108",
                  "text:4096", "byte:4112", "allocate:48",
                  "word:4116", "record:40960:12288",
                  "record:40984:12312", "word:4120", "word:4124",
                  "byte:4128", "flag:4128", "text:4128",
                  "byte:4144", "word:4148", "word:4152",
                  "word:4156", "text:4144"},
          "composite copy preserves field order and deep-copies records");
    composite.events.clear();
    pal4::EffectSystemConfigTree::CopyCompositePayload(
        0u, 0x2000u, composite);
    Check(composite.events.empty(),
          "null composite destination is skipped");

    NodeRuntime unique_tree(0u);
    unique_tree.CheckInvariants(0u, false);
    constexpr std::uint32_t source_base = 0x4000u;
    const std::vector<std::string> keys{
        "m", "g", "s", "d", "j", "p", "w", "h", "i", "a",
        "f", "k", "q", "u", "z", "b", "c", "e", "n", "r"};
    for (std::size_t i = 0u; i < keys.size(); ++i) {
        const auto source_address = source_base +
                                    static_cast<std::uint32_t>(i * 0x100u);
        unique_tree.payloads[source_address] = keys[i];
        const auto result = pal4::EffectSystemConfigTree::InsertElement(
            0x1000u, source_address, unique_tree);
        Check(result.inserted && result.node >= 0xa000u,
              "unique insertion returns newly allocated node");
        unique_tree.CheckInvariants(static_cast<std::uint32_t>(i + 1u),
                                    false);
    }
    unique_tree.payloads[0x8000u] = "j";
    const auto before_allocations = unique_tree.allocations;
    const auto duplicate = pal4::EffectSystemConfigTree::InsertElement(
        0x1000u, 0x8000u, unique_tree);
    Check(!duplicate.inserted &&
              unique_tree.payloads.at(duplicate.node + 12u) == "j" &&
              unique_tree.allocations == before_allocations &&
              unique_tree.copies == keys.size(),
          "unique branch returns existing node without allocation");
    unique_tree.CheckInvariants(static_cast<std::uint32_t>(keys.size()),
                                false);

    LookupRuntime lower_bound;
    lower_bound.words = unique_tree.words;
    lower_bound.node_names = unique_tree.payloads;
    Check(pal4::EffectSystemConfigTree::FindLowerBoundNode(
              0x1000u, "a", lower_bound) == unique_tree.FindNode("a") &&
              pal4::EffectSystemConfigTree::FindLowerBoundNode(
                  0x1000u, "j", lower_bound) == unique_tree.FindNode("j") &&
              pal4::EffectSystemConfigTree::FindLowerBoundNode(
                  0x1000u, "g0", lower_bound) == unique_tree.FindNode("h") &&
              pal4::EffectSystemConfigTree::FindLowerBoundNode(
                  0x1000u, "zz", lower_bound) == 0x2000u,
          "lower bound returns exact, next greater and header candidates");
    lower_bound.words[0x2004u] = 0x9000u;
    Check(pal4::EffectSystemConfigTree::FindLowerBoundNode(
              0x1000u, "any", lower_bound) == 0x2000u,
          "empty tree lower bound returns its header");

    unique_tree.payloads[0x8000u] = "j";
    unique_tree.payloads[0x8100u] = "g0";
    unique_tree.payloads[0x8200u] = "zz";
    Check(pal4::EffectSystemConfigTree::FindLowerBoundPayload(
              0x1000u, 0x8000u, unique_tree) ==
              unique_tree.FindNode("j") &&
              pal4::EffectSystemConfigTree::FindLowerBoundPayload(
                  0x1000u, 0x8100u, unique_tree) ==
                  unique_tree.FindNode("h") &&
              pal4::EffectSystemConfigTree::FindLowerBoundPayload(
                  0x1000u, 0x8200u, unique_tree) == 0x2000u,
          "native-payload lower bound returns exact, next and header");

    auto backward_cursor = unique_tree.FindNode("m");
    (void)pal4::EffectSystemConfigTree::AdvanceNodeCursorBackward(
        backward_cursor, unique_tree);
    Check(backward_cursor == unique_tree.FindNode("k"),
          "predecessor chooses the rightmost left-subtree node");
    backward_cursor = unique_tree.FindNode("a");
    (void)pal4::EffectSystemConfigTree::AdvanceNodeCursorBackward(
        backward_cursor, unique_tree);
    Check(backward_cursor == 0x2000u,
          "minimum predecessor climbs to the header");
    unique_tree.words[0x204cu] = 0u;
    backward_cursor = 0x2000u;
    Check(pal4::EffectSystemConfigTree::AdvanceNodeCursorBackward(
              backward_cursor, unique_tree) ==
              unique_tree.FindNode("z") &&
              backward_cursor == unique_tree.FindNode("z"),
          "header predecessor uses the maximum node");

    auto node_cursor = unique_tree.FindNode("m");
    Check(pal4::EffectSystemConfigTree::AdvanceNodeCursor(
              node_cursor, unique_tree) == 0x9000u &&
              node_cursor == unique_tree.FindNode("n"),
          "node successor descends to leftmost right-subtree node");
    node_cursor = unique_tree.FindNode("a");
    const auto parent_result =
        pal4::EffectSystemConfigTree::AdvanceNodeCursor(
            node_cursor, unique_tree);
    Check(node_cursor == unique_tree.FindNode("b") &&
              parent_result == node_cursor,
          "node successor climbs from a left child");
    node_cursor = unique_tree.FindNode("z");
    (void)pal4::EffectSystemConfigTree::AdvanceNodeCursor(
        node_cursor, unique_tree);
    Check(node_cursor == 0x2000u,
          "rightmost node advances to header sentinel");
    node_cursor = unique_tree.FindNode("m");
    std::uint32_t previous_node = 0u;
    Check(&pal4::EffectSystemConfigTree::AdvanceNodeCursorPost(
              node_cursor, previous_node, unique_tree) == &previous_node &&
              previous_node == unique_tree.FindNode("m") &&
              node_cursor == unique_tree.FindNode("n"),
          "post-increment returns the output slot holding old node");

    std::set<std::string> remaining(keys.begin(), keys.end());
    const std::vector<std::string> erase_order{
        "m", "a", "z", "g", "s", "d", "j", "p", "w", "h",
        "i", "f", "k", "q", "u", "b", "c", "e", "n", "r"};
    for (const auto& key : erase_order) {
        const auto node = unique_tree.FindNode(key);
        const auto next = remaining.upper_bound(key);
        const auto expected = next == remaining.end()
                                  ? 0x2000u : unique_tree.FindNode(*next);
        Check(node != 0u &&
                  pal4::EffectSystemConfigTree::EraseElement(
                      0x1000u, node, unique_tree, unique_tree) == expected &&
                  unique_tree.freed_nodes.back() == node &&
                  unique_tree.destroyed_configs.back() == node + 28u,
              "erase returns successor and destroys requested payload");
        remaining.erase(key);
        unique_tree.CheckInvariants(
            static_cast<std::uint32_t>(remaining.size()), false);
    }
    Check(unique_tree.freed_nodes.size() == keys.size(),
          "all nodes are released exactly once");

    NodeRuntime multi_tree(1u);
    multi_tree.payloads[0x4000u] = "same";
    multi_tree.payloads[0x4100u] = "same";
    multi_tree.payloads[0x4200u] = "same";
    for (const auto address : {0x4000u, 0x4100u, 0x4200u}) {
        const auto result = pal4::EffectSystemConfigTree::InsertElement(
            0x1000u, address, multi_tree);
        Check(result.inserted, "duplicate-enabled mode inserts equal keys");
    }
    multi_tree.CheckInvariants(3u, true);
    Check(pal4::EffectSystemConfigTree::DestroySubtree(
              0x9000u, multi_tree, multi_tree) == 0x9000u &&
              multi_tree.freed_nodes.empty(),
          "empty subtree destruction is a no-op");
    Check(pal4::EffectSystemConfigTree::DestroySubtree(
              multi_tree.words.at(0x2004u), multi_tree,
              multi_tree) == 0x9000u &&
              multi_tree.freed_nodes.size() == 3u &&
              multi_tree.destroyed_configs.size() == 3u,
          "subtree destructor cleans all nested values and nodes");

    NodeRuntime direct_tree(0u);
    direct_tree.payloads[0x4000u] = "root";
    direct_tree.payloads[0x4100u] = "left";
    const auto direct_root = pal4::EffectSystemConfigTree::InsertNodeAt(
        0x1000u, 0x9000u, 0x2000u, 0x4000u, direct_tree);
    Check(direct_root == 0xa000u &&
              direct_tree.words.at(0x2004u) == direct_root,
          "node helper inserts the first root");
    const auto direct_left = pal4::EffectSystemConfigTree::InsertNodeAt(
        0x1000u, direct_root, direct_root, 0x4100u, direct_tree);
    Check(direct_left == 0xa100u &&
              direct_tree.words.at(direct_root) == direct_left,
          "non-nil child hint selects the left link");
    direct_tree.CheckInvariants(2u, false);
    NodeRuntime constructed_tree(0u);
    const auto constructed = pal4::EffectSystemConfigTree::CreateNode(
        0x2000u, 0u, constructed_tree);
    Check(constructed == 0xa000u &&
              constructed_tree.words.at(constructed + 4u) == 0x2000u &&
              constructed_tree.words.at(constructed + 76u) == 0u,
          "node constructor allocates 80 bytes and stores parent and color");
    pal4::EffectSystemConfigTree::RotateRight(
        0x1000u, direct_root, direct_tree);
    Check(direct_tree.words.at(0x2004u) == direct_left &&
              direct_tree.words.at(direct_left + 4u) == 0x2000u &&
              direct_tree.words.at(direct_left + 8u) == direct_root &&
              direct_tree.words.at(direct_root + 4u) == direct_left,
          "right rotation replaces root and repairs parent links");
    pal4::EffectSystemConfigTree::RotateLeft(
        0x1000u, direct_left, direct_tree);
    direct_tree.CheckInvariants(2u, false);

    NodeRuntime internal_tree(0u);
    for (const auto [address, key] : {
             std::pair{0x4000u, "b"}, {0x4100u, "a"},
             {0x4200u, "d"}, {0x4300u, "c"}}) {
        internal_tree.payloads[address] = key;
        (void)pal4::EffectSystemConfigTree::InsertElement(
            0x1000u, address, internal_tree);
    }
    const auto internal_root = internal_tree.FindNode("b");
    const auto internal_right = internal_tree.FindNode("d");
    const auto internal_pivot = internal_tree.FindNode("c");
    pal4::EffectSystemConfigTree::RotateRight(
        0x1000u, internal_right, internal_tree);
    Check(internal_tree.words.at(internal_root + 8u) == internal_pivot &&
              internal_tree.words.at(internal_pivot + 4u) == internal_root &&
              internal_tree.words.at(internal_pivot + 8u) == internal_right &&
              internal_tree.words.at(internal_right + 4u) == internal_pivot,
          "non-root rotation reconnects parent and moved child");
    pal4::EffectSystemConfigTree::RotateLeft(
        0x1000u, internal_pivot, internal_tree);
    internal_tree.CheckInvariants(4u, false);
}
