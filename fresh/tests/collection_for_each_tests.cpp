#include "pal4/collection_for_each.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ICollectionForEachRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t next{0x5000u};
    std::uint32_t allocations{};
    std::uint32_t fail_allocation{};
    std::uint32_t throw_on_apply{};
    std::uint32_t expected_apply_slot{8u};

    std::uint32_t Allocate(const std::uint32_t bytes) override {
        calls.push_back("allocate:" + std::to_string(bytes));
        ++allocations;
        if (allocations == fail_allocation) return 0u;
        const auto result = next;
        next += 0x100u;
        return result;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
    }
    void Free(const std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
    bool AtEnd(const std::uint32_t iterator,
               const std::uint32_t vtable_offset) override {
        Check(vtable_offset == 24u, "end predicate uses virtual slot 24");
        const auto owner = ReadWord(iterator + 8u);
        return ReadWord(iterator + 4u) == ReadWord(owner + 8u);
    }
    void Advance(const std::uint32_t iterator,
                 const std::uint32_t vtable_offset) override {
        Check(vtable_offset == 36u, "advance uses virtual slot 36");
        calls.push_back("advance:" + std::to_string(iterator));
        WriteWord(iterator + 4u, ReadWord(iterator + 4u) + 4u);
    }
    void Apply(const std::uint32_t element,
               const pal4::CollectionForEachVisitorState& visitor,
               const std::uint32_t vtable_offset) override {
        Check(vtable_offset == expected_apply_slot &&
                  visitor.vtable == 0x849938u,
              "element receives visitor through selected virtual slot");
        calls.push_back("apply:" + std::to_string(element) + ":" +
                        std::to_string(visitor.value) + ":" +
                        std::to_string(visitor.parameter));
        if (element == throw_on_apply) throw std::runtime_error("apply");
    }
    std::uint32_t Transform(const std::uint32_t element,
                            const std::uint32_t value,
                            const std::uint32_t vtable_offset) override {
        Check(vtable_offset == 0u,
              "child context transformation uses virtual slot zero");
        calls.push_back("transform:" + std::to_string(element) + ":" +
                        std::to_string(value));
        return value + 0x10u;
    }
};

void SeedCollection(Runtime& runtime, const std::uint32_t group,
                    const std::uint32_t first,
                    const std::uint32_t third) {
    runtime.words[group + 4u] = first;
    runtime.words[group + 8u] = first;
    runtime.words[group + 36u] = third;
    runtime.words[group + 40u] = third;
}

void TestEmptyAndFailure() {
    Runtime runtime;
    SeedCollection(runtime, 0x1000u, 0x2000u, 0x2100u);
    Check(pal4::CollectionForEach::Visit(
              0x1000u, 0xabu, 0xcdu, runtime) == 0x5000u &&
              runtime.calls == std::vector<std::string>{
                  "allocate:12", "allocate:12", "free:20736",
                  "free:20480"},
          "empty collection creates both iterators and releases in reverse");

    Runtime failed_first;
    SeedCollection(failed_first, 0x1000u, 0x2000u, 0x2100u);
    failed_first.fail_allocation = 1u;
    bool caught = false;
    try {
        (void)pal4::CollectionForEach::Visit(
            0x1000u, 1u, 2u, failed_first);
    } catch (const std::bad_alloc&) {
        caught = true;
    }
    Check(caught && failed_first.calls ==
              std::vector<std::string>{"allocate:12"},
          "failed first iterator stops before any traversal");

    Runtime failed_third;
    SeedCollection(failed_third, 0x1000u, 0x2000u, 0x2100u);
    failed_third.fail_allocation = 2u;
    caught = false;
    try {
        (void)pal4::CollectionForEach::Visit(
            0x1000u, 1u, 2u, failed_third);
    } catch (const std::bad_alloc&) {
        caught = true;
    }
    Check(caught && failed_third.calls == std::vector<std::string>{
              "allocate:12", "allocate:12", "free:20480"},
          "failed third iterator releases the first");
}

void TestRecursiveVisit() {
    Runtime runtime;
    SeedCollection(runtime, 0x1000u, 0x2000u, 0x2100u);
    SeedCollection(runtime, 0x1100u, 0x2200u, 0x2300u);
    runtime.words[0x1008u] = 0x2008u;
    runtime.words[0x2000u] = 0x3000u;
    runtime.words[0x2004u] = 0x3004u;
    runtime.words[0x1028u] = 0x2104u;
    runtime.words[0x2100u] = 0x3100u;
    runtime.words[0x3104u] = 0x1100u;
    runtime.words[0x1108u] = 0x2204u;
    runtime.words[0x2200u] = 0x3200u;

    Check(pal4::CollectionForEach::Visit(
              0x1000u, 0x10u, 0x20u, runtime) == 0x5000u,
          "recursive collection visit completes");
    std::vector<std::string> actions;
    std::vector<std::string> releases;
    for (const auto& call : runtime.calls)
        if (call.starts_with("apply:") || call.starts_with("transform:"))
            actions.push_back(call);
        else if (call.starts_with("free:"))
            releases.push_back(call);
    Check(actions == std::vector<std::string>{
              "apply:12288:16:32", "apply:12292:16:32",
              "transform:12544:16", "apply:12800:32:32"} &&
              releases == std::vector<std::string>{
                  "free:21248", "free:20992", "free:20736",
                  "free:20480"},
          "direct applications precede transformed child and cleanup is nested");

    Runtime exceptional;
    exceptional.words = runtime.words;
    exceptional.throw_on_apply = 0x3200u;
    bool caught = false;
    try {
        (void)pal4::CollectionForEach::Visit(
            0x1000u, 0x10u, 0x20u, exceptional);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && exceptional.calls[exceptional.calls.size() - 3u] ==
              "free:20992" &&
              exceptional.calls[exceptional.calls.size() - 2u] ==
                  "free:20736" &&
              exceptional.calls.back() == "free:20480",
          "recursive callback failure releases each live iterator");
}

void TestSetProperties() {
    Runtime runtime;
    runtime.expected_apply_slot = 12u;
    SeedCollection(runtime, 0x1000u, 0x2000u, 0x2100u);
    SeedCollection(runtime, 0x1100u, 0x2200u, 0x2300u);
    runtime.words[0x1008u] = 0x2004u;
    runtime.words[0x2000u] = 0x3000u;
    runtime.words[0x1028u] = 0x2104u;
    runtime.words[0x2100u] = 0x3100u;
    runtime.words[0x3104u] = 0x1100u;
    runtime.words[0x1108u] = 0x2204u;
    runtime.words[0x2200u] = 0x3200u;
    Check(pal4::CollectionForEach::SetProperties(
              0x1000u, 0x10u, 0x20u, runtime) == 0x5000u,
          "property traversal returns first iterator cleanup result");
    std::vector<std::string> actions;
    std::vector<std::string> releases;
    for (const auto& call : runtime.calls)
        if (call.starts_with("apply:") || call.starts_with("transform:"))
            actions.push_back(call);
        else if (call.starts_with("free:"))
            releases.push_back(call);
    Check(actions == std::vector<std::string>{
              "apply:12288:16:32", "transform:12544:16",
              "apply:12800:32:32"},
          "property traversal applies direct items then transformed children");
    Check(releases == std::vector<std::string>{
              "free:21248", "free:20992", "free:20736", "free:20480"},
          "property traversal releases both nested iterator pairs");

    Runtime exceptional;
    exceptional.words = runtime.words;
    exceptional.expected_apply_slot = 12u;
    exceptional.throw_on_apply = 0x3200u;
    bool caught = false;
    try {
        (void)pal4::CollectionForEach::SetProperties(
            0x1000u, 0x10u, 0x20u, exceptional);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && exceptional.calls.back() == "free:20480",
          "property traversal cleans iterators after nested callback failure");
}

}  // namespace

int main() {
    TestEmptyAndFailure();
    TestRecursiveVisit();
    TestSetProperties();
    pal4::CollectionForEachRuntimeStub stub;
    Check(stub.Allocate(12u) == 0u && stub.ReadWord(0u) == 0u &&
              stub.Transform(0u, 0u, 0u) == 0u,
          "native for-each boundary has a linkable stub");
}
