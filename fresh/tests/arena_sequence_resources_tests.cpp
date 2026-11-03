#include "pal4/arena_sequence_resources.h"

#include <bit>
#include <algorithm>
#include <cstdint>
#include <map>
#include <iostream>
#include <functional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using Resources = pal4::ArenaSequenceResources;
using Call = pal4::ArenaSequenceCall;

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IArenaSequenceRuntime {
    struct Invocation {
        pal4::ArenaSequenceCall operation;
        std::vector<std::uint32_t> arguments;
        bool operator==(const Invocation&) const = default;
    };
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> frees;
    std::vector<Invocation> calls;
    std::uint32_t next_allocation = 0xa000u;
    std::uint32_t next_head = 0xb000u;
    std::uint32_t found_node = 0u;
    bool load_succeeds = true;
    std::uint32_t fail_allocation_index = 0u;
    bool file_opens = true;
    bool xml_parses = true;
    std::uint32_t first_sequence_node = 0u;
    std::uint32_t next_tree_node = 0xc000u;

    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        allocations.push_back(size);
        if (allocations.size() == fail_allocation_index) return 0u;
        const auto result = next_allocation;
        next_allocation += 0x100u;
        return result;
    }
    std::uint32_t Free(std::uint32_t pointer) override {
        frees.push_back(pointer);
        return pointer + 0x10000u;
    }
    std::uint32_t Call(
        pal4::ArenaSequenceCall operation,
        std::initializer_list<std::uint32_t> arguments) override {
        calls.push_back({operation, arguments});
        switch (operation) {
        case pal4::ArenaSequenceCall::AllocateTreeHead: {
            const auto result = next_head;
            next_head += 0x100u;
            return result;
        }
        case pal4::ArenaSequenceCall::FindSequenceNode: return found_node;
        case pal4::ArenaSequenceCall::ReadFps:
            return std::bit_cast<std::uint32_t>(24.0f);
        case pal4::ArenaSequenceCall::ReadCount: return 3u;
        case pal4::ArenaSequenceCall::ReadDir: return 0xd000u;
        case pal4::ArenaSequenceCall::ReadClumpName: return 0xd100u;
        case pal4::ArenaSequenceCall::ReadTextureName: return 0xd200u;
        case pal4::ArenaSequenceCall::ConstructClump: return *arguments.begin();
        case pal4::ArenaSequenceCall::GetAssetHost: return 0x7777u;
        case pal4::ArenaSequenceCall::LoadClump: return load_succeeds ? 1u : 0u;
        case pal4::ArenaSequenceCall::OpenSequenceFile:
            return file_opens ? 0xf000u : 0u;
        case pal4::ArenaSequenceCall::ParseXmlFile:
            return xml_parses ? 1u : 0u;
        case pal4::ArenaSequenceCall::FirstSequenceNode:
            return first_sequence_node;
        case pal4::ArenaSequenceCall::NextSequenceNode: return 0u;
        case pal4::ArenaSequenceCall::ReadSequenceLabel: return 0x9200u;
        case pal4::ArenaSequenceCall::CopyFirstSequenceChild: return 0x9300u;
        case pal4::ArenaSequenceCall::GetEmptyString: return 0xff00u;
        case pal4::ArenaSequenceCall::AllocateTreeNode: {
            const auto node = next_tree_node;
            next_tree_node += 0x100u;
            words[node + 4u] = *(arguments.begin() + 1);
            words[node + 32u] = 0u;
            return node;
        }
        default: return 0u;
        }
    }
};

void UppercaseKey() {
    Runtime runtime;
    constexpr std::uint32_t source = 0x5000u;
    constexpr std::uint32_t destination = 0x6000u;
    runtime.words[source + 4u] = 0x8000u;
    runtime.words[source + 8u] = 6u;
    const char* name = "sEq_A9";
    for (std::uint32_t i = 0u; i < 7u; ++i)
        runtime.bytes[0x8000u + i] = static_cast<std::uint8_t>(name[i]);
    Check(Resources::UppercaseKey(destination, source, runtime) ==
              destination &&
              runtime.allocations == (std::vector<std::uint32_t>{8u}) &&
              runtime.words[destination + 4u] == 0xa001u &&
              runtime.words[destination + 8u] == 6u &&
              runtime.words[destination + 12u] == 6u &&
              runtime.bytes[0xa000u] == 0u &&
              runtime.bytes[0xa001u] == 'S' &&
              runtime.bytes[0xa002u] == 'E' &&
              runtime.bytes[0xa003u] == 'Q' &&
              runtime.bytes[0xa004u] == '_' &&
              runtime.bytes[0xa005u] == 'A' &&
              runtime.bytes[0xa006u] == '9' &&
              runtime.bytes[0xa007u] == 0u,
          "uppercase key preserves punctuation and writes counted C string");
    runtime = Runtime{};
    Check(Resources::UppercaseKey(destination, source, runtime) ==
              destination && runtime.allocations.empty() &&
              runtime.words[destination + 4u] == 0u &&
              runtime.words[destination + 8u] == 0u,
          "empty source record produces empty key");
    runtime.words[source + 8u] = 3u;
    Check(Resources::UppercaseKey(destination, source, runtime) ==
              destination && runtime.allocations.empty(),
          "null source text uses empty-string fallback");
}

void ConstructionAndSharedSentinel() {
    Runtime runtime;
    std::uint32_t nil = 0u;
    std::uint32_t references = 0u;
    constexpr std::uint32_t first = 0x7e006000u;
    constexpr std::uint32_t second = 0x7d007000u;
    Check(Resources::Initialize(first, nil, references, runtime) == first &&
              nil == 0xa000u && references == 1u &&
              runtime.allocations == (std::vector<std::uint32_t>{36u}) &&
              runtime.words[0xa004u] == 0u &&
              runtime.words[0xa020u] == 1u &&
              runtime.words[0xa000u] == 0u &&
              runtime.words[0xa008u] == 0u &&
              runtime.calls == (std::vector<Runtime::Invocation>{
                  {Call::AllocateTreeHead, {first + 4u, nil, 0u}}}) &&
              runtime.bytes[first] == 0u &&
              runtime.bytes[first + 4u] == 0x7eu &&
              runtime.bytes[first + 5u] == 0x7eu &&
              runtime.bytes[first + 20u] == 0x7eu &&
              runtime.bytes[first + 36u] == 0x7eu &&
              runtime.words[first + 8u] == 0xb000u &&
              runtime.words[first + 16u] == 0u &&
              runtime.words[0xb000u] == 0xb000u &&
              runtime.words[0xb008u] == 0xb000u &&
              runtime.words[first + 24u] == 0u &&
              runtime.words[first + 40u] == 0u,
          "first constructor initializes 52-byte resource collection");
    Check(Resources::Initialize(second, nil, references, runtime) == second &&
              nil == 0xa000u && references == 2u &&
              runtime.allocations ==
                  (std::vector<std::uint32_t>{36u, 36u}) &&
              runtime.frees == (std::vector<std::uint32_t>{0xa100u}) &&
              runtime.words[second + 8u] == 0xb100u &&
              runtime.bytes[second + 4u] == 0x7du,
          "second constructor reuses shared nil and frees candidate");
}

void CleanupSharedAndResidualTree() {
    Runtime runtime;
    std::uint32_t nil = 0u;
    std::uint32_t references = 0u;
    constexpr std::uint32_t first = 0x6000u;
    constexpr std::uint32_t second = 0x7000u;
    (void)Resources::Initialize(first, nil, references, runtime);
    (void)Resources::Initialize(second, nil, references, runtime);
    runtime.calls.clear();
    runtime.frees.clear();
    runtime.words[first + 40u] = 0xd001u;
    runtime.bytes[0xd000u] = 2u;
    runtime.words[first + 24u] = 0xe000u;
    runtime.words[first + 16u] = 1u;
    runtime.words[0xb004u] = 0xf000u;
    Check(Resources::Cleanup(first, nil, references, runtime) == 1u &&
              references == 1u && nil == 0xa000u &&
              runtime.bytes[0xd000u] == 1u &&
              runtime.words[first + 40u] == 0u &&
              runtime.words[first + 24u] == 0u &&
              runtime.words[first + 8u] == 0u &&
              runtime.words[0xb004u] == nil &&
              runtime.calls == (std::vector<Runtime::Invocation>{
                  {Call::DeleteTreeSubtree, {first + 4u, 0xf000u}},
                  {Call::TreeFirstElement, {first + 4u}}}) &&
              runtime.frees ==
                  (std::vector<std::uint32_t>{0xe000u, 0xb000u}),
          "nonempty tree and counted string are cleaned in order");
    runtime.calls.clear();
    runtime.words[0xb100u] = 0xf100u;
    Check(Resources::Cleanup(second, nil, references, runtime) ==
              0x1a000u &&
              references == 0u && nil == 0u &&
              runtime.calls == (std::vector<Runtime::Invocation>{
                  {Call::EraseRemainingTreeNodes, {second + 4u,
                                                   0xb100u}}}) &&
              runtime.frees.back() == 0xa000u,
          "residual-node branch and last shared-nil release are preserved");
}

void UpdateAndRenderClumps() {
    Runtime runtime;
    constexpr std::uint32_t object = 0x6000u;
    runtime.bytes[object] = 1u;
    runtime.words[object + 24u] = 0x8000u;
    runtime.words[object + 28u] = 0x8008u;
    runtime.words[0x8000u] = 0x9000u;
    runtime.words[0x8004u] = 0x9100u;
    Resources::Update(object, 1.0 / 3.0, runtime);
    Resources::Render(object, 0x55u, runtime);
    const auto delta = std::bit_cast<std::uint32_t>(
        static_cast<float>(1.0 / 3.0));
    Check(runtime.calls == (std::vector<Runtime::Invocation>{
              {Call::UpdateAnimationTime, {0x9000u, delta}},
              {Call::UpdateAnimationTime, {0x9100u, delta}},
              {Call::RenderClump, {0x9000u, 0x55u}},
              {Call::RenderClump, {0x9100u, 0x55u}}}),
          "enabled collection updates and renders each resource in order");
    runtime.calls.clear();
    runtime.bytes[object] = 0u;
    Resources::Update(object, 2.0, runtime);
    Resources::Render(object, 0x55u, runtime);
    Check(runtime.calls.empty(), "disabled collection skips both traversals");
    pal4::ArenaSequenceRuntimeStub stub;
    Resources::Render(object, 0u, stub);
}

void LoadNamedClump() {
    Runtime runtime;
    constexpr std::uint32_t object = 0x6000u;
    constexpr std::uint32_t name = 0x7000u;
    constexpr std::uint32_t temporary = 0x8000u;
    runtime.bytes[object] = 1u;
    runtime.bytes[name] = 'f';
    runtime.bytes[name + 1u] = 'o';
    runtime.bytes[name + 2u] = 'o';
    runtime.bytes[name + 3u] = 0u;
    runtime.words[object + 8u] = 0xb000u;
    runtime.words[object + 40u] = 0xe000u;
    runtime.found_node = 0xc000u;
    runtime.words[0xc000u + 28u] = 0xc100u;
    runtime.words[0xa200u + 16u] = 0x1234u;
    Check(Resources::LoadClump(object, name, 1u, temporary, runtime) ==
              0x1234u &&
              runtime.allocations ==
                  (std::vector<std::uint32_t>{5u, 5u, 64u, 4u}) &&
              runtime.words[object + 24u] == 0xa300u &&
              runtime.words[object + 28u] == 0xa304u &&
              runtime.words[0xa300u] == 0xa200u &&
              runtime.bytes[0xa101u] == 'F' &&
              runtime.bytes[0xa102u] == 'O' &&
              runtime.bytes[0xa103u] == 'O' &&
              runtime.calls == (std::vector<Runtime::Invocation>{
                  {Call::FindSequenceNode, {object + 4u, temporary + 16u}},
                  {Call::ReadFps, {0xc100u}},
                  {Call::ReadCount, {0xc100u}},
                  {Call::ReadDir, {0xc100u}},
                  {Call::ReadClumpName, {0xc100u}},
                  {Call::ReadTextureName, {0xc100u}},
                  {Call::ConstructClump, {0xa200u}},
                  {Call::GetAssetHost, {}},
                  {Call::LoadClump,
                   {0xa200u, 0xe000u, 0xd100u, 0x7777u, 0xd200u,
                    std::bit_cast<std::uint32_t>(24.0f), 3u}}}) &&
              runtime.frees ==
                  (std::vector<std::uint32_t>{0u, 0xa100u, 0xa000u}),
          "loader resolves uppercase key and appends successful clump");

    runtime = Runtime{};
    runtime.bytes[object] = 1u;
    runtime.bytes[name] = 'x';
    runtime.bytes[name + 1u] = 0u;
    runtime.words[object + 8u] = 0xb000u;
    runtime.found_node = 0xb000u;
    Check(Resources::LoadClump(object, name, 1u, temporary, runtime) == 0u &&
              runtime.allocations ==
                  (std::vector<std::uint32_t>{3u, 3u}) &&
              runtime.calls == (std::vector<Runtime::Invocation>{
                  {Call::FindSequenceNode, {object + 4u,
                                            temporary + 16u}}}) &&
              runtime.frees ==
                  (std::vector<std::uint32_t>{0xa100u, 0xa000u}),
          "missing sequence does not allocate a clump");

    runtime = Runtime{};
    runtime.bytes[object] = 1u;
    runtime.bytes[name] = 'x';
    runtime.bytes[name + 1u] = 0u;
    runtime.words[object + 8u] = 0xb000u;
    runtime.found_node = 0xc000u;
    runtime.load_succeeds = false;
    Check(Resources::LoadClump(object, name, 1u, temporary, runtime) == 0u &&
              runtime.calls.back() == Runtime::Invocation{
                  Call::DestroyClump, {0xa200u, 1u}} &&
              runtime.words[object + 24u] == 0u,
          "failed native load destroys unretained clump");

    runtime = Runtime{};
    runtime.bytes[object] = 1u;
    runtime.bytes[name] = 'x';
    runtime.bytes[name + 1u] = 0u;
    runtime.words[object + 8u] = 0xb000u;
    runtime.found_node = 0xc000u;
    runtime.fail_allocation_index = 3u;
    Check(Resources::LoadClump(object, name, 1u, temporary, runtime) == 0u &&
              runtime.allocations.back() == 64u &&
              runtime.calls.back().operation == Call::ReadTextureName,
          "clump allocation failure skips construction and native load");

    runtime = Runtime{};
    Check(Resources::LoadClump(object, name, 1u, temporary, runtime) == 0u &&
              runtime.calls.empty() && runtime.allocations.empty(),
          "disabled collection skips lookup");
    runtime.bytes[object] = 1u;
    Check(Resources::LoadClump(object, 0u, 1u, temporary, runtime) == 0u &&
              Resources::LoadClump(object, name, 0u, temporary,
                                   runtime) == 0u &&
              runtime.calls.empty(),
          "null name and zero gate skip lookup");
}

void LoadSequenceXmlAndPath() {
    Runtime runtime;
    constexpr std::uint32_t object = 0x6000u;
    constexpr std::uint32_t source = 0x7000u;
    constexpr std::uint32_t document = 0x8000u;
    constexpr std::uint32_t temporary = 0x8100u;
    const std::string path = "C:\\Dir\\sequence.xml";
    for (std::uint32_t i = 0u; i <= path.size(); ++i)
        runtime.bytes[source + i] = static_cast<std::uint8_t>(path[i]);
    runtime.bytes[0x9200u] = 'd';
    runtime.bytes[0x9201u] = 'e';
    runtime.bytes[0x9202u] = 'm';
    runtime.bytes[0x9203u] = 'o';
    runtime.bytes[0x9204u] = 0u;
    runtime.first_sequence_node = 0x9100u;
    runtime.words[object + 8u] = 0xb000u;
    runtime.words[0xb000u] = 0xb000u;
    runtime.words[object + 40u] = 0xd001u;
    runtime.bytes[0xd000u] = 0xffu;
    Check(Resources::LoadSequenceXml(object, source, document,
                                     temporary, 0xe000u, runtime) == 1u &&
              runtime.bytes[object] == 1u &&
              runtime.allocations ==
                  (std::vector<std::uint32_t>{6u, 6u,
                                               static_cast<std::uint32_t>(path.size() + 2u),
                                               8u, 9u}) &&
              runtime.words[object + 40u] == 0xa401u &&
              runtime.words[object + 44u] == 7u &&
              runtime.bytes[0xa401u] == 'C' &&
              runtime.bytes[0xa407u] == '\\' &&
              runtime.bytes[0xa408u] == 0u &&
              runtime.bytes[0xa101u] == 'D' &&
              runtime.calls == (std::vector<Runtime::Invocation>{
                  {Call::ClearTree, {object + 4u, 0xb000u, 0xb000u}},
                  {Call::OpenSequenceFile, {source}},
                  {Call::InitXmlDocument, {document}},
                  {Call::ParseXmlFile, {0xf000u, document}},
                  {Call::CloseSequenceFile, {0xf000u}},
                  {Call::FirstSequenceNode, {document}},
                  {Call::ReadSequenceLabel, {0x9100u}},
                  {Call::CopyFirstSequenceChild, {0x9100u}},
                  {Call::InsertSequenceEntry,
                   {object + 4u, temporary + 16u, 0x9300u}},
                  {Call::NextSequenceNode, {document, 0x9100u}},
                  {Call::DestroyXmlDocument, {document}}}) &&
              runtime.frees ==
                  (std::vector<std::uint32_t>{0xd000u, 0xa100u,
                                               0xa000u, 0xa300u,
                                               0xa200u}),
          "XML load inserts uppercase labels and retains directory prefix");

    runtime = Runtime{};
    runtime.file_opens = false;
    runtime.words[object + 8u] = 0xb000u;
    runtime.words[0xb000u] = 0xb000u;
    Check(Resources::LoadSequenceXml(object, source, document,
                                     temporary, 0xe000u, runtime) == 0u &&
              runtime.calls == (std::vector<Runtime::Invocation>{
                  {Call::ClearTree, {object + 4u, 0xb000u, 0xb000u}},
                  {Call::OpenSequenceFile, {source}}}),
          "open failure stops after reset");
    runtime = Runtime{};
    runtime.xml_parses = false;
    runtime.words[object + 8u] = 0xb000u;
    runtime.words[0xb000u] = 0xb000u;
    Check(Resources::LoadSequenceXml(object, source, document,
                                     temporary, 0xe000u, runtime) == 0u &&
              runtime.calls == (std::vector<Runtime::Invocation>{
                  {Call::ClearTree, {object + 4u, 0xb000u, 0xb000u}},
                  {Call::OpenSequenceFile, {source}},
                  {Call::InitXmlDocument, {document}},
                  {Call::ParseXmlFile, {0xf000u, document}},
                  {Call::CloseSequenceFile, {0xf000u}},
                  {Call::DestroyXmlDocument, {document}}}),
          "parse failure closes file and destroys document");

    runtime = Runtime{};
    runtime.words[object + 8u] = 0xb000u;
    runtime.words[0xb000u] = 0xb000u;
    const std::string bare_name = "sequence.xml";
    for (std::uint32_t i = 0u; i <= bare_name.size(); ++i)
        runtime.bytes[source + i] =
            static_cast<std::uint8_t>(bare_name[i]);
    Check(Resources::LoadSequenceXml(object, source, document,
                                     temporary, 0xe000u, runtime) == 1u &&
              runtime.words[object + 44u] == 1u &&
              runtime.bytes[runtime.words[object + 40u]] == '\\' &&
              runtime.bytes[runtime.words[object + 40u] + 1u] == 0u,
          "source without directory uses one backslash as resource path");
}

void SubstringAndReverseFind() {
    Runtime runtime;
    constexpr std::uint32_t source = 0x5000u;
    constexpr std::uint32_t destination = 0x6000u;
    runtime.words[source + 4u] = 0x8001u;
    runtime.words[source + 8u] = 6u;
    runtime.words[source + 12u] = 6u;
    const char* value = "Ab\\CdE";
    for (std::uint32_t i = 0u; i < 7u; ++i)
        runtime.bytes[0x8001u + i] = static_cast<std::uint8_t>(value[i]);
    runtime.bytes[0x8000u] = 0u;
    Check(Resources::FindLastOf(source, "\\/", 0xffffffffu, runtime) == 2 &&
              Resources::FindLastOf(source, "\\/", 1u, runtime) == -1 &&
              Resources::FindLastOf(source, "", 0xffffffffu, runtime) == -1,
          "reverse find clamps start and handles missing character set");
    Check(Resources::Substring(destination, source, 0u, 0xffffffffu,
                               runtime) == destination &&
              runtime.words[destination + 4u] == 0x8001u &&
              runtime.words[destination + 8u] == 6u &&
              runtime.bytes[0x8000u] == 1u &&
              runtime.allocations.empty(),
          "full substring shares eligible reference-counted buffer");
    Resources::ReleaseText(destination, runtime);
    Check(runtime.bytes[0x8000u] == 0u,
          "shared substring cleanup balances reference count");
    Check(Resources::Substring(destination, source, 3u, 2u,
                               runtime) == destination &&
              runtime.allocations == (std::vector<std::uint32_t>{4u}) &&
              runtime.words[destination + 4u] == 0xa001u &&
              runtime.words[destination + 8u] == 2u &&
              runtime.bytes[0xa001u] == 'C' &&
              runtime.bytes[0xa002u] == 'd' &&
              runtime.bytes[0xa003u] == 0u,
          "partial substring allocates selected range");
    bool threw = false;
    try {
        (void)Resources::Substring(destination, source, 7u, 1u, runtime);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    Check(threw, "substring start beyond source length throws range error");
    runtime.bytes[0x8000u] = 0xfeu;
    runtime.allocations.clear();
    Check(Resources::Substring(destination, source, 0u, 6u, runtime) ==
              destination &&
              runtime.allocations == (std::vector<std::uint32_t>{8u}),
          "high reference count copies instead of sharing");
}

void ResetSequenceResources() {
    Runtime runtime;
    constexpr std::uint32_t object = 0x6000u;
    constexpr std::uint32_t head = 0xb000u;
    constexpr std::uint32_t nil = 0xe000u;
    constexpr std::uint32_t first = 0xc000u;
    constexpr std::uint32_t second = 0xd000u;
    runtime.bytes[object] = 1u;
    runtime.words[object + 8u] = head;
    runtime.words[head] = first;
    runtime.words[head + 8u] = second;
    runtime.words[first] = nil;
    runtime.words[first + 4u] = head;
    runtime.words[first + 8u] = second;
    runtime.words[first + 28u] = 0xf100u;
    runtime.words[second] = nil;
    runtime.words[second + 4u] = first;
    runtime.words[second + 8u] = nil;
    runtime.words[second + 28u] = 0xf200u;
    runtime.words[object + 24u] = 0x9000u;
    runtime.words[object + 28u] = 0x9008u;
    runtime.words[0x9000u] = 0xa100u;
    runtime.words[0x9004u] = 0xa200u;
    runtime.words[object + 40u] = 0x5001u;
    runtime.bytes[0x5000u] = 0u;
    runtime.words[object + 52u] = 0x3f000000u;
    Resources::Reset(object, nil, runtime);
    Check(runtime.bytes[object] == 0u &&
              runtime.words[object + 28u] == 0x9000u &&
              runtime.words[object + 40u] == 0u &&
              runtime.words[object + 52u] == 0u &&
              runtime.frees == (std::vector<std::uint32_t>{0x5000u}) &&
              runtime.calls == (std::vector<Runtime::Invocation>{
                  {Call::DestroyXmlNode, {0xf100u}},
                  {Call::DestroyXmlNode, {0xf200u}},
                  {Call::ClearTree, {object + 4u, first, head}},
                  {Call::CleanupClump, {0xa100u, 0x3f000000u}},
                  {Call::DestroyClump, {0xa100u, 1u}},
                  {Call::CleanupClump, {0xa200u, 0x3f000000u}},
                  {Call::DestroyClump, {0xa200u, 1u}}}),
          "reset releases XML, tree, resources, path and active time in order");
}

void ReleaseTextAndApplyEntries() {
    Runtime runtime;
    constexpr std::uint32_t record = 0x5000u;
    runtime.words[record + 4u] = 0x3001u;
    runtime.bytes[0x3000u] = 2u;
    Resources::ReleaseText(record, runtime);
    Check(runtime.bytes[0x3000u] == 1u && runtime.frees.empty() &&
              runtime.words[record + 4u] == 0u &&
              runtime.words[record + 8u] == 0u &&
              runtime.words[record + 12u] == 0u,
          "text cleanup decrements shared allocation and clears triple");
    runtime.words[record + 4u] = 0x3101u;
    runtime.bytes[0x3100u] = 0xffu;
    Resources::ReleaseText(record, runtime);
    Check(runtime.frees == (std::vector<std::uint32_t>{0x3100u}),
          "sentinel reference count frees text allocation");

    runtime = Runtime{};
    constexpr std::uint32_t object = 0x6000u;
    constexpr std::uint32_t head = 0xb000u;
    constexpr std::uint32_t nil = 0xe000u;
    constexpr std::uint32_t first = 0xc000u;
    constexpr std::uint32_t second = 0xd000u;
    runtime.bytes[object] = 1u;
    runtime.words[object + 8u] = head;
    runtime.words[head] = first;
    runtime.words[head + 8u] = second;
    runtime.words[first] = nil;
    runtime.words[first + 4u] = head;
    runtime.words[first + 8u] = second;
    runtime.words[first + 16u] = 0x9100u;
    runtime.words[second] = nil;
    runtime.words[second + 4u] = first;
    runtime.words[second + 8u] = nil;
    runtime.words[second + 16u] = 0x9200u;
    runtime.bytes[0x9100u] = 'a';
    runtime.bytes[0x9200u] = 'b';
    runtime.found_node = head;
    Resources::ApplySequenceEntries(object, 0x3f800000u, nil,
                                    0x8000u, runtime);
    Check(runtime.words[object + 52u] == 0x3f800000u &&
              std::count_if(runtime.calls.begin(), runtime.calls.end(),
                            [](const Runtime::Invocation& call) {
                                return call.operation == Call::FindSequenceNode;
                            }) == 2 &&
              runtime.calls[0] == Runtime::Invocation{
                  Call::FindSequenceNode, {object + 4u, 0x8010u}} &&
              runtime.frees.size() == 4u,
          "apply traverses both tree nodes and forwards gate bits");
    runtime.calls.clear();
    runtime.bytes[object] = 0u;
    Resources::ApplySequenceEntries(object, 0u, nil, 0x8000u, runtime);
    Check(runtime.calls.empty(), "disabled sequence collection is not applied");
}

void SequenceTreeInsertion() {
    Runtime runtime;
    constexpr std::uint32_t tree = 0x6004u;
    constexpr std::uint32_t head = 0xb000u;
    constexpr std::uint32_t nil = 0xe000u;
    constexpr std::uint32_t result = 0x9000u;
    runtime.words[tree + 4u] = head;
    runtime.bytes[tree + 8u] = 1u;
    runtime.words[head] = head;
    runtime.words[head + 4u] = nil;
    runtime.words[head + 8u] = head;
    runtime.words[nil + 32u] = 1u;

    const auto entry = [&runtime](std::uint32_t address,
                                  std::uint32_t text, const char* key) {
        runtime.words[address + 4u] = text;
        runtime.words[address + 8u] = static_cast<std::uint32_t>(
            std::char_traits<char>::length(key));
        for (std::uint32_t i = 0u; key[i] != 0; ++i)
            runtime.bytes[text + i] = static_cast<std::uint8_t>(key[i]);
    };
    entry(0x7000u, 0x7100u, "M");
    entry(0x7200u, 0x7300u, "G");
    entry(0x7400u, 0x7500u, "C");
    pal4::ArenaSequenceTree::Insert(tree, result, 0x7000u, nil, runtime);
    Check(runtime.words[result] == 0xc000u && runtime.bytes[result + 4u] == 1u &&
              runtime.words[head] == 0xc000u &&
              runtime.words[head + 4u] == 0xc000u &&
              runtime.words[head + 8u] == 0xc000u &&
              runtime.words[tree + 12u] == 1u &&
              runtime.words[0xc000u + 32u] == 1u,
          "first sequence entry becomes black root and both boundaries");
    pal4::ArenaSequenceTree::Insert(tree, result, 0x7200u, nil, runtime);
    Check(runtime.words[result] == 0xc100u &&
              runtime.words[head] == 0xc100u &&
              runtime.words[0xc000u] == 0xc100u &&
              runtime.words[0xc100u + 4u] == 0xc000u &&
              runtime.words[tree + 12u] == 2u,
          "smaller key becomes the leftmost child");
    pal4::ArenaSequenceTree::Insert(tree, result, 0x7400u, nil, runtime);
    Check(runtime.words[head + 4u] == 0xc100u &&
              runtime.words[head] == 0xc200u &&
              runtime.words[0xc100u] == 0xc200u &&
              runtime.words[0xc100u + 8u] == 0xc000u &&
              runtime.words[0xc100u + 32u] == 1u &&
              runtime.words[0xc000u + 32u] == 0u &&
              runtime.words[tree + 12u] == 3u &&
              runtime.words[0xc200u + 4u] == 0xc100u &&
              runtime.words[0xc000u + 4u] == 0xc100u,
          "left-left insertion recolors and rotates at the root");
}

void SequenceTreeDuplicateAndOrdinaryInsert() {
    Runtime runtime;
    constexpr std::uint32_t tree = 0x6004u;
    constexpr std::uint32_t head = 0xb000u;
    constexpr std::uint32_t root = 0xc000u;
    constexpr std::uint32_t nil = 0xe000u;
    constexpr std::uint32_t result = 0x9000u;
    runtime.words[tree + 4u] = head;
    runtime.words[head] = root;
    runtime.words[head + 4u] = root;
    runtime.words[head + 8u] = root;
    runtime.words[tree + 12u] = 1u;
    runtime.words[root] = nil;
    runtime.words[root + 4u] = head;
    runtime.words[root + 8u] = nil;
    runtime.words[root + 32u] = 1u;
    runtime.words[nil + 32u] = 1u;
    runtime.words[root + 16u] = 0x8100u;
    runtime.words[root + 20u] = 1u;
    runtime.bytes[0x8100u] = 'M';
    runtime.words[0x7004u] = 0x8200u;
    runtime.words[0x7008u] = 1u;
    runtime.bytes[0x8200u] = 'M';
    pal4::ArenaSequenceTree::Insert(tree, result, 0x7000u, nil, runtime);
    Check(runtime.words[result] == root && runtime.bytes[result + 4u] == 0u &&
              runtime.calls.empty(),
          "duplicate key returns existing node without allocating");

    runtime.bytes[0x8200u] = 'Z';
    pal4::ArenaSequenceTree::Insert(tree, result, 0x7000u, nil, runtime);
    Check(runtime.words[result] == 0xa000u &&
              runtime.bytes[result + 4u] == 1u &&
              runtime.words[root + 8u] == 0xa000u &&
              runtime.words[head + 8u] == 0xa000u &&
              runtime.words[tree + 12u] == 2u &&
              runtime.allocations == (std::vector<std::uint32_t>{36u}) &&
              runtime.words[0xa010u] == 0x8200u &&
              runtime.bytes[0x81ffu] == 1u,
          "ordinary insertion allocates and links a 36-byte node");
}

void SequenceTreeBalancingVariants() {
    constexpr std::uint32_t tree = 0x6004u;
    constexpr std::uint32_t head = 0xb000u;
    constexpr std::uint32_t nil = 0xe000u;
    constexpr std::uint32_t result = 0x9000u;
    const auto run = [](std::string_view keys) {
        Runtime runtime;
        runtime.words[tree + 4u] = head;
        runtime.bytes[tree + 8u] = 1u;
        runtime.words[head] = head;
        runtime.words[head + 4u] = nil;
        runtime.words[head + 8u] = head;
        runtime.words[nil + 32u] = 1u;
        for (std::uint32_t i = 0u; i < keys.size(); ++i) {
            const auto entry = 0x7000u + i * 0x20u;
            const auto text = 0x8000u + i * 0x20u;
            runtime.words[entry + 4u] = text;
            runtime.words[entry + 8u] = 1u;
            runtime.bytes[text] = static_cast<std::uint8_t>(keys[i]);
            pal4::ArenaSequenceTree::Insert(tree, result, entry, nil, runtime);
        }
        return runtime;
    };
    const auto right_right = run("MTZ");
    Check(right_right.words.at(head + 4u) == 0xc100u &&
              right_right.words.at(head + 8u) == 0xc200u &&
              right_right.words.at(0xc100u) == 0xc000u &&
              right_right.words.at(0xc100u + 8u) == 0xc200u &&
              right_right.words.at(0xc100u + 32u) == 1u &&
              right_right.words.at(0xc000u + 4u) == 0xc100u &&
              right_right.words.at(0xc200u + 4u) == 0xc100u,
          "right-right insertion performs one left rotation");

    const auto left_right = run("MCG");
    Check(left_right.words.at(head + 4u) == 0xc200u &&
              left_right.words.at(head) == 0xc100u &&
              left_right.words.at(head + 8u) == 0xc000u &&
              left_right.words.at(0xc200u) == 0xc100u &&
              left_right.words.at(0xc200u + 8u) == 0xc000u &&
              left_right.words.at(0xc100u + 4u) == 0xc200u &&
              left_right.words.at(0xc000u + 4u) == 0xc200u,
          "left-right insertion performs two rotations");

    const auto recolored = run("MCTA");
    Check(recolored.words.at(head + 4u) == 0xc000u &&
              recolored.words.at(0xc000u + 32u) == 1u &&
              recolored.words.at(0xc100u + 32u) == 1u &&
              recolored.words.at(0xc200u + 32u) == 1u &&
              recolored.words.at(0xc300u + 32u) == 0u &&
              recolored.words.at(tree + 12u) == 4u,
          "red uncle recolors both children and preserves a black root");
}

void SequenceTreeFindAndNext() {
    Runtime runtime;
    constexpr std::uint32_t tree = 0x6004u;
    constexpr std::uint32_t head = 0xb000u;
    constexpr std::uint32_t root = 0xc000u;
    constexpr std::uint32_t left = 0xc100u;
    constexpr std::uint32_t right = 0xc200u;
    constexpr std::uint32_t nil = 0xe000u;
    constexpr std::uint32_t result = 0x9000u;
    runtime.words[tree + 4u] = head;
    runtime.words[head + 4u] = root;
    runtime.words[head] = left;
    runtime.words[head + 8u] = right;
    runtime.words[root] = left;
    runtime.words[root + 4u] = head;
    runtime.words[root + 8u] = right;
    runtime.words[root + 32u] = 1u;
    runtime.words[left] = nil;
    runtime.words[left + 4u] = root;
    runtime.words[left + 8u] = nil;
    runtime.words[right] = nil;
    runtime.words[right + 4u] = root;
    runtime.words[right + 8u] = nil;
    for (const auto [node, character] :
         {std::pair{left, 'C'}, {root, 'M'}, {right, 'T'}}) {
        runtime.words[node + 16u] = node + 0x40u;
        runtime.words[node + 20u] = 1u;
        runtime.bytes[node + 0x40u] = static_cast<std::uint8_t>(character);
    }
    runtime.words[0x7004u] = 0x7100u;
    runtime.words[0x7008u] = 1u;
    runtime.bytes[0x7100u] = 'M';
    pal4::ArenaSequenceTree::FindExact(tree, result, 0x7000u, nil, runtime);
    Check(runtime.words[result] == root, "exact sequence key finds its node");
    runtime.bytes[0x7100u] = 'N';
    pal4::ArenaSequenceTree::FindExact(tree, result, 0x7000u, nil, runtime);
    Check(runtime.words[result] == head,
          "lower bound with a distinct key does not count as a match");
    runtime.words[result] = root;
    pal4::ArenaSequenceTree::Next(result, nil, runtime);
    Check(runtime.words[result] == right,
          "successor descends into the right subtree");
    Check(pal4::ArenaSequenceTree::MinimumFromLink(root + 8u, nil,
                                                    runtime) == right &&
              pal4::ArenaSequenceTree::MinimumFromLink(left + 8u, nil,
                                                        runtime) == left + 8u,
          "minimum helper returns a child or its empty input link");
    runtime.words[result] = root;
    pal4::ArenaSequenceTree::NextAndCapture(result, 0x9008u, nil, runtime);
    Check(runtime.words[result] == right && runtime.words[0x9008u] == root,
          "iterator helper preserves the original node while advancing");
    runtime.words[result] = left;
    pal4::ArenaSequenceTree::Next(result, nil, runtime);
    Check(runtime.words[result] == root,
          "successor climbs to the first left-ancestor");
    runtime.words[result] = right;
    pal4::ArenaSequenceTree::Next(result, nil, runtime);
    Check(runtime.words[result] == head,
          "successor of greatest node is the tree head");
    runtime.words[result] = head;
    pal4::ArenaSequenceTree::Previous(result, nil, runtime);
    Check(runtime.words[result] == right,
          "predecessor of head is the greatest node");
    runtime.words[result] = root;
    pal4::ArenaSequenceTree::Previous(result, nil, runtime);
    Check(runtime.words[result] == left,
          "predecessor descends to rightmost node of left subtree");
    runtime.words[result] = right;
    pal4::ArenaSequenceTree::Previous(result, nil, runtime);
    Check(runtime.words[result] == root,
          "predecessor climbs to the first right-ancestor");
}

void SequenceTreeEraseRanges() {
    constexpr std::uint32_t tree = 0x6004u;
    constexpr std::uint32_t head = 0xb000u;
    constexpr std::uint32_t root = 0xc000u;
    constexpr std::uint32_t left = 0xc100u;
    constexpr std::uint32_t nil = 0xe000u;
    constexpr std::uint32_t result = 0x9000u;
    const auto make_tree = [] {
        Runtime runtime;
        runtime.words[tree + 4u] = head;
        runtime.words[tree + 12u] = 2u;
        runtime.words[head] = left;
        runtime.words[head + 4u] = root;
        runtime.words[head + 8u] = root;
        runtime.words[root] = left;
        runtime.words[root + 4u] = head;
        runtime.words[root + 8u] = nil;
        runtime.words[left] = nil;
        runtime.words[left + 4u] = root;
        runtime.words[left + 8u] = nil;
        runtime.words[root + 16u] = 0x5001u;
        runtime.words[left + 16u] = 0x5101u;
        runtime.bytes[0x5000u] = 0u;
        runtime.bytes[0x5100u] = 0u;
        return runtime;
    };
    auto partial = make_tree();
    pal4::ArenaSequenceTree::EraseRange(tree, result, left, root, nil, partial);
    Check(partial.words[result] == root && partial.words[tree + 12u] == 1u &&
              partial.words[head] == root && partial.words[root] == nil &&
              partial.frees ==
                  (std::vector<std::uint32_t>{0x5100u, left}),
          "partial range advances, removes the node and updates the minimum");
    auto runtime = make_tree();
    pal4::ArenaSequenceTree::EraseRange(tree, result, left, head, nil, runtime);
    Check(runtime.words[result] == head && runtime.words[tree + 12u] == 0u &&
              runtime.words[head] == head &&
              runtime.words[head + 4u] == nil &&
              runtime.words[head + 8u] == head &&
              runtime.frees ==
                  (std::vector<std::uint32_t>{0x5000u, root, 0x5100u, left}) &&
              runtime.calls.empty(),
          "whole-tree range releases right branches and left spine");
}

void SequenceTreeDeleteSubtree() {
    Runtime runtime;
    constexpr std::uint32_t root = 0xc000u;
    constexpr std::uint32_t left = 0xc100u;
    constexpr std::uint32_t right = 0xc200u;
    constexpr std::uint32_t inner = 0xc300u;
    constexpr std::uint32_t nil = 0xe000u;
    runtime.words[root] = left;
    runtime.words[root + 8u] = right;
    runtime.words[left] = nil;
    runtime.words[left + 8u] = nil;
    runtime.words[right] = inner;
    runtime.words[right + 8u] = nil;
    runtime.words[inner] = nil;
    runtime.words[inner + 8u] = nil;
    Check(pal4::ArenaSequenceTree::DeleteSubtree(root, nil, runtime) == nil &&
              runtime.frees ==
                  (std::vector<std::uint32_t>{right, inner, root, left}),
          "subtree destruction recursively releases right before left spine");
    runtime.frees.clear();
    Check(pal4::ArenaSequenceTree::DeleteSubtree(nil, nil, runtime) == nil &&
              runtime.frees.empty(),
          "empty subtree does not release nodes");
}

void SequenceTreeCopyEntry() {
    Runtime runtime;
    constexpr std::uint32_t source = 0x7000u;
    constexpr std::uint32_t destination = 0x6000u;
    runtime.bytes[source] = 0x35u;
    runtime.words[source + 4u] = 0x8001u;
    runtime.words[source + 8u] = 3u;
    runtime.words[source + 12u] = 8u;
    runtime.words[source + 16u] = 0x9000u;
    runtime.bytes[0x8000u] = 1u;
    runtime.bytes[0x8001u] = 'K';
    runtime.bytes[0x8002u] = 'E';
    runtime.bytes[0x8003u] = 'Y';
    pal4::ArenaSequenceTree::CopyEntry(destination, source, runtime);
    Check(runtime.bytes[destination] == 0x35u &&
              runtime.words[destination + 4u] == 0x8001u &&
              runtime.words[destination + 8u] == 3u &&
              runtime.words[destination + 12u] == 8u &&
              runtime.words[destination + 16u] == 0x9000u &&
              runtime.bytes[0x8000u] == 2u && runtime.allocations.empty(),
          "eligible key buffer is shared with its XML child pointer");

    runtime.bytes[0x8000u] = 0xfeu;
    runtime.allocations.clear();
    pal4::ArenaSequenceTree::CopyEntry(destination + 0x20u, source, runtime);
    Check(runtime.allocations == (std::vector<std::uint32_t>{5u}) &&
              runtime.words[destination + 0x24u] == 0xa001u &&
              runtime.words[destination + 0x28u] == 3u &&
              runtime.words[destination + 0x30u] == 0x9000u &&
              runtime.bytes[0xa001u] == 'K' &&
              runtime.bytes[0xa003u] == 'Y' &&
              runtime.bytes[0xa004u] == 0u,
          "saturated key reference count causes a deep copy");

    runtime.words[source + 8u] = 0u;
    runtime.allocations.clear();
    pal4::ArenaSequenceTree::CopyEntry(destination + 0x40u, source, runtime);
    Check(runtime.words[destination + 0x44u] == 0u &&
              runtime.words[destination + 0x50u] == 0x9000u &&
              runtime.allocations.empty(),
          "empty key keeps an empty text record and the XML child");
    pal4::ArenaSequenceTree::CopyEntry(0u, source, runtime);
    Check(runtime.allocations.empty(), "null destination is ignored");
}

void SequenceTreeMixedOperations() {
    Runtime runtime;
    constexpr std::uint32_t tree = 0x6004u;
    constexpr std::uint32_t head = 0xb000u;
    constexpr std::uint32_t nil = 0xe000u;
    constexpr std::uint32_t result = 0x9000u;
    runtime.words[tree + 4u] = head;
    runtime.bytes[tree + 8u] = 1u;
    runtime.words[head] = head;
    runtime.words[head + 4u] = nil;
    runtime.words[head + 8u] = head;
    runtime.words[nil + 32u] = 1u;
    const auto check_tree = [&runtime] {
        const auto root = runtime.words[head + 4u];
        const auto count = runtime.words[tree + 12u];
        if (root == nil) {
            Check(count == 0u && runtime.words[head] == head &&
                      runtime.words[head + 8u] == head,
                  "empty tree has head boundaries and zero count");
            return;
        }
        Check(runtime.words[root + 4u] == head &&
                  runtime.words[root + 32u] == 1u,
              "red-black root is black and owned by head");
        std::set<std::uint32_t> visited;
        std::string ordered;
        std::function<int(std::uint32_t, std::uint32_t)> visit =
            [&](std::uint32_t node, std::uint32_t parent) -> int {
            if (node == nil) return 1;
            Check(visited.insert(node).second &&
                      runtime.words[node + 4u] == parent,
                  "tree has acyclic parent links");
            const auto left = runtime.words[node];
            const auto right = runtime.words[node + 8u];
            if (runtime.words[node + 32u] == 0u)
                Check(runtime.words[left + 32u] == 1u &&
                          runtime.words[right + 32u] == 1u,
                      "red node has black children");
            const auto left_height = visit(left, node);
            ordered.push_back(static_cast<char>(
                runtime.bytes[runtime.words[node + 16u]]));
            const auto right_height = visit(right, node);
            Check(left_height == right_height,
                  "all root-to-nil paths have equal black height");
            return left_height + (runtime.words[node + 32u] == 1u ? 1 : 0);
        };
        (void)visit(root, head);
        Check(visited.size() == count &&
                  std::is_sorted(ordered.begin(), ordered.end()),
              "tree count and in-order key ordering agree");
        auto minimum = root;
        while (runtime.words[minimum] != nil)
            minimum = runtime.words[minimum];
        auto maximum = root;
        while (runtime.words[maximum + 8u] != nil)
            maximum = runtime.words[maximum + 8u];
        Check(runtime.words[head] == minimum &&
                  runtime.words[head + 8u] == maximum,
              "head tracks both extreme nodes");
    };
    const std::string insertion = "FCIBADHEJG";
    for (std::uint32_t index = 0u; index < insertion.size(); ++index) {
        const auto entry = 0x7000u + index * 0x20u;
        const auto text = 0x8000u + index * 0x20u;
        runtime.words[entry + 4u] = text;
        runtime.words[entry + 8u] = 1u;
        runtime.bytes[text] = static_cast<std::uint8_t>(insertion[index]);
        pal4::ArenaSequenceTree::Insert(tree, result, entry, nil, runtime);
        check_tree();
    }
    const std::string removal = "FCIBADHEJG";
    for (const auto character : removal) {
        runtime.words[0x9004u] = 0x9100u;
        runtime.words[0x9008u] = 1u;
        runtime.bytes[0x9100u] = static_cast<std::uint8_t>(character);
        pal4::ArenaSequenceTree::FindExact(tree, result, 0x9000u,
                                            nil, runtime);
        const auto node = runtime.words[result];
        Check(node != head, "inserted key remains findable before deletion");
        pal4::ArenaSequenceTree::EraseNode(tree, result, node, nil, runtime);
        check_tree();
    }
}

void SequenceTreeEraseNode() {
    constexpr std::uint32_t tree = 0x6004u;
    constexpr std::uint32_t head = 0xb000u;
    constexpr std::uint32_t nil = 0xe000u;
    constexpr std::uint32_t result = 0x9000u;
    const auto make_tree = [](std::string_view keys) {
        Runtime runtime;
        runtime.words[tree + 4u] = head;
        runtime.bytes[tree + 8u] = 1u;
        runtime.words[head] = head;
        runtime.words[head + 4u] = nil;
        runtime.words[head + 8u] = head;
        runtime.words[nil + 32u] = 1u;
        for (std::uint32_t i = 0u; i < keys.size(); ++i) {
            const auto entry = 0x7000u + i * 0x20u;
            const auto text = 0x8000u + i * 0x20u;
            runtime.words[entry + 4u] = text;
            runtime.words[entry + 8u] = 1u;
            runtime.bytes[text] = static_cast<std::uint8_t>(keys[i]);
            pal4::ArenaSequenceTree::Insert(tree, result, entry, nil, runtime);
        }
        runtime.calls.clear();
        return runtime;
    };
    auto singleton = make_tree("M");
    pal4::ArenaSequenceTree::EraseNode(tree, result, 0xc000u, nil, singleton);
    Check(singleton.words[result] == head &&
              singleton.words[head + 4u] == nil &&
              singleton.words[head] == head &&
              singleton.words[head + 8u] == head &&
              singleton.words[tree + 12u] == 0u &&
              singleton.frees == (std::vector<std::uint32_t>{0xc000u}) &&
              singleton.bytes[0x7fffu] == 0u,
          "deleting the only black node restores empty-tree boundaries");

    auto two_children = make_tree("MCX");
    pal4::ArenaSequenceTree::EraseNode(tree, result, 0xc000u, nil,
                                       two_children);
    Check(two_children.words[result] == 0xc200u &&
              two_children.words[head + 4u] == 0xc200u &&
              two_children.words[head] == 0xc100u &&
              two_children.words[head + 8u] == 0xc200u &&
              two_children.words[0xc200u] == 0xc100u &&
              two_children.words[0xc100u + 4u] == 0xc200u &&
              two_children.words[0xc200u + 32u] == 1u &&
              two_children.words[tree + 12u] == 2u &&
              two_children.frees ==
                  (std::vector<std::uint32_t>{0xc000u}) &&
              two_children.bytes[0x7fffu] == 0u,
          "two-child deletion moves successor and preserves its node payload");

    auto black_leaf = make_tree("MCTA");
    pal4::ArenaSequenceTree::EraseNode(tree, result, 0xc200u, nil,
                                       black_leaf);
    Check(black_leaf.words[result] == head &&
              black_leaf.words[head + 4u] == 0xc100u &&
              black_leaf.words[head] == 0xc300u &&
              black_leaf.words[head + 8u] == 0xc000u &&
              black_leaf.words[0xc100u] == 0xc300u &&
              black_leaf.words[0xc100u + 8u] == 0xc000u &&
              black_leaf.words[0xc000u + 4u] == 0xc100u &&
              black_leaf.words[tree + 12u] == 3u &&
              black_leaf.words[0xc100u + 32u] == 1u &&
              black_leaf.words[0xc000u + 32u] == 1u,
          "black right leaf deletion repairs color with mirror rotation");
}

}  // namespace

int main() {
    try {
        UppercaseKey();
        ConstructionAndSharedSentinel();
        CleanupSharedAndResidualTree();
        UpdateAndRenderClumps();
        LoadNamedClump();
        LoadSequenceXmlAndPath();
        ReleaseTextAndApplyEntries();
        ResetSequenceResources();
        SubstringAndReverseFind();
        SequenceTreeInsertion();
        SequenceTreeDuplicateAndOrdinaryInsert();
        SequenceTreeBalancingVariants();
        SequenceTreeFindAndNext();
        SequenceTreeEraseRanges();
        SequenceTreeDeleteSubtree();
        SequenceTreeCopyEntry();
        SequenceTreeMixedOperations();
        SequenceTreeEraseNode();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
