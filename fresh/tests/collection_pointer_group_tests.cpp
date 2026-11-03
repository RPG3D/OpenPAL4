#include "pal4/collection_pointer_group.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ICollectionPointerGroupRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("read8:" + std::to_string(address));
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.push_back("write8:" + std::to_string(address) + ":" +
                        std::to_string(value));
        bytes[address] = value;
    }
    std::uint32_t FreeMemory(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
        return address + 0x10u;
    }
    void DeleteObject(std::uint32_t address,
                      std::uint32_t flag) override {
        calls.push_back("delete:" + std::to_string(address) + ":" +
                        std::to_string(flag));
    }
};

struct ProcessRuntime final : pal4::ICollectionProcessAllRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> output_values;
    std::vector<std::string> calls;
    std::uint32_t index{};
    std::uint32_t next_allocation{0x5000u};
    std::uint32_t iterator{0x7000u};
    bool throw_on_process{};
    std::uint32_t seen_input{};
    std::uint32_t seen_vtable{};
    std::uint32_t seen_initial_output{1u};

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    std::uint32_t AllocateMemory(std::uint32_t bytes) override {
        calls.push_back("allocate:" + std::to_string(bytes));
        const auto result = next_allocation;
        next_allocation += 0x100u;
        return result;
    }
    void FreeMemory(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
    std::uint32_t CreateIterator(std::uint32_t source) override {
        calls.push_back("create:" + std::to_string(source));
        return iterator;
    }
    std::uint32_t IsEnd(std::uint32_t address,
                        std::uint32_t slot) override {
        calls.push_back("end:" + std::to_string(address) + ":" +
                        std::to_string(slot));
        return index < output_values.size() ? 0u : 1u;
    }
    std::uint32_t Current(std::uint32_t address,
                          std::uint32_t slot) override {
        calls.push_back("current:" + std::to_string(address) + ":" +
                        std::to_string(slot));
        return 0x4000u + index;
    }
    void ProcessCurrent(std::uint32_t element,
                        pal4::CollectionPointerVisitorState& visitor,
                        std::uint32_t slot) override {
        calls.push_back("process:" + std::to_string(element) + ":" +
                        std::to_string(slot));
        seen_input = visitor.input;
        seen_vtable = visitor.vtable;
        seen_initial_output = visitor.output;
        if (throw_on_process) throw std::runtime_error("process");
        visitor.output = output_values.at(index);
    }
    void Advance(std::uint32_t address, std::uint32_t slot) override {
        calls.push_back("advance:" + std::to_string(address) + ":" +
                        std::to_string(slot));
        ++index;
    }
    std::int32_t DestroyIterator(std::uint32_t address,
                                 std::uint32_t flag) override {
        calls.push_back("destroy:" + std::to_string(address) + ":" +
                        std::to_string(flag));
        return -7;
    }
};

struct SortRuntime final : pal4::ICollectionPointerSortRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t allocated{0x5000u};
    std::uint32_t split_offset{32u};
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t AllocateMemory(std::uint32_t bytes) override {
        calls.push_back("allocate:" + std::to_string(bytes));
        return allocated;
    }
    void FreeMemory(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
    void InsertionSort(std::uint32_t begin, std::uint32_t end,
                       std::uint32_t sentinel,
                       std::uint32_t original_end) override {
        calls.push_back("insertion:" + std::to_string(begin) + ":" +
                        std::to_string(end) + ":" +
                        std::to_string(sentinel) + ":" +
                        std::to_string(original_end));
        std::vector<std::uint32_t> values;
        for (auto address = begin; address != end; address += 4u)
            values.push_back(words.at(address));
        std::sort(values.begin(), values.end());
        for (std::size_t i = 0; i < values.size(); ++i)
            words[begin + static_cast<std::uint32_t>(i) * 4u] = values[i];
    }
    std::uint32_t CompareThree(std::uint32_t first,
                                std::uint32_t middle,
                                std::uint32_t last,
                                std::uint32_t original_end) override {
        calls.push_back("three:" + std::to_string(first) + ":" +
                        std::to_string(middle) + ":" +
                        std::to_string(last) + ":" +
                        std::to_string(original_end));
        return middle;
    }
    std::uint32_t Partition(std::uint32_t begin,
                            std::uint32_t end,
                            std::uint32_t pivot) override {
        calls.push_back("partition:" + std::to_string(begin) + ":" +
                        std::to_string(end) + ":" +
                        std::to_string(pivot));
        return begin + split_offset;
    }
    void PartitionRange(std::uint32_t begin,
                        std::uint32_t end,
                        std::uint32_t original_end) override {
        calls.push_back("range:" + std::to_string(begin) + ":" +
                        std::to_string(end) + ":" +
                        std::to_string(original_end));
    }
    std::int32_t CompareItems(std::uint32_t candidate,
                              std::uint32_t previous) override {
        calls.push_back("compare:" + std::to_string(candidate) + ":" +
                        std::to_string(previous));
        return candidate < previous ? -1 : candidate > previous ? 1 : 0;
    }
};

struct NameRuntime final : pal4::ICollectionNameLookupRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::uint32_t> records;
    std::vector<std::uint32_t> slots;
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    std::uint32_t GetNameRecord(std::uint32_t object,
                                std::uint32_t slot) override {
        slots.push_back(slot);
        return records.at(object);
    }
    void Put(std::uint32_t index, const std::string& name,
             bool null_data = false) {
        const auto object = 0x5000u + index;
        const auto record = 0x6000u + index * 0x20u;
        const auto data = 0x7000u + index * 0x20u;
        words[0x2000u + index * 4u] = object;
        records[object] = record;
        words[record + 4u] = null_data ? 0u : data;
        words[record + 8u] = static_cast<std::uint32_t>(name.size());
        for (std::size_t i = 0u; i < name.size(); ++i)
            bytes[data + static_cast<std::uint32_t>(i)] =
                static_cast<std::uint8_t>(name[i]);
    }
};

pal4::CollectionPointerGroupState MakeGroup() {
    return {0x1234u,
            0x1000u, 0x1008u, 0x1010u,
            0x5555u,
            0x2000u, 0x2004u, 0x2008u,
            0x6666u,
            0x3000u, 0x3008u, 0x3010u};
}

void Seed(Runtime& runtime) {
    runtime.words[0x1000u] = 0x5000u;
    runtime.words[0x1004u] = 0u;
    runtime.words[0x3000u] = 0x4000u;
    runtime.words[0x3004u] = 0u;
    runtime.words[0x400cu] = 0x6000u;
    runtime.words[0x4010u] = 3u;
    runtime.words[0x4014u] = 8u;
    runtime.bytes[0x5fffu] = 2u;
}

void TestStringHolderCleanup() {
    for (const std::uint8_t references : {2u, 0u, 0xffu}) {
        Runtime runtime;
        Seed(runtime);
        runtime.bytes[0x5fffu] = references;
        const auto result = pal4::CollectionPointerGroup::CleanupStringHolder(
            0x4000u, runtime);
        Check(runtime.words.at(0x400cu) == 0u &&
                  runtime.words.at(0x4010u) == 0u &&
                  runtime.words.at(0x4014u) == 0u,
              "holder cleanup clears only its three text fields");
        if (references == 2u) {
            Check(result == 0x5fffu &&
                      runtime.bytes.at(0x5fffu) == 1u &&
                      runtime.calls[1] == "read8:24575" &&
                      runtime.calls[2] == "write8:24575:1",
                  "shared reference header decrements and returns header");
        } else {
            Check(result == 0x600fu &&
                      runtime.calls[2] == "free:24575",
                  "zero and ff headers free and forward free result");
        }
    }
    Runtime empty;
    empty.words[0x400cu] = 0u;
    Check(pal4::CollectionPointerGroup::CleanupStringHolder(
              0x4000u, empty) == 0u &&
              empty.words.at(0x400cu) == 0u,
          "null holder text skips reference-header access");
}

void TestClearAndCleanup() {
    Runtime runtime;
    Seed(runtime);
    auto group = MakeGroup();
    Check(pal4::CollectionPointerGroup::Clear(group, runtime) == 0x3008u &&
              group.first_end == group.first_begin &&
              group.second_end == group.second_begin &&
              group.third_end == group.third_begin &&
              group.first_capacity == 0x1010u &&
              group.second_capacity == 0x2008u &&
              group.third_capacity == 0x3010u &&
              runtime.bytes.at(0x5fffu) == 1u,
          "clear releases objects and holders while retaining array storage");
    const auto first_delete = std::find(runtime.calls.begin(),
                                        runtime.calls.end(),
                                        "delete:20480:1");
    const auto holder_free = std::find(runtime.calls.begin(),
                                       runtime.calls.end(),
                                       "free:16384");
    Check(first_delete != runtime.calls.end() &&
              holder_free != runtime.calls.end() &&
              first_delete < holder_free,
          "object deletion precedes holder cleanup");

    runtime.calls.clear();
    Check(pal4::CollectionPointerGroup::Cleanup(group, runtime) == 0x1010u &&
              group.first_begin == 0u && group.first_end == 0u &&
              group.first_capacity == 0u &&
              group.second_begin == 0u && group.second_end == 0u &&
              group.second_capacity == 0u &&
              group.third_begin == 0u && group.third_end == 0u &&
              group.third_capacity == 0u &&
              group.vtable == 0x1234u &&
              group.unknown_10 == 0x5555u &&
              group.unknown_20 == 0x6666u &&
              runtime.calls == std::vector<std::string>{
                  "free:12288", "free:8192", "free:4096"},
          "cleanup frees third, second, first storage and preserves other fields");
}

void TestDirectCleanupPopulated() {
    Runtime runtime;
    Seed(runtime);
    auto group = MakeGroup();
    (void)pal4::CollectionPointerGroup::Cleanup(group, runtime);
    const auto holder_free = std::find(runtime.calls.begin(),
                                       runtime.calls.end(),
                                       "free:16384");
    const auto third_free = std::find(runtime.calls.begin(),
                                      runtime.calls.end(),
                                      "free:12288");
    Check(holder_free != runtime.calls.end() &&
              third_free != runtime.calls.end() &&
              holder_free < third_free,
          "direct cleanup destroys contained holder before its array");
    pal4::CollectionPointerGroupRuntimeStub stub;
    Check(stub.ReadWord(1u) == 0u && stub.FreeMemory(0x20u) == 0x20u,
          "native boundary has a linkable no-op stub");
}

void TestProcessAll() {
    ProcessRuntime runtime;
    runtime.output_values = {0xaaaau, 0xbbbbu};
    pal4::CollectionPointerGroupState group{};
    Check(pal4::CollectionPointerGroup::ProcessAll(
              group, 0x9000u, 0x5555u, runtime) == -7 &&
              runtime.seen_vtable == 0x849718u &&
              runtime.seen_input == 0x5555u &&
              runtime.seen_initial_output == 0u &&
              runtime.words.at(0x5100u) == 0xaaaau &&
              runtime.words.at(0x5104u) == 0xbbbbu &&
              group.first_begin == 0x5100u &&
              group.first_end == 0x5108u &&
              group.first_capacity == 0x5108u &&
              runtime.calls.front() == "create:36864" &&
              runtime.calls.back() == "destroy:28672:1",
          "iterator visits and appends both outputs before deleting itself");
    Check(std::find(runtime.calls.begin(), runtime.calls.end(),
                    "advance:28672:36") != runtime.calls.end() &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                    "current:28672:12") != runtime.calls.end(),
          "iterator virtual slots twelve and thirty-six are used");

    ProcessRuntime empty;
    Check(pal4::CollectionPointerGroup::ProcessAll(
              group, 0x9000u, 0u, empty) == -7 &&
              empty.calls == std::vector<std::string>{
                  "create:36864", "end:28672:24", "destroy:28672:1"},
          "empty iterator still invokes deleting virtual");
}

void TestProcessAllException() {
    ProcessRuntime runtime;
    runtime.output_values = {0x1111u};
    runtime.throw_on_process = true;
    pal4::CollectionPointerGroupState group{};
    bool caught = false;
    try {
        (void)pal4::CollectionPointerGroup::ProcessAll(
            group, 0x9000u, 1u, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && group.first_begin == 0u &&
              runtime.calls.back() == "destroy:28672:1",
          "exceptional visitor dispatch destroys iterator before propagating");
}

void TestSortCopyBranches() {
    SortRuntime runtime;
    auto group = MakeGroup();
    group.first_end = 0x1008u;
    group.second_end = 0x200cu;
    group.second_capacity = 0x2010u;
    runtime.words[0x1000u] = 2u;
    runtime.words[0x1004u] = 1u;
    pal4::CollectionPointerGroup::Sort(group, runtime);
    Check(group.second_end == 0x2008u &&
              group.second_capacity == 0x2010u &&
              runtime.words.at(0x2000u) == 1u &&
              runtime.words.at(0x2004u) == 2u &&
              runtime.calls == std::vector<std::string>{
                  "insertion:8192:8200:0:8200"},
          "shorter source overwrites destination then sorts without allocation");

    runtime.calls.clear();
    group.first_end = 0x100cu;
    group.second_end = 0x2004u;
    runtime.words[0x1000u] = 3u;
    runtime.words[0x1004u] = 1u;
    runtime.words[0x1008u] = 2u;
    pal4::CollectionPointerGroup::Sort(group, runtime);
    Check(group.second_end == 0x200cu &&
              group.second_capacity == 0x2010u &&
              runtime.words.at(0x2000u) == 1u &&
              runtime.words.at(0x2004u) == 2u &&
              runtime.words.at(0x2008u) == 3u &&
              runtime.calls.size() == 1u,
          "larger source uses existing capacity and appends missing tail");
}

void TestSortReallocationAndFailure() {
    SortRuntime runtime;
    auto group = MakeGroup();
    group.first_end = 0x100cu;
    group.second_end = 0x2000u;
    group.second_capacity = 0x2004u;
    runtime.words[0x1000u] = 3u;
    runtime.words[0x1004u] = 1u;
    runtime.words[0x1008u] = 2u;
    pal4::CollectionPointerGroup::Sort(group, runtime);
    Check(group.second_begin == 0x5000u &&
              group.second_end == 0x500cu &&
              group.second_capacity == 0x500cu &&
              runtime.words.at(0x5000u) == 1u &&
              runtime.words.at(0x5004u) == 2u &&
              runtime.words.at(0x5008u) == 3u &&
              runtime.calls == std::vector<std::string>{
                  "free:8192", "allocate:12",
                  "insertion:20480:20492:0:20492"},
          "insufficient capacity frees, reallocates, copies and sorts");

    runtime.allocated = 0u;
    runtime.calls.clear();
    group.second_begin = 0x2000u;
    group.second_end = 0x2000u;
    group.second_capacity = 0x2004u;
    bool caught = false;
    try {
        pal4::CollectionPointerGroup::Sort(group, runtime);
    } catch (const std::bad_alloc&) {
        caught = true;
    }
    Check(caught && group.second_begin == 0u &&
              group.second_end == 0u &&
              group.second_capacity == 0u &&
              runtime.calls == std::vector<std::string>{
                  "free:8192", "allocate:12"},
          "allocation failure leaves the freed destination metadata clear");
}

void TestLargeSortDispatch() {
    SortRuntime runtime;
    pal4::CollectionPointerGroupState group{};
    group.first_begin = 0x1000u;
    group.first_end = 0x1044u;
    for (std::uint32_t i = 0u; i < 16u; ++i)
        runtime.words[0x1000u + i * 4u] = i + 1u;
    runtime.words[0x1040u] = 10u;
    pal4::CollectionPointerGroup::Sort(group, runtime);
    Check(group.second_begin == 0x5000u &&
              group.second_end == 0x5044u &&
              runtime.calls[0] == "free:0" &&
              runtime.calls[1] == "allocate:68" &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "range:20480:20512:20548") != runtime.calls.end() &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "insertion:20480:20544:0:20548") !=
                  runtime.calls.end() &&
              runtime.words.at(0x5024u) == 10u &&
              runtime.words.at(0x5028u) == 10u &&
              runtime.words.at(0x5040u) == 16u,
          "large sort dispatches partition helpers then inserts final item");

    SortRuntime other;
    other.split_offset = 48u;
    other.words = runtime.words;
    group.second_begin = 0x6000u;
    group.second_end = 0x6000u;
    group.second_capacity = 0x6044u;
    pal4::CollectionPointerGroup::Sort(group, other);
    Check(std::find(other.calls.begin(), other.calls.end(),
                    "range:24624:24644:24644") != other.calls.end(),
          "opposite partition size dispatches the right segment");
}

void TestFindByName() {
    NameRuntime runtime;
    runtime.Put(0u, "", true);
    runtime.Put(1u, "alpha");
    runtime.Put(2u, "alpha");
    runtime.Put(3u, "beta");
    runtime.Put(4u, "cyan");
    pal4::CollectionPointerGroupState group{};
    group.second_begin = 0x2000u;
    group.second_end = 0x2014u;
    Check(pal4::CollectionPointerGroup::FindByName(
              group, "", runtime) == 0x5000u &&
              pal4::CollectionPointerGroup::FindByName(
                  group, "alpha", runtime) == 0x5001u &&
              pal4::CollectionPointerGroup::FindByName(
                  group, "cyan", runtime) == 0x5004u,
          "binary lookup finds empty, first duplicate and final name");
    Check(pal4::CollectionPointerGroup::FindByName(
              group, "bet", runtime) == 0u &&
              pal4::CollectionPointerGroup::FindByName(
                  group, "a", runtime) == 0u &&
              pal4::CollectionPointerGroup::FindByName(
                  group, "delta", runtime) == 0u &&
              pal4::CollectionPointerGroup::FindByName(
                  group, "Alpha", runtime) == 0u &&
              std::all_of(runtime.slots.begin(), runtime.slots.end(),
                          [](std::uint32_t slot) { return slot == 4u; }),
          "prefix, absent and case-different names miss via virtual slot four");
    group.second_end = group.second_begin;
    runtime.slots.clear();
    Check(pal4::CollectionPointerGroup::FindByName(
              group, "alpha", runtime) == 0u &&
              runtime.slots.empty(),
          "empty sorted array returns null without virtual lookup");
    pal4::CollectionNameLookupRuntimeStub stub;
    Check(stub.ReadWord(1u) == 0u &&
              stub.GetNameRecord(1u, 4u) == 0u,
          "native name lookup boundary has a linkable stub");
}

}  // namespace

int main() {
    TestStringHolderCleanup();
    TestClearAndCleanup();
    TestDirectCleanupPopulated();
    TestProcessAll();
    TestProcessAllException();
    TestSortCopyBranches();
    TestSortReallocationAndFailure();
    TestLargeSortDispatch();
    TestFindByName();
}
