#include "pal4/pal_object_manager_traversal.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IPalObjectManagerTraversalRuntime {
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint32_t> words{
        {0x1010u, 0x2000u}, {0x3010u, 0x4000u}, {0x3110u, 0x4100u}};
    std::uint32_t count{2u};
    std::uint32_t empty_result{0u};

    [[nodiscard]] std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        Check(words.contains(address), "expected manager or instance field");
        return words[address];
    }
    [[nodiscard]] std::uint32_t IsPropertyEmpty(
        const std::uint32_t object) override {
        Check(object == 0x1000u, "empty check object");
        calls.push_back("empty");
        return empty_result;
    }
    void InitializeIterator(pal4::PalObjectManagerIterator& source,
                            const std::uint32_t collection) override {
        Check(collection == 0x2000u, "collection at object plus sixteen");
        calls.push_back("initialize");
        source = {0u, count};
    }
    void WrapIterator(pal4::PalObjectManagerIterator& destination,
                      pal4::PalObjectManagerIterator& source) override {
        calls.push_back("wrap");
        destination = source;
    }
    [[nodiscard]] std::uint32_t IsEnd(
        pal4::PalObjectManagerIterator& iterator) override {
        calls.push_back("end");
        return iterator.first == iterator.second ? 0x770001u : 0x880000u;
    }
    [[nodiscard]] std::uint32_t GetInstance(
        pal4::PalObjectManagerIterator& iterator) override {
        calls.push_back("instance");
        return 0x3000u + iterator.first * 0x100u;
    }
    void Invoke(const std::uint32_t target, const std::uint32_t slot,
                const std::uint32_t argument) override {
        Check((target == 0x4000u || target == 0x4100u) &&
                  (slot == 8u || slot == 12u) && argument == 0x66u,
              "virtual call target, slot and argument");
        calls.push_back("invoke:" + std::to_string(target) + ":" +
                        std::to_string(slot));
    }
    void Remove(pal4::PalObjectManagerIterator& iterator,
                const std::uint32_t flag) override {
        Check(flag == 0u, "iterator removal flag");
        calls.push_back("remove");
        ++iterator.first;
    }
};

void TestFirstEntry() {
    Runtime runtime;
    Check(pal4::PalObjectManagerTraversal::Process(
              0x1000u, 0x66u, runtime) == 0x770001u,
          "first traversal returns final iterator result");
    Check(runtime.calls == std::vector<std::string>{
              "read:4112", "initialize", "wrap", "end", "instance",
              "read:12304", "invoke:16384:8", "remove", "end", "instance",
              "read:12560", "invoke:16640:8", "remove", "end"},
          "first traversal callback and removal order");

    runtime.calls.clear();
    runtime.count = 0u;
    Check(pal4::PalObjectManagerTraversal::Process(
              0x1000u, 0x66u, runtime) == 0x770001u &&
              runtime.calls == std::vector<std::string>{
                  "read:4112", "initialize", "wrap", "end"},
          "empty first traversal only checks iterator");
}

void TestSecondEntry() {
    Runtime runtime;
    runtime.empty_result = 0x550001u;
    Check(pal4::PalObjectManagerTraversal::ProcessWhenPresent(
              0x1000u, 0x66u, 0xdeadu, runtime) == 0x550001u &&
              runtime.calls == std::vector<std::string>{"empty"},
          "second traversal exits on nonzero low empty byte");

    runtime.calls.clear();
    runtime.empty_result = 0x550000u;
    Check(pal4::PalObjectManagerTraversal::ProcessWhenPresent(
              0x1000u, 0x66u, 0xdeadu, runtime) == 0x770001u,
          "second traversal returns iterator completion");
    Check(runtime.calls == std::vector<std::string>{
              "empty", "read:4112", "initialize", "wrap", "end",
              "instance", "read:12304", "invoke:16384:12", "remove", "end",
              "instance", "read:12560", "invoke:16640:12", "remove", "end"},
          "second traversal uses virtual slot twelve and ignores third argument");
}

}  // namespace

int main() {
    TestFirstEntry();
    TestSecondEntry();
}
