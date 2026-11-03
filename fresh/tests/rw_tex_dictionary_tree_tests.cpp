#include "pal4/rw_tex_dictionary_tree.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IRwTexDictionaryTreeRuntime {
public:
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 40u, "tree nodes use 40 bytes");
        const auto address = next;
        next += 0x100u;
        events.push_back("allocate:" + std::to_string(address));
        return address;
    }
    std::uint32_t Free(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
        return 0xfaceu;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t SharedNil() override { return nil; }
    void SetSharedNil(std::uint32_t address) override { nil = address; }
    std::uint32_t Users() override { return users; }
    void SetUsers(std::uint32_t count) override { users = count; }
    void FirstElement(pal4::RwTexDictionaryTreeState&) override {
        events.push_back("first");
    }
    void DestroyDictionary(std::uint32_t dictionary) override {
        events.push_back("dictionary:" + std::to_string(dictionary));
    }

    std::uint32_t next{0x1000u};
    std::uint32_t nil{};
    std::uint32_t users{};
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;
};

class InsertRuntime final : public pal4::IRwTexDictionaryInsertRuntime {
public:
    void InitializeName(pal4::RwTexDictionaryName& name,
                        std::uint8_t tag) override {
        events.push_back("init");
        name = {tag, 0u, 0u};
    }
    bool AllocateName(pal4::RwTexDictionaryName& name,
                      std::uint32_t length, bool terminate) override {
        Check(terminate && length == expected_length,
              "name allocation receives strlen and terminator flag");
        events.push_back("allocate");
        name.buffer = allocate ? 0x5000u : 0u;
        return allocate;
    }
    void CopyName(pal4::RwTexDictionaryName&, const char* source,
                  std::uint32_t length) override {
        Check(std::string(source, length) == expected_name,
              "name copy uses exact bytes without terminator");
        events.push_back("copy");
    }
    void SetNameSize(pal4::RwTexDictionaryName& name,
                     std::uint32_t length) override {
        name.size = length;
        events.push_back("size");
    }
    void SetExceptionMessage(pal4::RwTexDictionaryName& name,
                             const char* source,
                             std::uint32_t length) override {
        Check(std::string(source, length) == expected_name &&
                  length == expected_length,
              "existence lookup receives complete source text");
        name.buffer = 0x5000u;
        name.size = length;
        events.push_back("message");
    }
    int CompareRange(std::uint32_t node_length,
                     std::uint32_t name_buffer,
                     std::uint32_t name_length) override {
        Check(node_length == 4u && name_buffer == 0x5000u &&
                  name_length == 4u,
              "candidate lookup compares node length and query bytes");
        events.push_back("range");
        return range_comparison;
    }
    int CompareNodeName(const pal4::RwTexDictionaryName& name,
                        std::uint32_t node_name_address) override {
        Check(name.tag == expected_tag &&
                  node_name_address == candidate + 12u,
              "candidate name is compared at node offset 12");
        events.push_back("compare");
        return comparison;
    }
    int CompareNodeAgainstName(
        std::uint32_t node_name_address,
        const pal4::RwTexDictionaryName& name) override {
        Check(node_name_address == expected_search_node + 12u &&
                  name.tag == expected_tag,
              "tree descent compares node name against lookup name");
        events.push_back("compare node");
        return search_comparison;
    }
    int CompareBytes(std::uint32_t left_buffer,
                     std::uint32_t right_buffer,
                     std::uint32_t length) override {
        if (left_buffer == 0x6000u) {
            Check(right_buffer == expected_query_buffer &&
                      length == expected_common_length,
                  "existence lookup compares node bytes before query bytes");
            events.push_back("node bytes");
            return node_byte_comparison;
        }
        Check(left_buffer == (allocate ? 0x5000u : 0u) &&
                  right_buffer == 0x6000u &&
                  length == expected_common_length,
              "final lookup compares common name bytes");
        events.push_back("bytes");
        return byte_comparison;
    }
    void SetCurrentDictionary(std::uint32_t dictionary) override {
        Check(dictionary == 0x7000u && tree_for_current != nullptr &&
                  tree_for_current->words.at(candidate + 32u) ==
                      expected_current_count,
              "usage count increments before RenderWare set-current call");
        events.push_back("set current");
    }
    void AssignName(pal4::RwTexDictionaryName& target,
                    const pal4::RwTexDictionaryName& source,
                    std::uint32_t start,
                    std::uint32_t count) override {
        Check(start == 0u && count == 0xffffffffu &&
                  target.tag == source.tag,
              "inserted name copies the entire lookup name");
        target.buffer = 0x6000u;
        target.size = source.size;
        target.capacity = source.capacity;
        events.push_back("assign");
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        events.push_back(address == 0x5fffu ? "release copy" :
                         "release lookup");
        return bytes[address];
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t FreeNameBuffer(std::uint32_t address) override {
        freed.push_back(address);
        return free_result;
    }

    std::string expected_name{"name"};
    std::uint32_t expected_length{4u};
    std::uint32_t expected_common_length{4u};
    std::uint32_t expected_query_buffer{0x5000u};
    std::uint8_t expected_tag{};
    std::uint32_t expected_search_node{0x3000u};
    int search_comparison{};
    int byte_comparison{};
    int node_byte_comparison{};
    std::uint32_t candidate{0x1000u};
    int comparison{};
    int range_comparison{};
    Runtime* tree_for_current{};
    std::uint32_t expected_current_count{};
    bool allocate{true};
    std::uint32_t free_result{0xfaceu};
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> freed;
    std::vector<std::string> events;
};

class NodeInsertRuntime final : public pal4::IRwTexDictionaryNodeInsertRuntime {
public:
    std::uint32_t SharedNil() override { return nil; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    int CompareRange(const pal4::RwTexDictionaryName&,
                     std::uint32_t node_buffer,
                     std::uint32_t node_length) override {
        Check(node_buffer == 0x6000u && node_length == 4u,
              "search compares node name buffer and length");
        events.push_back("range");
        return range_result;
    }
    int CompareNameToNode(const pal4::RwTexDictionaryName&,
                          std::uint32_t node_name_address) override {
        Check(node_name_address == compare_parent + 12u,
              "direct insertion compares new name with parent name");
        events.push_back("name to node");
        return name_to_node;
    }
    int CompareNodeToName(std::uint32_t node_name_address,
                          const pal4::RwTexDictionaryName&) override {
        Check(node_name_address == compare_parent + 12u,
              "checked insertion compares predecessor name");
        events.push_back("node to name");
        return node_to_name;
    }
    std::uint32_t AllocateNode(std::uint32_t bytes) override {
        Check(bytes == 40u, "created node uses 40 bytes");
        events.push_back("create");
        return created;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t EmptyNameBuffer() override { return empty_buffer; }
    void SetNameErrorFlag(std::uint32_t name,
                          std::uint8_t flag) override {
        Check(flag == 1u && name != 0u,
              "shared node name sets its ownership flag");
        events.push_back("copy node");
    }
    bool AllocateNameBuffer(std::uint32_t name,
                            std::uint32_t length,
                            bool terminate) override {
        Check(terminate && name != 0u,
              "node name allocation receives terminator flag");
        events.push_back("allocate name");
        if (!allocate_name) return false;
        words[name + 4u] = copied_buffer;
        words[name + 12u] = length;
        return true;
    }
    void CopyNameBytes(std::uint32_t destination,
                       std::uint32_t source,
                       std::uint32_t length) override {
        events.push_back("copy bytes");
        for (std::uint32_t i = 0u; i < length; ++i)
            bytes[destination + i] = bytes[source + i];
    }
    void DetachNameBuffer(std::uint32_t name) override {
        Check(name != 0u, "in-place copy detaches name record");
        events.push_back("detach");
    }
    void InitializeHead() {
        words[head] = head;
        words[head + 4u] = nil;
        words[head + 8u] = head;
        words[nil + 36u] = 1u;
    }
    void SetRoot(std::uint32_t root) {
        words[head] = root;
        words[head + 4u] = root;
        words[head + 8u] = root;
        words[root] = nil;
        words[root + 4u] = head;
        words[root + 8u] = nil;
        words[root + 16u] = 0x6000u;
        words[root + 20u] = 4u;
        words[root + 36u] = 1u;
    }

    std::uint32_t nil{0x1000u};
    std::uint32_t head{0x2000u};
    std::uint32_t created{0x4000u};
    std::uint32_t compare_parent{0x3000u};
    std::uint32_t empty_buffer{0x9001u};
    std::uint32_t copied_buffer{0x7001u};
    bool allocate_name{true};
    int range_result{-1};
    int name_to_node{-1};
    int node_to_name{-1};
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;
};

void ConstructAndShareNil() {
    Runtime runtime;
    pal4::RwTexDictionaryTreeState first{0xabu, 0xcdu, 3u, 4u,
                                          0u, 9u, 5u, 6u, 7u, 99u};
    Check(pal4::RwTexDictionaryTree::Construct(first, 1u, 2u, runtime) ==
              &first, "constructor returns this");
    Check(first.tag == 1u && first.second == 2u && first.flag == 0u &&
              first.count == 0u && first.head == 0x1100u,
          "constructor initializes observed fields");
    Check(first.unused2 == 3u && first.unused3 == 4u &&
              first.unused9 == 5u && first.unused10 == 6u &&
              first.unused11 == 7u, "constructor preserves other bytes");
    Check(runtime.nil == 0x1000u && runtime.users == 1u &&
              runtime.words.at(0x1004u) == 0u &&
              runtime.words.at(0x1024u) == 1u &&
              runtime.words.at(0x1000u) == 0u &&
              runtime.words.at(0x1008u) == 0u,
          "first instance creates shared nil");
    Check(runtime.words.at(0x1100u) == 0x1100u &&
              runtime.words.at(0x1104u) == 0x1000u &&
              runtime.words.at(0x1108u) == 0x1100u &&
              runtime.words.at(0x1124u) == 0u,
          "head links to itself and nil");

    pal4::RwTexDictionaryTreeState second;
    (void)pal4::RwTexDictionaryTree::Construct(second, 8u, 9u, runtime);
    Check(second.head == 0x1300u && runtime.nil == 0x1000u &&
              runtime.users == 2u &&
              runtime.events[2] == "allocate:4608" &&
              runtime.events[3] == "free:4608",
          "second instance discards candidate nil");
    Check(pal4::RwTexDictionaryTree::Destruct(first, runtime) == 1u &&
              first.head == 0u && runtime.nil == 0x1000u,
          "nonfinal destructor retains shared nil");
    Check(pal4::RwTexDictionaryTree::Destruct(second, runtime) == 0xfaceu &&
              runtime.nil == 0u && runtime.users == 0u,
          "final destructor releases shared nil and forwards free result");
    Check(runtime.events.back() == "free:4096",
          "shared nil is freed last");
}

void RecursiveAndRemovalPaths() {
    Runtime runtime;
    pal4::RwTexDictionaryTreeState state;
    (void)pal4::RwTexDictionaryTree::Construct(state, 0u, 0u, runtime);
    runtime.events.clear();
    state.count = 2u;
    runtime.words[state.head + 4u] = 0x3000u;
    runtime.words[0x3000u] = runtime.nil;
    runtime.words[0x3008u] = runtime.nil;
    runtime.words[0x3010u] = 0u;
    Check(pal4::RwTexDictionaryTree::Destruct(state, runtime) == 0xfaceu,
          "recursive destructor returns final free result");
    Check(runtime.events == std::vector<std::string>{
              "free:12288", "first", "free:4352", "free:4096"},
          "nonempty tree destroys root, resets head and frees nodes in order");

    Runtime sparse;
    pal4::RwTexDictionaryTreeState other;
    (void)pal4::RwTexDictionaryTree::Construct(other, 0u, 0u, sparse);
    sparse.events.clear();
    sparse.words[other.head] = 0x3000u;
    sparse.words[other.head + 4u] = 0x3000u;
    sparse.words[other.head + 8u] = 0x3100u;
    sparse.words[sparse.nil + 36u] = 1u;
    sparse.words[0x3000u] = sparse.nil;
    sparse.words[0x3004u] = other.head;
    sparse.words[0x3008u] = 0x3100u;
    sparse.words[0x3024u] = 1u;
    sparse.words[0x3100u] = sparse.nil;
    sparse.words[0x3104u] = 0x3000u;
    sparse.words[0x3108u] = sparse.nil;
    sparse.words[0x3124u] = 0u;
    for (const auto node : {0x3000u, 0x3100u}) {
        sparse.words[node + 16u] = 0u;
        sparse.words[node + 20u] = 0u;
        sparse.words[node + 24u] = 0u;
    }
    (void)pal4::RwTexDictionaryTree::Destruct(other, sparse);
    Check(sparse.events == std::vector<std::string>{
              "free:12288", "free:12544",
              "free:4352", "free:4096"},
          "zero-count linked nodes use rebuilt removal");
}

void CleanupEmptyAndPopulatedTrees() {
    Runtime runtime;
    pal4::RwTexDictionaryTreeState state;
    (void)pal4::RwTexDictionaryTree::Construct(state, 0u, 0u, runtime);
    runtime.events.clear();
    Check(pal4::RwTexDictionaryTree::Cleanup(state, runtime) == state.head &&
              runtime.events.empty(),
          "empty cleanup returns head without destroying dictionaries");

    const auto head = state.head;
    const auto root = 0x3000u;
    const auto right = 0x3100u;
    runtime.words[head] = root;
    runtime.words[head + 4u] = root;
    runtime.words[head + 8u] = right;
    runtime.words[root] = runtime.nil;
    runtime.words[root + 4u] = head;
    runtime.words[root + 8u] = right;
    runtime.words[root + 28u] = 0x7000u;
    runtime.words[root + 32u] = 1u;
    runtime.words[root + 16u] = 0u;
    runtime.words[right] = runtime.nil;
    runtime.words[right + 4u] = root;
    runtime.words[right + 8u] = runtime.nil;
    runtime.words[right + 28u] = 0x7100u;
    runtime.words[right + 32u] = 1u;
    runtime.words[right + 16u] = 0u;
    state.count = 2u;
    Check(pal4::RwTexDictionaryTree::Cleanup(state, runtime) == head &&
              state.count == 0u && runtime.words[head] == head &&
              runtime.words[head + 4u] == runtime.nil &&
              runtime.words[head + 8u] == head &&
              runtime.words[root + 32u] == 0u &&
              runtime.words[right + 32u] == 0u,
          "cleanup clears dictionary references and resets the head");
    Check(runtime.events == std::vector<std::string>{
              "dictionary:28672", "dictionary:28928",
              "free:12544", "free:12288"},
          "cleanup destroys dictionaries in order before tree nodes");
    Check(runtime.nil == 0x1000u && runtime.users == 1u,
          "cleanup retains shared nil and instance");
}

void CleanupZeroCountLinkedNodes() {
    Runtime runtime;
    pal4::RwTexDictionaryTreeState state;
    (void)pal4::RwTexDictionaryTree::Construct(state, 0u, 0u, runtime);
    runtime.events.clear();
    const auto head = state.head;
    const auto node = 0x3000u;
    runtime.words[head] = node;
    runtime.words[head + 4u] = node;
    runtime.words[head + 8u] = node;
    runtime.words[runtime.nil + 36u] = 1u;
    runtime.words[node] = runtime.nil;
    runtime.words[node + 4u] = head;
    runtime.words[node + 8u] = runtime.nil;
    runtime.words[node + 16u] = 0u;
    runtime.words[node + 20u] = 0u;
    runtime.words[node + 24u] = 0u;
    runtime.words[node + 28u] = 0x7000u;
    runtime.words[node + 32u] = 1u;
    runtime.words[node + 36u] = 1u;
    Check(pal4::RwTexDictionaryTree::Cleanup(state, runtime) == head &&
              runtime.words[node + 32u] == 0u,
          "zero-count cleanup still destroys dictionary and removes node");
    Check(runtime.events == std::vector<std::string>{
              "dictionary:28672", "free:12288"},
          "zero-count cleanup takes the removal path");
}

void InsertTextureBranches() {
    pal4::RwTexDictionaryTreeState state;
    state.head = 0x1000u;
    const char* name = "name";
    InsertRuntime runtime;
    NodeInsertRuntime nodes;
    nodes.head = state.head;
    nodes.nil = 0x0800u;
    nodes.compare_parent = state.head;
    nodes.InitializeHead();
    runtime.expected_tag = static_cast<std::uint8_t>(
        reinterpret_cast<std::uintptr_t>(name));
    Check(pal4::RwTexDictionaryTree::AddTexture(
              state, name, 0x7000u, runtime, nodes) &&
              nodes.events == std::vector<std::string>{
                  "create", "copy node"} && state.count == 1u &&
              nodes.words[nodes.head + 4u] == nodes.created,
          "missing name inserts texture");
    Check(runtime.events == std::vector<std::string>{
              "init", "allocate", "copy", "size", "init",
              "assign", "release copy", "release lookup"},
          "insertion follows native construction, lookup and release order");

    runtime.events.clear();
    nodes.events.clear();
    nodes.SetRoot(0x3000u);
    nodes.words[0x3000u] = nodes.nil;
    nodes.words[0x3008u] = nodes.nil;
    Check(!pal4::RwTexDictionaryTree::AddTexture(
              state, name, 0x7000u, runtime, nodes) &&
              nodes.events.empty() &&
              runtime.events == std::vector<std::string>{
                  "init", "allocate", "copy", "size",
                  "compare node", "bytes", "release lookup"},
          "existing name skips copy and insert");

    runtime.events.clear();
    nodes.InitializeHead();
    state.count = 0u;
    runtime.allocate = false;
    Check(pal4::RwTexDictionaryTree::AddTexture(
              state, name, 0x7000u, runtime, nodes) &&
              nodes.events == std::vector<std::string>{
                  "create", "allocate name", "copy bytes"} &&
              state.count == 1u &&
              runtime.events == std::vector<std::string>{
                  "init", "allocate", "init", "assign",
                  "release copy"},
          "allocation failure still reaches native lookup and insertion path");
}

void ReleaseNameReferenceCounts() {
    InsertRuntime runtime;
    pal4::RwTexDictionaryName name;
    name = {0xabu, 2u, 3u, 4u, 0x5000u, 12u, 20u};
    runtime.bytes[0x4fffu] = 3u;
    Check(pal4::RwTexDictionaryTree::ReleaseName(name, runtime) ==
              0x4fffu && runtime.bytes[0x4fffu] == 2u &&
              runtime.freed.empty(),
          "shared name decrements its reference byte");
    Check(name.tag == 0xabu && name.unused1 == 2u &&
              name.unused2 == 3u && name.unused3 == 4u &&
              name.buffer == 0u && name.size == 0u &&
              name.capacity == 0u,
          "release clears only name buffer metadata");
    Check(pal4::RwTexDictionaryTree::ReleaseName(name, runtime) == 0u &&
              runtime.freed.empty(),
          "empty name requires no allocation access");

    name.buffer = 0x5000u;
    runtime.bytes[0x4fffu] = 0u;
    Check(pal4::RwTexDictionaryTree::ReleaseName(name, runtime) ==
              0xfaceu && runtime.freed ==
              std::vector<std::uint32_t>{0x4fffu},
          "zero reference count frees allocation header");
    name.buffer = 0x5000u;
    runtime.bytes[0x4fffu] = 0xffu;
    Check(pal4::RwTexDictionaryTree::ReleaseName(name, runtime) ==
              0xfaceu && runtime.freed ==
              std::vector<std::uint32_t>{0x4fffu, 0x4fffu},
          "all-ones reference byte also frees allocation");
}

void RemoveTextureByNameBranches() {
    pal4::RwTexDictionaryTreeState state;
    state.head = 0x1000u;
    const char* name = "name";
    InsertRuntime names;
    names.expected_tag = static_cast<std::uint8_t>(
        reinterpret_cast<std::uintptr_t>(name));
    Runtime tree;
    tree.nil = 0x0800u;
    tree.words[state.head + 4u] = tree.nil;
    Check(!pal4::RwTexDictionaryTree::RemoveTextureByName(
              state, name, names, tree) && tree.events.empty() &&
              names.events == std::vector<std::string>{
                  "init", "allocate", "copy", "size",
                  "release lookup"},
          "missing node releases lookup name without tree writes");

    names.events.clear();
    tree.words[state.head + 4u] = 0x3000u;
    tree.words[0x3000u] = tree.nil;
    tree.words[0x3008u] = tree.nil;
    tree.words[0x3010u] = 0x6000u;
    tree.words[0x3014u] = 4u;
    tree.words[0x301cu] = 0u;
    Check(!pal4::RwTexDictionaryTree::RemoveTextureByName(
              state, name, names, tree) && tree.events.empty(),
          "node without dictionary returns false");

    names.events.clear();
    tree.words[0x301cu] = 0x7000u;
    tree.words[0x3020u] = 2u;
    tree.words[0x0824u] = 1u;
    tree.words[state.head] = 0x3000u;
    tree.words[state.head + 4u] = 0x3000u;
    tree.words[state.head + 8u] = 0x3000u;
    tree.words[0x3000u] = tree.nil;
    tree.words[0x3004u] = state.head;
    tree.words[0x3008u] = tree.nil;
    tree.bytes[0x5fffu] = 3u;
    tree.words[0x3018u] = 0u;
    tree.words[0x3024u] = 1u;
    state.count = 1u;
    Check(pal4::RwTexDictionaryTree::RemoveTextureByName(
              state, name, names, tree) && tree.words[0x3020u] == 1u &&
              tree.words[0x301cu] == 0x7000u && tree.events.empty(),
          "nonfinal reference keeps dictionary and node");

    names.events.clear();
    Check(pal4::RwTexDictionaryTree::RemoveTextureByName(
              state, name, names, tree) && tree.words[0x3020u] == 0u &&
              tree.words[0x301cu] == 0u &&
              state.count == 0u && tree.words[state.head] == state.head &&
              tree.events == std::vector<std::string>{
                  "dictionary:28672", "free:12288"} &&
              names.events.back() == "release lookup",
          "last reference destroys dictionary, removes node and releases name");
}

void SetCurrentCandidateBranches() {
    pal4::RwTexDictionaryTreeState state;
    state.head = 0x1000u;
    const char* name = "name";
    InsertRuntime names;
    names.expected_tag = static_cast<std::uint8_t>(
        reinterpret_cast<std::uintptr_t>(name));
    Runtime tree;
    tree.nil = 0x2000u;
    tree.words[state.head + 4u] = tree.nil;
    Check(!pal4::RwTexDictionaryTree::SetCurrent(
              state, name, names, tree) &&
              names.events == std::vector<std::string>{
                  "init", "allocate", "copy", "size",
                  "release lookup"},
          "head candidate returns false without comparison");

    names.events.clear();
    names.candidate = 0x3000u;
    tree.words[state.head + 4u] = names.candidate;
    tree.words[names.candidate] = tree.nil;
    tree.words[names.candidate + 8u] = tree.nil;
    tree.words[names.candidate + 20u] = 4u;
    names.comparison = -1;
    Check(!pal4::RwTexDictionaryTree::SetCurrent(
              state, name, names, tree) &&
              names.events == std::vector<std::string>{
                  "init", "allocate", "copy", "size", "range",
                  "compare", "release lookup"},
          "negative name comparison rejects candidate");

    names.events.clear();
    names.comparison = 0;
    tree.words[0x301cu] = 0u;
    Check(!pal4::RwTexDictionaryTree::SetCurrent(
              state, name, names, tree) &&
              names.events.back() == "release lookup",
          "candidate without dictionary returns false");

    names.events.clear();
    tree.words[0x301cu] = 0x7000u;
    tree.words[0x3020u] = 2u;
    names.tree_for_current = &tree;
    names.expected_current_count = 3u;
    Check(pal4::RwTexDictionaryTree::SetCurrent(
              state, name, names, tree) &&
              tree.words[0x3020u] == 3u &&
              names.events == std::vector<std::string>{
                  "init", "allocate", "copy", "size", "range",
                  "compare", "set current", "release lookup"},
          "valid candidate increments usage then sets current dictionary");
}

void CheckTextureExistsBranches() {
    pal4::RwTexDictionaryTreeState state;
    state.head = 0x1000u;
    const char* name = "name";
    InsertRuntime names;
    names.expected_tag = static_cast<std::uint8_t>(
        reinterpret_cast<std::uintptr_t>(name));
    Runtime tree;
    tree.nil = 0x0800u;
    tree.words[state.head + 4u] = tree.nil;
    Check(!pal4::RwTexDictionaryTree::CheckTextureExists(
              state, name, names, tree) &&
              names.events == std::vector<std::string>{
                  "init", "message", "release lookup"},
          "head result skips name comparison and returns false");

    names.events.clear();
    names.candidate = 0x3000u;
    tree.words[state.head + 4u] = names.candidate;
    tree.words[names.candidate] = tree.nil;
    tree.words[names.candidate + 8u] = tree.nil;
    tree.words[names.candidate + 16u] = 0x6000u;
    tree.words[names.candidate + 20u] = 4u;
    names.comparison = -1;
    Check(!pal4::RwTexDictionaryTree::CheckTextureExists(
              state, name, names, tree) &&
              names.events == std::vector<std::string>{
                  "init", "message", "node bytes", "compare",
                  "release lookup"},
          "negative comparison rejects candidate");

    names.events.clear();
    names.comparison = 0;
    Check(pal4::RwTexDictionaryTree::CheckTextureExists(
              state, name, names, tree) &&
              names.events == std::vector<std::string>{
                  "init", "message", "node bytes", "compare",
                  "release lookup"},
          "matching name exists without inspecting dictionary pointer");
}

void InternalInsertBranches() {
    pal4::RwTexDictionaryEntry entry;
    entry.name.buffer = 0x5000u;
    entry.name.size = 4u;
    entry.dictionary = 0x7000u;
    entry.references = 1u;

    NodeInsertRuntime checked;
    checked.InitializeHead();
    checked.compare_parent = checked.head;
    pal4::RwTexDictionaryTreeState state;
    state.head = checked.head;
    const auto initial = pal4::RwTexDictionaryTree::InsertInternal(
        state, entry, checked);
    Check(initial.node == checked.created && initial.inserted &&
              checked.events == std::vector<std::string>{
                  "create", "copy node"} && state.count == 1u &&
              checked.words[checked.head + 4u] == checked.created,
          "checked empty insertion creates and links a black root");

    checked.events.clear();
    checked.SetRoot(0x3000u);
    state.count = 1u;
    checked.compare_parent = 0x3000u;
    checked.range_result = 0;
    checked.node_to_name = 0;
    const auto duplicate = pal4::RwTexDictionaryTree::InsertInternal(
        state, entry, checked);
    Check(duplicate.node == 0x3000u && !duplicate.inserted &&
              checked.events == std::vector<std::string>{
                  "range", "node to name"},
          "checked insertion returns existing node without allocation");
    checked.events.clear();
    checked.node_to_name = -1;
    const auto inserted = pal4::RwTexDictionaryTree::InsertInternal(
        state, entry, checked);
    Check(inserted.node == checked.created && inserted.inserted &&
              checked.events == std::vector<std::string>{
                  "range", "node to name", "create", "copy node",
                  "name to node"} && state.count == 2u,
          "checked nonduplicate uses rebuilt node linkage");

    NodeInsertRuntime direct;
    direct.InitializeHead();
    state = {};
    state.head = direct.head;
    state.flag = 1u;
    const auto first = pal4::RwTexDictionaryTree::InsertInternal(
        state, entry, direct);
    Check(first.node == direct.created && first.inserted &&
              state.count == 1u &&
              direct.words[direct.head] == direct.created &&
              direct.words[direct.head + 4u] == direct.created &&
              direct.words[direct.head + 8u] == direct.created &&
              direct.words[direct.created] == direct.nil &&
              direct.words[direct.created + 8u] == direct.nil &&
              direct.words[direct.created + 36u] == 1u,
          "direct empty insertion links and colors first node");

    NodeInsertRuntime left;
    left.InitializeHead();
    left.SetRoot(0x3000u);
    state = {};
    state.head = left.head;
    state.flag = 1u;
    state.count = 1u;
    const auto left_result = pal4::RwTexDictionaryTree::InsertInternal(
        state, entry, left);
    Check(left_result.inserted && state.count == 2u &&
              left.words[0x3000u] == left.created &&
              left.words[left.head] == left.created &&
              left.words[left.head + 8u] == 0x3000u,
          "direct left insertion updates leftmost head link");

    NodeInsertRuntime right;
    right.InitializeHead();
    right.SetRoot(0x3000u);
    right.range_result = 1;
    right.name_to_node = 1;
    state = {};
    state.head = right.head;
    state.flag = 1u;
    state.count = 1u;
    const auto right_result = pal4::RwTexDictionaryTree::InsertInternal(
        state, entry, right);
    Check(right_result.inserted && right.words[0x3008u] == right.created &&
              right.words[right.head + 8u] == right.created,
          "direct right insertion updates rightmost head link");
}

void InternalInsertRotation() {
    NodeInsertRuntime runtime;
    runtime.InitializeHead();
    runtime.SetRoot(0x3000u);
    runtime.words[0x3000u] = 0x3100u;
    runtime.words[0x3100u] = runtime.nil;
    runtime.words[0x3104u] = 0x3000u;
    runtime.words[0x3108u] = runtime.nil;
    runtime.words[0x3110u] = 0x6000u;
    runtime.words[0x3114u] = 4u;
    runtime.words[0x3124u] = 0u;
    runtime.words[runtime.head] = 0x3100u;
    runtime.compare_parent = 0x3100u;
    pal4::RwTexDictionaryTreeState state;
    state.head = runtime.head;
    state.flag = 1u;
    state.count = 2u;
    pal4::RwTexDictionaryEntry entry;
    entry.name.buffer = 0x5000u;
    entry.name.size = 4u;
    entry.dictionary = 0x7000u;
    const auto result = pal4::RwTexDictionaryTree::InsertInternal(
        state, entry, runtime);
    Check(result.inserted && state.count == 3u &&
              runtime.words[runtime.head + 4u] == 0x3100u &&
              runtime.words[0x3124u] == 1u &&
              runtime.words[0x3024u] == 0u &&
              runtime.words[0x3108u] == 0x3000u &&
              runtime.words[0x3004u] == 0x3100u,
          "left-left insertion recolors and rotates the former root");
}

void InternalInsertMirrorAndRecolor() {
    pal4::RwTexDictionaryEntry entry;
    entry.name.buffer = 0x5000u;
    entry.name.size = 4u;
    entry.dictionary = 0x7000u;
    NodeInsertRuntime right;
    right.InitializeHead();
    right.SetRoot(0x3000u);
    right.words[0x3008u] = 0x3200u;
    right.words[0x3200u] = right.nil;
    right.words[0x3204u] = 0x3000u;
    right.words[0x3208u] = right.nil;
    right.words[0x3210u] = 0x6000u;
    right.words[0x3214u] = 4u;
    right.words[0x3224u] = 0u;
    right.words[right.head + 8u] = 0x3200u;
    right.compare_parent = 0x3200u;
    right.range_result = 1;
    right.name_to_node = 1;
    pal4::RwTexDictionaryTreeState state;
    state.head = right.head;
    state.flag = 1u;
    state.count = 2u;
    const auto result = pal4::RwTexDictionaryTree::InsertInternal(
        state, entry, right);
    Check(result.inserted && right.words[right.head + 4u] == 0x3200u &&
              right.words[0x3224u] == 1u &&
              right.words[0x3024u] == 0u &&
              right.words[0x3200u] == 0x3000u &&
              right.words[0x3004u] == 0x3200u,
          "right-right insertion recolors and rotates root left");

    NodeInsertRuntime recolor;
    recolor.InitializeHead();
    recolor.SetRoot(0x3000u);
    recolor.words[0x3000u] = 0x3100u;
    recolor.words[0x3008u] = 0x3200u;
    recolor.words[0x3100u] = recolor.nil;
    recolor.words[0x3104u] = 0x3000u;
    recolor.words[0x3108u] = recolor.nil;
    recolor.words[0x3110u] = 0x6000u;
    recolor.words[0x3114u] = 4u;
    recolor.words[0x3124u] = 0u;
    recolor.words[0x3224u] = 0u;
    recolor.words[recolor.head] = 0x3100u;
    recolor.compare_parent = 0x3100u;
    state = {};
    state.head = recolor.head;
    state.flag = 1u;
    state.count = 3u;
    const auto recolored = pal4::RwTexDictionaryTree::InsertInternal(
        state, entry, recolor);
    Check(recolored.inserted && state.count == 4u,
          "red-uncle insertion creates one node");
    Check(recolor.words[0x3124u] == 1u &&
              recolor.words[0x3224u] == 1u &&
              recolor.words[0x3024u] == 1u,
          "red uncle recolors both children and restores black root");
    Check(recolor.events.back() == "name to node",
          "red-uncle insertion needs no rotation");
}

void RemoveNodeStructuresAndText() {
    Runtime runtime;
    pal4::RwTexDictionaryTreeState state;
    (void)pal4::RwTexDictionaryTree::Construct(state, 0u, 0u, runtime);
    const auto head = state.head;
    const auto nil = runtime.nil;
    const auto root = 0x3000u;
    runtime.words[nil + 36u] = 1u;
    runtime.words[head] = root;
    runtime.words[head + 4u] = root;
    runtime.words[head + 8u] = root;
    runtime.words[root] = nil;
    runtime.words[root + 4u] = head;
    runtime.words[root + 8u] = nil;
    runtime.words[root + 16u] = 0x5000u;
    runtime.words[root + 20u] = 4u;
    runtime.words[root + 24u] = 8u;
    runtime.words[root + 36u] = 1u;
    runtime.bytes[0x4fffu] = 3u;
    state.count = 1u;
    runtime.events.clear();
    Check(pal4::RwTexDictionaryTree::RemoveNodeAndRebalance(
              state, root, runtime) == head &&
              state.count == 0u && runtime.words[head] == head &&
              runtime.words[head + 4u] == nil &&
              runtime.words[head + 8u] == head &&
              runtime.words[root + 16u] == 0u &&
              runtime.words[root + 20u] == 0u &&
              runtime.words[root + 24u] == 0u &&
              runtime.bytes[0x4fffu] == 2u &&
              runtime.events == std::vector<std::string>{"free:12288"},
          "single root removal resets head and releases name reference");

    runtime.words[head] = 0x3100u;
    runtime.words[head + 4u] = root;
    runtime.words[head + 8u] = root;
    runtime.words[root] = 0x3100u;
    runtime.words[root + 4u] = head;
    runtime.words[root + 8u] = nil;
    runtime.words[root + 36u] = 1u;
    runtime.words[0x3100u] = nil;
    runtime.words[0x3104u] = root;
    runtime.words[0x3108u] = nil;
    runtime.words[0x3110u] = 0u;
    runtime.words[0x3114u] = 0u;
    runtime.words[0x3118u] = 0u;
    runtime.words[0x3124u] = 0u;
    state.count = 2u;
    runtime.events.clear();
    Check(pal4::RwTexDictionaryTree::RemoveNodeAndRebalance(
              state, 0x3100u, runtime) == root &&
              state.count == 1u && runtime.words[root] == nil &&
              runtime.words[head] == root &&
              runtime.events == std::vector<std::string>{"free:12544"},
          "red left leaf removal updates leftmost head link");
}

void RemoveNodeWithSuccessor() {
    Runtime runtime;
    pal4::RwTexDictionaryTreeState state;
    (void)pal4::RwTexDictionaryTree::Construct(state, 0u, 0u, runtime);
    const auto head = state.head;
    const auto nil = runtime.nil;
    runtime.words[nil + 36u] = 1u;
    runtime.words[head] = 0x3100u;
    runtime.words[head + 4u] = 0x3000u;
    runtime.words[head + 8u] = 0x3200u;
    runtime.words[0x3000u] = 0x3100u;
    runtime.words[0x3004u] = head;
    runtime.words[0x3008u] = 0x3200u;
    runtime.words[0x3010u] = 0u;
    runtime.words[0x3014u] = 0u;
    runtime.words[0x3018u] = 0u;
    runtime.words[0x3024u] = 1u;
    for (const auto node : {0x3100u, 0x3200u}) {
        runtime.words[node] = nil;
        runtime.words[node + 4u] = 0x3000u;
        runtime.words[node + 8u] = nil;
        runtime.words[node + 16u] = 0u;
        runtime.words[node + 20u] = 0u;
        runtime.words[node + 24u] = 0u;
        runtime.words[node + 36u] = 0u;
    }
    state.count = 3u;
    runtime.events.clear();
    Check(pal4::RwTexDictionaryTree::RemoveNodeAndRebalance(
              state, 0x3000u, runtime) == 0x3200u &&
              state.count == 2u &&
              runtime.words[head + 4u] == 0x3200u &&
              runtime.words[0x3104u] == 0x3200u &&
              runtime.words[head] == 0x3100u &&
              runtime.words[head + 8u] == 0x3200u &&
              runtime.events == std::vector<std::string>{"free:12288"},
          "two-child deletion promotes successor and frees original node");
}

void FindNodeBranches() {
    Runtime tree;
    pal4::RwTexDictionaryTreeState state;
    (void)pal4::RwTexDictionaryTree::Construct(state, 0u, 0u, tree);
    InsertRuntime names;
    pal4::RwTexDictionaryName key;
    key.buffer = 0x5000u;
    key.size = 4u;
    names.expected_tag = key.tag;
    Check(pal4::RwTexDictionaryTree::FindNode(
              state, key, names, tree) == state.head &&
              names.events.empty(),
          "empty tree lookup returns head without comparing names");

    tree.words[state.head + 4u] = 0x3000u;
    tree.words[0x3000u] = tree.nil;
    tree.words[0x3008u] = tree.nil;
    tree.words[0x3010u] = 0x6000u;
    tree.words[0x3014u] = 4u;
    Check(pal4::RwTexDictionaryTree::FindNode(
              state, key, names, tree) == 0x3000u &&
              names.events == std::vector<std::string>{
                  "compare node", "bytes"},
          "equal name returns candidate node");

    names.events.clear();
    names.search_comparison = -1;
    Check(pal4::RwTexDictionaryTree::FindNode(
              state, key, names, tree) == state.head &&
              names.events == std::vector<std::string>{"compare node"},
          "tree descent to right can leave no candidate");

    names.events.clear();
    names.search_comparison = 0;
    names.byte_comparison = -1;
    Check(pal4::RwTexDictionaryTree::FindNode(
              state, key, names, tree) == state.head,
          "negative byte comparison rejects candidate");

    names.events.clear();
    names.byte_comparison = 0;
    tree.words[0x3014u] = 5u;
    Check(pal4::RwTexDictionaryTree::FindNode(
              state, key, names, tree) == state.head,
          "short query prefix rejects longer node name");

    names.events.clear();
    tree.words[0x3014u] = 3u;
    names.expected_common_length = 3u;
    Check(pal4::RwTexDictionaryTree::FindNode(
              state, key, names, tree) == 0x3000u,
          "longer query prefix accepts shorter candidate per native rule");
}

void NextNodeBranches() {
    Runtime tree;
    tree.nil = 0x1000u;
    constexpr auto head = 0x2000u;
    constexpr auto root = 0x3000u;
    constexpr auto left = 0x3100u;
    constexpr auto right = 0x3200u;
    constexpr auto right_left = 0x3300u;
    tree.words[head + 4u] = root;
    tree.words[head + 8u] = right;
    tree.words[root] = left;
    tree.words[root + 4u] = head;
    tree.words[root + 8u] = right;
    tree.words[left] = tree.nil;
    tree.words[left + 4u] = root;
    tree.words[left + 8u] = tree.nil;
    tree.words[right] = right_left;
    tree.words[right + 4u] = root;
    tree.words[right + 8u] = tree.nil;
    tree.words[right_left] = tree.nil;
    tree.words[right_left + 4u] = right;
    tree.words[right_left + 8u] = tree.nil;

    Check(pal4::RwTexDictionaryTree::NextNode(root, tree) == right_left,
          "right subtree descent reaches its leftmost node");
    Check(pal4::RwTexDictionaryTree::NextNode(left, tree) == root,
          "left child advances to parent");
    Check(pal4::RwTexDictionaryTree::NextNode(right_left, tree) == right,
          "right subtree child advances to parent");
    Check(pal4::RwTexDictionaryTree::NextNode(right, tree) == head,
          "maximum climbs to the head sentinel");

    tree.words[head + 8u] = root;
    tree.words[root] = tree.nil;
    tree.words[root + 8u] = tree.nil;
    Check(pal4::RwTexDictionaryTree::NextNode(root, tree) == head,
          "single root advances to the head sentinel");
}

void FindNodeCandidateBranches() {
    Runtime tree;
    tree.nil = 0x2000u;
    pal4::RwTexDictionaryTreeState state;
    state.head = 0x1000u;
    tree.words[state.head + 4u] = tree.nil;
    InsertRuntime names;
    pal4::RwTexDictionaryName key;
    key.buffer = 0x5000u;
    key.size = 4u;
    Check(pal4::RwTexDictionaryTree::FindNodeCandidate(
              state, key, names, tree) == state.head &&
              names.events.empty(),
          "empty candidate search returns head without comparing");

    constexpr auto root = 0x3000u;
    constexpr auto left = 0x3100u;
    tree.words[state.head + 4u] = root;
    tree.words[root] = left;
    tree.words[root + 8u] = tree.nil;
    tree.words[root + 20u] = 4u;
    tree.words[left] = tree.nil;
    tree.words[left + 8u] = tree.nil;
    tree.words[left + 20u] = 4u;
    Check(pal4::RwTexDictionaryTree::FindNodeCandidate(
              state, key, names, tree) == left &&
              names.events == std::vector<std::string>{"range", "range"},
          "nonnegative comparisons retain the leftmost candidate");

    names.events.clear();
    names.range_comparison = -1;
    Check(pal4::RwTexDictionaryTree::FindNodeCandidate(
              state, key, names, tree) == state.head &&
              names.events == std::vector<std::string>{"range"},
          "negative comparison descends right without a candidate");
}

void DestroyRecursiveOrderAndNames() {
    Runtime runtime;
    runtime.nil = 0x1000u;
    constexpr auto root = 0x3000u;
    constexpr auto left = 0x3100u;
    constexpr auto right = 0x3200u;
    constexpr auto right_left = 0x3300u;
    runtime.words[root] = left;
    runtime.words[root + 8u] = right;
    runtime.words[left] = runtime.nil;
    runtime.words[left + 8u] = runtime.nil;
    runtime.words[right] = right_left;
    runtime.words[right + 8u] = runtime.nil;
    runtime.words[right_left] = runtime.nil;
    runtime.words[right_left + 8u] = runtime.nil;
    runtime.words[root + 16u] = 0x5001u;
    runtime.words[left + 16u] = 0u;
    runtime.words[right + 16u] = 0x5101u;
    runtime.words[right_left + 16u] = 0x5201u;
    for (const auto node : {root, left, right, right_left}) {
        runtime.words[node + 20u] = 4u;
        runtime.words[node + 24u] = 8u;
    }
    runtime.bytes[0x5000u] = 2u;
    runtime.bytes[0x5100u] = 0u;
    runtime.bytes[0x5200u] = 0xffu;

    Check(pal4::RwTexDictionaryTree::DestroyRecursive(
              root, runtime) == runtime.nil,
          "recursive destruction returns shared nil");
    Check(runtime.events == std::vector<std::string>{
              "free:20736", "free:12800", "free:20992",
              "free:13056", "free:12288", "free:12544"},
          "right subtrees are destroyed before left-chain nodes");
    Check(runtime.bytes[0x5000u] == 1u &&
              runtime.words[root + 16u] == 0u &&
              runtime.words[root + 20u] == 0u &&
              runtime.words[root + 24u] == 0u &&
              runtime.words[left + 16u] == 0u,
          "name reference decrement and record clearing follow release");
}

void RotateNonRootBranches() {
    for (const bool node_is_left : {true, false}) {
        NodeInsertRuntime left;
        left.InitializeHead();
        pal4::RwTexDictionaryTreeState state;
        state.head = left.head;
        constexpr auto parent = 0x3000u;
        constexpr auto node = 0x3100u;
        constexpr auto pivot = 0x3200u;
        constexpr auto moved = 0x3300u;
        left.words[left.head + 4u] = parent;
        left.words[parent + (node_is_left ? 0u : 8u)] = node;
        left.words[node + 4u] = parent;
        left.words[node + 8u] = pivot;
        left.words[pivot] = moved;
        left.words[pivot + 4u] = node;
        left.words[moved + 4u] = pivot;
        Check(pal4::RwTexDictionaryTree::RotateLeft(
                  state, node, left) == pivot &&
                  left.words[parent + (node_is_left ? 0u : 8u)] == pivot &&
                  left.words[pivot + 4u] == parent &&
                  left.words[pivot] == node &&
                  left.words[node + 4u] == pivot &&
                  left.words[node + 8u] == moved &&
                  left.words[moved + 4u] == node,
              "left rotation repairs nonroot and moved-child links");

        NodeInsertRuntime right;
        right.InitializeHead();
        state.head = right.head;
        right.words[right.head + 4u] = parent;
        right.words[parent + (node_is_left ? 0u : 8u)] = node;
        right.words[node + 4u] = parent;
        right.words[node] = pivot;
        right.words[pivot + 8u] = moved;
        right.words[pivot + 4u] = node;
        right.words[moved + 4u] = pivot;
        Check(pal4::RwTexDictionaryTree::RotateRight(
                  state, node, right) == pivot &&
                  right.words[parent + (node_is_left ? 0u : 8u)] == pivot &&
                  right.words[pivot + 4u] == parent &&
                  right.words[pivot + 8u] == node &&
                  right.words[node + 4u] == pivot &&
                  right.words[node] == moved &&
                  right.words[moved + 4u] == node,
              "right rotation repairs nonroot and moved-child links");
    }
}

void FindNodeForExistsBranches() {
    Runtime tree;
    tree.nil = 0x0800u;
    pal4::RwTexDictionaryTreeState state;
    state.head = 0x1000u;
    tree.words[state.head + 4u] = tree.nil;
    InsertRuntime names;
    pal4::RwTexDictionaryName key;
    key.buffer = 0x5000u;
    key.size = 4u;
    Check(pal4::RwTexDictionaryTree::FindNodeForExists(
              state, key, names, tree) == state.head &&
              names.events.empty(),
          "empty bytewise lookup returns head");

    constexpr auto root = 0x3000u;
    tree.words[state.head + 4u] = root;
    tree.words[root] = tree.nil;
    tree.words[root + 8u] = tree.nil;
    tree.words[root + 16u] = 0x6000u;
    tree.words[root + 20u] = 4u;
    Check(pal4::RwTexDictionaryTree::FindNodeForExists(
              state, key, names, tree) == root,
          "equal bytes retain candidate and descend left");

    names.events.clear();
    names.node_byte_comparison = -1;
    Check(pal4::RwTexDictionaryTree::FindNodeForExists(
              state, key, names, tree) == state.head,
          "node bytes below query descend right");

    names.node_byte_comparison = 1;
    Check(pal4::RwTexDictionaryTree::FindNodeForExists(
              state, key, names, tree) == root,
          "node bytes above query retain candidate");

    names.node_byte_comparison = 0;
    tree.words[root + 20u] = 3u;
    names.expected_common_length = 3u;
    Check(pal4::RwTexDictionaryTree::FindNodeForExists(
              state, key, names, tree) == state.head,
          "shorter equal-prefix node descends right");
    tree.words[root + 20u] = 5u;
    names.expected_common_length = 4u;
    Check(pal4::RwTexDictionaryTree::FindNodeForExists(
              state, key, names, tree) == root,
          "longer equal-prefix node remains a candidate");
    key.buffer = 0u;
    names.expected_query_buffer = 0u;
    Check(pal4::RwTexDictionaryTree::FindNodeForExists(
              state, key, names, tree) == root,
          "null query buffer reaches comparison boundary");
}

void CreateNodeFields() {
    NodeInsertRuntime runtime;
    runtime.words[runtime.created] = 0xabcdu;
    runtime.words[runtime.created + 8u] = 0x1234u;
    Check(pal4::RwTexDictionaryTree::CreateNode(
              0x3000u, 1u, runtime) == runtime.created &&
              runtime.words[runtime.created + 4u] == 0x3000u &&
              runtime.words[runtime.created + 36u] == 1u &&
              runtime.words[runtime.created] == 0xabcdu &&
              runtime.words[runtime.created + 8u] == 0x1234u &&
              runtime.events == std::vector<std::string>{"create"},
          "node creation writes only parent and color after allocation");
}

void PreviousNodeBranches() {
    Runtime tree;
    tree.nil = 0x1000u;
    constexpr auto head = 0x2000u;
    constexpr auto root = 0x3000u;
    constexpr auto left = 0x3100u;
    constexpr auto inner = 0x3200u;
    constexpr auto right = 0x3300u;
    tree.words[head] = left;
    tree.words[head + 4u] = root;
    tree.words[head + 8u] = right;
    tree.words[head + 36u] = 0u;
    tree.words[root] = left;
    tree.words[root + 4u] = head;
    tree.words[root + 8u] = right;
    tree.words[root + 36u] = 1u;
    tree.words[left] = tree.nil;
    tree.words[left + 4u] = root;
    tree.words[left + 8u] = inner;
    tree.words[left + 36u] = 1u;
    tree.words[inner] = tree.nil;
    tree.words[inner + 4u] = left;
    tree.words[inner + 8u] = tree.nil;
    tree.words[inner + 36u] = 0u;
    tree.words[right] = tree.nil;
    tree.words[right + 4u] = root;
    tree.words[right + 8u] = tree.nil;
    tree.words[right + 36u] = 1u;

    Check(pal4::RwTexDictionaryTree::PreviousNode(root, tree) == inner,
          "left subtree descent reaches its rightmost node");
    Check(pal4::RwTexDictionaryTree::PreviousNode(inner, tree) == left,
          "right child ascends to its parent");
    Check(pal4::RwTexDictionaryTree::PreviousNode(right, tree) == root,
          "right branch ascends to root");
    Check(pal4::RwTexDictionaryTree::PreviousNode(left, tree) == head,
          "minimum ascends to the head sentinel");
    Check(pal4::RwTexDictionaryTree::PreviousNode(head, tree) == right,
          "red-header special case returns cached maximum");
}

void CopyNodeDataBranches() {
    pal4::RwTexDictionaryEntry source;
    source.name.tag = 0x5au;
    source.name.buffer = 0x5001u;
    source.name.size = 4u;
    source.name.capacity = 9u;
    source.dictionary = 0x7000u;
    source.references = 3u;
    constexpr auto destination = 0x400cu;

    NodeInsertRuntime shared;
    shared.bytes[0x5000u] = 0xfdu;
    pal4::RwTexDictionaryTree::CopyNodeData(
        destination, source, shared);
    Check(shared.bytes[destination] == 0x5au &&
              shared.words[destination + 4u] == 0x5001u &&
              shared.words[destination + 8u] == 4u &&
              shared.words[destination + 12u] == 9u &&
              shared.words[destination + 16u] == 0x7000u &&
              shared.words[destination + 20u] == 3u &&
              shared.bytes[0x5000u] == 0xfeu &&
              shared.events == std::vector<std::string>{"copy node"},
          "shareable name copies metadata and increments reference byte");

    for (const std::uint8_t saturated : {0xfeu, 0xffu}) {
        NodeInsertRuntime deep;
        deep.bytes[0x5000u] = saturated;
        for (std::uint32_t i = 0u; i < 4u; ++i)
            deep.bytes[0x5001u + i] = static_cast<std::uint8_t>('a' + i);
        pal4::RwTexDictionaryTree::CopyNodeData(
            destination, source, deep);
        Check(deep.words[destination + 4u] == deep.copied_buffer &&
                  deep.words[destination + 8u] == 4u &&
                  deep.words[destination + 12u] == 4u &&
                  deep.words[destination + 16u] == 0x7000u &&
                  deep.words[destination + 20u] == 3u &&
                  deep.bytes[deep.copied_buffer] == 'a' &&
                  deep.bytes[deep.copied_buffer + 3u] == 'd' &&
                  deep.bytes[deep.copied_buffer + 4u] == 0u &&
                  deep.events == std::vector<std::string>{
                      "allocate name", "copy bytes"},
              "saturated reference count forces terminated deep copy");
    }

    NodeInsertRuntime failed;
    failed.allocate_name = false;
    failed.bytes[0x5000u] = 0xfeu;
    pal4::RwTexDictionaryTree::CopyNodeData(
        destination, source, failed);
    Check(failed.words[destination + 4u] == 0u &&
              failed.words[destination + 8u] == 0u &&
              failed.words[destination + 12u] == 0u &&
              failed.words[destination + 16u] == 0x7000u &&
              failed.words[destination + 20u] == 3u &&
              failed.events == std::vector<std::string>{"allocate name"},
          "failed deep allocation still copies dictionary payload");

    NodeInsertRuntime empty;
    source.name.buffer = 0u;
    source.name.size = 0u;
    pal4::RwTexDictionaryTree::CopyNodeData(
        destination, source, empty);
    Check(empty.words[destination + 4u] == empty.copied_buffer &&
              empty.words[destination + 8u] == 0u &&
              empty.bytes[empty.copied_buffer] == 0u &&
              empty.events == std::vector<std::string>{
                  "allocate name", "copy bytes"},
          "zero-length name takes allocation branch");
    empty.events.clear();
    pal4::RwTexDictionaryTree::CopyNodeData(0u, source, empty);
    Check(empty.events.empty(), "null destination skips copy");

    NodeInsertRuntime in_place;
    in_place.bytes[destination] = 0x5au;
    in_place.words[destination + 4u] = 0x5001u;
    in_place.words[destination + 8u] = 4u;
    in_place.words[destination + 12u] = 9u;
    in_place.words[destination + 16u] = 0x7000u;
    in_place.words[destination + 20u] = 3u;
    pal4::RwTexDictionaryTree::CopyNodeData(
        destination, destination, in_place);
    Check(in_place.words[destination + 4u] == 0u &&
              in_place.words[destination + 8u] == 0u &&
              in_place.words[destination + 12u] == 0u &&
              in_place.words[destination + 16u] == 0x7000u &&
              in_place.words[destination + 20u] == 3u &&
              in_place.events == std::vector<std::string>{
                  "detach", "detach"},
          "self source resets name and preserves trailing payload");
}

void DestroyNodeFlags() {
    Runtime runtime;
    constexpr auto payload = 0x300cu;
    runtime.words[payload + 4u] = 0u;
    runtime.words[payload + 8u] = 4u;
    runtime.words[payload + 12u] = 8u;
    Check(pal4::RwTexDictionaryTree::DestroyNode(
              payload, 0u, runtime) == payload &&
              runtime.words[payload + 8u] == 0u &&
              runtime.words[payload + 12u] == 0u &&
              runtime.events.empty(),
          "zero flags clear name without freeing payload");

    runtime.words[payload + 4u] = 0x5001u;
    runtime.words[payload + 8u] = 4u;
    runtime.words[payload + 12u] = 8u;
    runtime.bytes[0x5000u] = 0u;
    Check(pal4::RwTexDictionaryTree::DestroyNode(
              payload, 3u, runtime) == payload &&
              runtime.words[payload + 4u] == 0u &&
              runtime.events == std::vector<std::string>{
                  "free:20480", "free:12300"},
          "low flag bit frees name header before payload");
}

}  // namespace

int main() {
    ConstructAndShareNil();
    RecursiveAndRemovalPaths();
    CleanupEmptyAndPopulatedTrees();
    CleanupZeroCountLinkedNodes();
    InsertTextureBranches();
    ReleaseNameReferenceCounts();
    RemoveTextureByNameBranches();
    SetCurrentCandidateBranches();
    CheckTextureExistsBranches();
    InternalInsertBranches();
    InternalInsertRotation();
    InternalInsertMirrorAndRecolor();
    RemoveNodeStructuresAndText();
    RemoveNodeWithSuccessor();
    FindNodeBranches();
    NextNodeBranches();
    FindNodeCandidateBranches();
    DestroyRecursiveOrderAndNames();
    RotateNonRootBranches();
    FindNodeForExistsBranches();
    CreateNodeFields();
    PreviousNodeBranches();
    CopyNodeDataBranches();
    DestroyNodeFlags();
    std::cout << "24 RW texture dictionary tree test cases passed\n";
}
