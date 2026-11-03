#include "pal4/sound_player.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <utility>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISoundPlayerRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::map<std::uint32_t, bool> allocation_result;
    std::map<std::uint32_t, std::uint32_t> next_node;
    std::map<std::pair<std::uint32_t, std::uint32_t>, bool> less_answers;
    std::vector<std::uint32_t> freed;
    std::vector<std::uint32_t> deleted_nodes;
    std::uint32_t found_node = 0u;
    std::uint32_t insert_child = 0u;
    std::uint32_t insert_parent = 0u;
    std::uint32_t insert_key = 0u;
    int insert_count = 0;
    std::uint32_t previous_result = 0u;
    bool less = false;
    std::uint32_t played_path = 0u;
    std::uint32_t played_repeat = 0u;
    std::uint8_t play_result = 0x5au;
    bool throw_grow = false;
    std::uint32_t copied_destination = 0u;
    std::uint32_t copied_source = 0u;
    std::uint32_t copied_bytes = 0u;
    std::uint32_t assigned_destination = 0u;
    std::uint32_t assigned_source = 0u;
    std::uint32_t assigned_count = 0u;
    int temporary_depth = 0;
    int fail_key = -1;
    int fail_lookup = -1;
    int current_event = -1;

    std::uint32_t ImportedBaseVtable() override {
        calls.emplace_back("base vtable");
        return 0x770000u;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.emplace_back("read byte " + std::to_string(address));
        return bytes[address];
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read word " + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.emplace_back("word " + std::to_string(address));
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.emplace_back("byte " + std::to_string(address));
        bytes[address] = value;
    }
    void ConstructTree(std::uint32_t tree) override {
        calls.emplace_back("construct tree");
        Check(tree == 0x5004u && words.at(0x5000u) == 0x770000u &&
                  bytes.at(0x5004u) == 0xa5u &&
                  bytes.at(0x5005u) == 0xa5u &&
                  bytes.at(0x500cu) == 0u,
              "base fields initialized before tree constructor");
    }
    void BeginTemporaryText(std::uint8_t seed) override {
        calls.emplace_back("begin temp");
        Check(seed == 0xa5u && temporary_depth == 0,
              "temporary record gets stack seed");
        ++temporary_depth;
    }
    void EndTemporaryText() override {
        calls.emplace_back("end temp");
        Check(temporary_depth == 1, "temporary record cleanup balanced");
        --temporary_depth;
    }
    std::uint32_t CopyCeguiKey(pal4::SoundEvent event) override {
        current_event = static_cast<int>(event);
        calls.emplace_back("copy key " + std::to_string(current_event));
        if (current_event == fail_key)
            throw std::runtime_error("key copy failed");
        return 0xa000u + static_cast<std::uint32_t>(current_event);
    }
    void DestroyCeguiKey(std::uint32_t key) override {
        calls.emplace_back("destroy key");
        Check(key == 0xa000u + static_cast<std::uint32_t>(current_event),
              "destroy current CEGUI key");
    }
    std::uint32_t FindOrInsert(std::uint32_t tree,
                               std::uint32_t key) override {
        calls.emplace_back("find " + std::to_string(current_event));
        Check(tree == 0x5004u && key ==
                  0xa000u + static_cast<std::uint32_t>(current_event) &&
                  current_event != 1,
              "find event key and tree");
        if (current_event == fail_lookup)
            throw std::runtime_error("lookup failed");
        return 0xb000u + 0x100u *
                           static_cast<std::uint32_t>(current_event);
    }
    std::uint32_t InsertAndResolve(std::uint32_t tree,
                                   std::uint32_t key) override {
        calls.emplace_back("insert " + std::to_string(current_event));
        Check(tree == 0x5004u && key == 0xa001u &&
                  current_event == 1,
              "denied-click key uses insertion variant");
        if (current_event == fail_lookup)
            throw std::runtime_error("insertion failed");
        return 0xb100u;
    }
    bool AllocateText(std::uint32_t record, std::uint32_t length,
                      std::uint8_t mode) override {
        calls.emplace_back("allocate text " + std::to_string(record));
        Check(mode == 1u && length == 8u,
              "wav path allocation mode and length");
        const auto found = allocation_result.find(record);
        if (found != allocation_result.end() && !found->second)
            return false;
        words[record + 4u] = 0xc000u + (record - 0xb0a4u);
        return true;
    }
    void CopyText(std::uint32_t destination,
                  std::string_view text) override {
        calls.emplace_back("copy " + std::string(text));
        for (std::size_t i = 0; i < text.size(); ++i)
            bytes[destination + static_cast<std::uint32_t>(i)] =
                static_cast<std::uint8_t>(text[i]);
    }
    void SetTextSize(std::uint32_t record,
                     std::uint32_t length) override {
        calls.emplace_back("set size");
        words[record + 8u] = length;
        bytes[words.at(record + 4u) + length] = 0u;
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.emplace_back("free " + std::to_string(address));
        freed.push_back(address);
        return 0xde000000u | address;
    }
    void FreeTreeNodes(std::uint32_t tree,
                       std::uint32_t root) override {
        calls.emplace_back("free tree nodes");
        Check(tree == 0x5004u && root == 0x9200u,
              "bulk tree free receives root");
    }
    std::uint32_t NextTreeNode(std::uint32_t node) override {
        calls.emplace_back("next node");
        return next_node.at(node);
    }
    void DeleteTreeNode(std::uint32_t tree,
                        std::uint32_t node) override {
        calls.emplace_back("delete tree node");
        Check(tree == 0x5004u, "incremental tree owner");
        deleted_nodes.push_back(node);
    }
    void GetFirstTreeNode(std::uint32_t tree) override {
        calls.emplace_back("first node");
        Check(tree == 0x5004u, "first node after bulk reset");
    }
    std::uint32_t FindTreeNode(std::uint32_t tree,
                               std::uint32_t key) override {
        calls.emplace_back("find tree node");
        Check(tree == 0x5004u && key == 0xa000u,
              "playback lookup arguments");
        return found_node;
    }
    bool CeguiLess(std::uint32_t left,
                   std::uint32_t right) override {
        calls.emplace_back("cegui less");
        const auto answer = less_answers.find({left, right});
        if (answer != less_answers.end()) return answer->second;
        Check(left == 0xa000u && right == found_node + 12u,
              "candidate key comparison or configured tree comparison");
        return less;
    }
    std::uint32_t EmptyTextPointer() override {
        calls.emplace_back("empty text");
        return 0x8800u;
    }
    std::uint8_t PlayMusic(std::uint32_t path,
                           std::uint32_t repeat) override {
        calls.emplace_back("play music");
        played_path = path;
        played_repeat = repeat;
        return play_result;
    }
    std::uint32_t InsertTreeNode(std::uint32_t tree,
                                 std::uint32_t child,
                                 std::uint32_t parent,
                                 std::uint32_t key) override {
        calls.emplace_back("insert tree node");
        Check(tree == 0x5004u, "insertion tree address");
        insert_child = child;
        insert_parent = parent;
        insert_key = key;
        ++insert_count;
        return 0xb000u;
    }
    std::uint32_t PreviousTreeNode(std::uint32_t node) override {
        calls.emplace_back("previous tree node");
        Check(node == 0x9100u, "predecessor starts at search parent");
        return previous_result;
    }
    void GrowCeguiString(std::uint32_t string,
                          std::uint32_t length) override {
        calls.emplace_back("grow cegui");
        Check(string == 0x5000u, "grow destination string");
        if (throw_grow) throw std::runtime_error("grow failed");
        if (length > 32u) {
            words[string + 4u] = 64u;
            words[string + 148u] = 0xd000u;
        }
    }
    void SetCeguiStringLength(std::uint32_t string,
                              std::uint32_t length) override {
        calls.emplace_back("set cegui length");
        words[string] = length;
        const std::uint32_t data = words[string + 4u] <= 32u
                                       ? string + 20u
                                       : words.at(string + 148u);
        words[data + length * 4u] = 0u;
    }
    void CopyMemory(std::uint32_t destination,
                     std::uint32_t source,
                     std::uint32_t count) override {
        calls.emplace_back("copy memory");
        copied_destination = destination;
        copied_source = source;
        copied_bytes = count;
        for (std::uint32_t i = 0; i < count; ++i)
            bytes[destination + i] = bytes[source + i];
    }
    std::uint32_t AssignTextRecord(std::uint32_t destination,
                                   std::uint32_t source,
                                   std::uint32_t start,
                                   std::uint32_t count) override {
        calls.emplace_back("assign text");
        Check(destination == 0x5098u && start == 0u &&
                  words.at(destination + 4u) == 0u &&
                  words.at(destination + 8u) == 0u &&
                  words.at(destination + 12u) == 0u,
              "embedded text record zeroed before assignment");
        assigned_destination = destination;
        assigned_source = source;
        assigned_count = count;
        return 0xdeadbeefu;
    }
};

std::size_t Index(const Runtime& runtime, std::string_view call) {
    for (std::size_t i = 0; i < runtime.calls.size(); ++i)
        if (runtime.calls[i] == call) return i;
    throw std::runtime_error("missing call: " + std::string(call));
}

std::size_t NthIndex(const Runtime& runtime, std::string_view call,
                     std::size_t ordinal) {
    for (std::size_t i = 0; i < runtime.calls.size(); ++i) {
        if (runtime.calls[i] != call) continue;
        if (ordinal-- == 0u) return i;
    }
    throw std::runtime_error("missing nth call: " + std::string(call));
}

bool Has(const Runtime& runtime, std::string_view call) {
    for (const auto& entry : runtime.calls)
        if (entry == call) return true;
    return false;
}

void CheckSuccess() {
    Runtime runtime;
    Check(pal4::SoundPlayer::Construct(0x5000u, 0xa5u, runtime) ==
              0x5000u &&
              runtime.words.at(0x5000u) == pal4::SoundPlayer::kVtable &&
              runtime.temporary_depth == 0,
          "constructor return, vtable and temporary cleanup");
    Check(Index(runtime, "construct tree") <
              NthIndex(runtime, "word 20480", 1u) &&
              Index(runtime, "find 0") <
              Index(runtime, "copy wa01.wav") &&
              Index(runtime, "copy wa01.wav") <
              Index(runtime, "insert 1") &&
              Index(runtime, "insert 1") <
              Index(runtime, "copy wa05.wav") &&
              Index(runtime, "copy wa05.wav") <
              Index(runtime, "find 2") &&
              Index(runtime, "find 2") <
              Index(runtime, "copy wa02.wav") &&
              Index(runtime, "copy wa02.wav") <
              Index(runtime, "set size"),
          "three sound bindings follow IDA order");
    for (const auto [node, name] : {
             std::pair{0xb000u, "wa01.wav"},
             std::pair{0xb100u, "wa05.wav"},
             std::pair{0xb200u, "wa02.wav"}}) {
        const std::uint32_t record = node + 164u;
        const std::uint32_t data = runtime.words.at(record + 4u);
        Check(runtime.words.at(record + 8u) == 8u &&
                  runtime.bytes.at(data) == 'w' &&
                  runtime.bytes.at(data + 7u) == 'v' &&
                  runtime.bytes.at(data + 8u) == 0u &&
                  std::string_view(name).size() == 8u,
              "sound path bytes, length and terminator");
    }
    Check(Index(runtime, "destroy key") <
              Index(runtime, "end temp") &&
              Index(runtime, "end temp") <
              Index(runtime, "allocate text 45220"),
          "first temporary key and text released before file allocation");
}

void CheckAllocationFailures() {
    for (const auto failed_node : {0xb000u, 0xb100u, 0xb200u}) {
        Runtime runtime;
        runtime.allocation_result[failed_node + 164u] = false;
        Check(pal4::SoundPlayer::Construct(
                  0x5000u, 0xa5u, runtime) == 0x5000u &&
                  runtime.temporary_depth == 0 &&
                  runtime.words.find(failed_node + 172u) ==
                      runtime.words.end(),
              "failed path allocation skips copy and still returns object");
        Check(Has(runtime, "find 2") &&
                  !Has(runtime, "copy " +
                                    std::string(failed_node == 0xb000u
                                                    ? "wa01.wav"
                                                    : failed_node == 0xb100u
                                                          ? "wa05.wav"
                                                          : "wa02.wav")),
              "all event registrations occur despite one path failure");
    }
}

void CheckExceptionalCleanup() {
    for (const int failed_event : {0, 1, 2}) {
        Runtime runtime;
        runtime.fail_lookup = failed_event;
        bool failed = false;
        try {
            (void)pal4::SoundPlayer::Construct(
                0x5000u, 0xa5u, runtime);
        } catch (const std::runtime_error&) {
            failed = true;
        }
        Check(failed && runtime.temporary_depth == 0 &&
                  runtime.calls.at(runtime.calls.size() - 2u) ==
                      "destroy key" &&
                  runtime.calls.back() == "end temp" &&
                  runtime.words.at(0x5000u) ==
                      pal4::SoundPlayer::kVtable,
              "lookup exception destroys CEGUI key then temp text");
    }
    Runtime key_failure;
    key_failure.fail_key = 1;
    bool failed = false;
    try {
        (void)pal4::SoundPlayer::Construct(
            0x5000u, 0xa5u, key_failure);
    } catch (const std::runtime_error&) {
        failed = true;
    }
    Check(failed && key_failure.temporary_depth == 0 &&
              key_failure.calls.back() == "end temp" &&
              Index(key_failure, "copy key 1") <
                  key_failure.calls.size() - 1u,
          "failed key copy releases only the created text record");
}

void CheckTreeDestroy() {
    Runtime bulk;
    bulk.words[0x5008u] = 0x9000u;
    bulk.words[0x5010u] = 2u;
    bulk.words[0x9000u] = 0x9100u;
    bulk.words[0x9004u] = 0x9200u;
    bulk.words[0x8edfc4u] = 2u;
    bulk.words[0x8edfc8u] = 0xa000u;
    Check(pal4::SoundPlayer::DestroyTree(0x5004u, bulk) == 1u &&
              Has(bulk, "free tree nodes") &&
              Has(bulk, "first node") &&
              bulk.words.at(0x9000u) == 0x9000u &&
              bulk.words.at(0x9008u) == 0x9000u &&
              bulk.words.at(0x5008u) == 0u &&
              bulk.words.at(0x5010u) == 0u &&
              bulk.words.at(0x8edfc4u) == 1u &&
              bulk.words.at(0x8edfc8u) == 0xa000u &&
              bulk.freed == std::vector<std::uint32_t>{0x9000u},
          "nonempty tree bulk frees nodes and retains shared nil");

    Runtime incremental;
    incremental.words[0x5008u] = 0x9000u;
    incremental.words[0x5010u] = 0u;
    incremental.words[0x9000u] = 0x9100u;
    incremental.words[0x8edfc4u] = 1u;
    incremental.words[0x8edfc8u] = 0xa000u;
    incremental.next_node[0x9100u] = 0x9200u;
    incremental.next_node[0x9200u] = 0x9000u;
    Check(pal4::SoundPlayer::DestroyTree(
              0x5004u, incremental) == 0xde00a000u &&
              incremental.deleted_nodes ==
                  (std::vector<std::uint32_t>{0x9100u, 0x9200u}) &&
              incremental.freed ==
                  (std::vector<std::uint32_t>{0x9000u, 0xa000u}) &&
              incremental.words.at(0x8edfc4u) == 0u &&
              incremental.words.at(0x8edfc8u) == 0u,
          "zero-count residual nodes and last nil reference released");

    Runtime empty_nil;
    empty_nil.words[0x5008u] = 0x9000u;
    empty_nil.words[0x9000u] = 0x9000u;
    empty_nil.words[0x8edfc4u] = 1u;
    Check(pal4::SoundPlayer::DestroyTree(0x5004u, empty_nil) == 0u &&
              empty_nil.freed == std::vector<std::uint32_t>{0x9000u},
          "last null nil returns zero after head free");
}

void CheckPlayback() {
    Runtime runtime;
    runtime.words[0x5008u] = 0x9017u;
    runtime.found_node = 0x9017u;
    Check(pal4::SoundPlayer::FindAndPlayMusic(
              0x5000u, 0xa000u, runtime) == 0x17u &&
              !Has(runtime, "cegui less") &&
              !Has(runtime, "play music"),
          "missing key returns low byte of tree head");

    runtime.found_node = 0x9100u;
    runtime.less = true;
    Check(pal4::SoundPlayer::FindAndPlayMusic(
              0x5000u, 0xa000u, runtime) == 0x17u &&
              !Has(runtime, "play music"),
          "strictly greater candidate is also a miss");

    runtime.less = false;
    runtime.words[0x91a8u] = 0xc000u;
    Check(pal4::SoundPlayer::FindAndPlayMusic(
              0x5000u, 0xa000u, runtime) == 0x5au &&
              runtime.played_path == 0xc000u &&
              runtime.played_repeat == 1u,
          "found key plays its stored path with repeat flag one");
    runtime.words[0x91a8u] = 0u;
    Check(pal4::SoundPlayer::FindAndPlayMusic(
              0x5000u, 0xa000u, runtime) == 0x5au &&
              runtime.played_path == 0x8800u &&
              Has(runtime, "empty text"),
          "null path uses the global empty text pointer");
}

void CheckFindOrInsert() {
    Runtime empty;
    empty.words[0x5008u] = 0x9000u;
    empty.words[0x8edfc8u] = 0x8000u;
    empty.words[0x9000u] = 0x9000u;
    empty.words[0x9004u] = 0x8000u;
    Check(pal4::SoundPlayer::FindOrInsertNode(
              0x5004u, 0x7000u, 0xa000u, empty) == 0x7000u &&
              empty.words.at(0x7000u) == 0xb000u &&
              empty.bytes.at(0x7004u) == 1u &&
              empty.insert_child == 0x8000u &&
              empty.insert_parent == 0x9000u,
          "empty unique tree inserts at head");

    Runtime duplicate;
    duplicate.words[0x5008u] = 0x9000u;
    duplicate.words[0x8edfc8u] = 0x8000u;
    duplicate.words[0x9004u] = 0x9100u;
    duplicate.words[0x9108u] = 0x8000u;
    duplicate.bytes[0x500cu] = 1u;
    duplicate.less_answers[{0xa000u, 0x910cu}] = false;
    (void)pal4::SoundPlayer::FindOrInsertNode(
        0x5004u, 0x7000u, 0xa000u, duplicate);
    Check(duplicate.insert_count == 1 &&
              duplicate.insert_parent == 0x9100u &&
              duplicate.insert_child == 0x8000u &&
              duplicate.bytes.at(0x7004u) == 1u,
          "duplicate-allowed tree inserts without equality check");

    Runtime equal;
    equal.words[0x5008u] = 0x9000u;
    equal.words[0x8edfc8u] = 0x8000u;
    equal.words[0x9004u] = 0x9100u;
    equal.words[0x9108u] = 0x8000u;
    equal.less_answers[{0xa000u, 0x910cu}] = false;
    equal.less_answers[{0x910cu, 0xa000u}] = false;
    (void)pal4::SoundPlayer::FindOrInsertNode(
        0x5004u, 0x7000u, 0xa000u, equal);
    Check(equal.words.at(0x7000u) == 0x9100u &&
              equal.bytes.at(0x7004u) == 0u &&
              equal.insert_count == 0,
          "equal key returns existing unique node");

    Runtime minimum;
    minimum.words[0x5008u] = 0x9000u;
    minimum.words[0x8edfc8u] = 0x8000u;
    minimum.words[0x9000u] = 0x9100u;
    minimum.words[0x9004u] = 0x9100u;
    minimum.words[0x9100u] = 0x8000u;
    minimum.less_answers[{0xa000u, 0x910cu}] = true;
    (void)pal4::SoundPlayer::FindOrInsertNode(
        0x5004u, 0x7000u, 0xa000u, minimum);
    Check(minimum.insert_count == 1 &&
              !Has(minimum, "previous tree node"),
          "key before minimum inserts without predecessor lookup");

    Runtime predecessor;
    predecessor.words[0x5008u] = 0x9000u;
    predecessor.words[0x8edfc8u] = 0x8000u;
    predecessor.words[0x9000u] = 0x9200u;
    predecessor.words[0x9004u] = 0x9100u;
    predecessor.words[0x9100u] = 0x8000u;
    predecessor.previous_result = 0x9200u;
    predecessor.less_answers[{0xa000u, 0x910cu}] = true;
    predecessor.less_answers[{0x920cu, 0xa000u}] = false;
    (void)pal4::SoundPlayer::FindOrInsertNode(
        0x5004u, 0x7000u, 0xa000u, predecessor);
    Check(predecessor.words.at(0x7000u) == 0x9200u &&
              predecessor.bytes.at(0x7004u) == 0u &&
              predecessor.insert_count == 0 &&
              Has(predecessor, "previous tree node"),
          "predecessor equal key is reused");

    Runtime greater;
    greater.words[0x5008u] = 0x9000u;
    greater.words[0x8edfc8u] = 0x8000u;
    greater.words[0x9004u] = 0x9100u;
    greater.words[0x9108u] = 0x8000u;
    greater.less_answers[{0xa000u, 0x910cu}] = false;
    greater.less_answers[{0x910cu, 0xa000u}] = true;
    (void)pal4::SoundPlayer::FindOrInsertNode(
        0x5004u, 0x7000u, 0xa000u, greater);
    Check(greater.insert_count == 1 &&
              greater.insert_parent == 0x9100u &&
              greater.insert_key == 0xa000u,
          "key greater than candidate inserts at its parent");
}

void CheckKeyRecordCopy() {
    Runtime small;
    small.words[0x6000u] = 2u;
    small.words[0x6004u] = 32u;
    small.bytes[0x6014u] = 'A';
    small.bytes[0x6018u] = 'B';
    small.bytes[0x7000u] = 0xa5u;
    Check(pal4::SoundEventKeyRecord::CopyWithProperties(
              0x5000u, 0x6000u, 0x7000u, small) == 0x5000u &&
              small.words.at(0x5000u) == 2u &&
              small.words.at(0x5004u) == 32u &&
              small.words.at(0x501cu) == 0u &&
              small.copied_destination == 0x5014u &&
              small.copied_source == 0x6014u &&
              small.copied_bytes == 8u &&
              small.bytes.at(0x5014u) == 'A' &&
              small.bytes.at(0x5018u) == 'B' &&
              small.bytes.at(0x5098u) == 0xa5u &&
              small.assigned_destination == 0x5098u &&
              small.assigned_source == 0x7000u &&
              small.assigned_count == 0xffffffffu,
          "inline UTF-32 copy and attached text assignment");
    Check(Index(small, "grow cegui") <
              Index(small, "copy memory") &&
              Index(small, "copy memory") <
              Index(small, "assign text"),
          "grow, copy and text assignment order");

    Runtime large;
    large.words[0x6000u] = 33u;
    large.words[0x6004u] = 64u;
    large.words[0x6094u] = 0xb000u;
    large.bytes[0xb000u] = 'Z';
    large.bytes[0xb083u] = 0x42u;
    large.bytes[0x7000u] = 0x7eu;
    Check(pal4::SoundEventKeyRecord::CopyWithProperties(
              0x5000u, 0x6000u, 0x7000u, large) == 0x5000u &&
              large.words.at(0x5004u) == 64u &&
              large.words.at(0xd084u) == 0u &&
              large.copied_destination == 0xd000u &&
              large.copied_source == 0xb000u &&
              large.copied_bytes == 132u &&
              large.bytes.at(0xd000u) == 'Z' &&
              large.bytes.at(0xd083u) == 0x42u,
          "heap UTF-32 buffers selected above inline capacity");

    Runtime failing;
    failing.words[0x6000u] = 33u;
    failing.throw_grow = true;
    bool threw = false;
    try {
        (void)pal4::SoundEventKeyRecord::CopyWithProperties(
            0x5000u, 0x6000u, 0x7000u, failing);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && failing.words.at(0x5000u) == 0u &&
              failing.words.at(0x5004u) == 32u &&
              !Has(failing, "assign text"),
          "grow failure leaves only the initialized string header");
}

void CheckCopyFromRecord() {
    Runtime null_target;
    Check(pal4::SoundEventKeyRecord::CopyFromRecord(
              0u, 0x6000u, null_target) == 0u &&
              null_target.calls.empty(),
          "null destination returns zero without source access");

    Runtime small;
    small.words[0x6000u] = 2u;
    small.words[0x6004u] = 32u;
    small.bytes[0x6014u] = 'A';
    small.bytes[0x6018u] = 'B';
    small.bytes[0x6098u] = 0x91u;
    Check(pal4::SoundEventKeyRecord::CopyFromRecord(
              0x5000u, 0x6000u, small) == 0xdeadbeefu &&
              small.words.at(0x5000u) == 2u &&
              small.words.at(0x501cu) == 0u &&
              small.copied_source == 0x6014u &&
              small.copied_destination == 0x5014u &&
              small.copied_bytes == 8u &&
              small.bytes.at(0x5014u) == 'A' &&
              small.bytes.at(0x5018u) == 'B' &&
              small.bytes.at(0x5098u) == 0x91u &&
              small.assigned_source == 0x6098u,
          "inline full record copy and raw assignment return");
    Check(Index(small, "grow cegui") <
              Index(small, "set cegui length") &&
              Index(small, "set cegui length") <
              Index(small, "copy memory") &&
              Index(small, "copy memory") <
              Index(small, "assign text"),
          "setlen precedes UTF-32 copy and text assignment");

    Runtime large;
    large.words[0x6000u] = 33u;
    large.words[0x6004u] = 64u;
    large.words[0x6094u] = 0xb000u;
    large.bytes[0xb000u] = 'Z';
    large.bytes[0x6098u] = 0x55u;
    Check(pal4::SoundEventKeyRecord::CopyFromRecord(
              0x5000u, 0x6000u, large) == 0xdeadbeefu &&
              large.words.at(0x5000u) == 33u &&
              large.words.at(0xd084u) == 0u &&
              large.copied_source == 0xb000u &&
              large.copied_destination == 0xd000u &&
              large.copied_bytes == 132u &&
              large.bytes.at(0xd000u) == 'Z' &&
              large.bytes.at(0x5098u) == 0x55u,
          "heap full record copy uses source and destination pointers");
}

}  // namespace

int main() {
    try {
        CheckSuccess();
        CheckAllocationFailures();
        CheckExceptionalCleanup();
        CheckTreeDestroy();
        CheckPlayback();
        CheckFindOrInsert();
        CheckKeyRecordCopy();
        CheckCopyFromRecord();
        std::cout << "sound_player_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
