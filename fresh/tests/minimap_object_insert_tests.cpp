#include "pal4/minimap_object_collection.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IMinimapObjectInsertRuntime {
public:
    std::map<std::uint32_t, std::uint32_t> words{
        {0x6000u, 1u}, {0x6014u, 2u},
        {0x6028u, 3u}, {0x9000u, 9u}};
    std::vector<std::string> events;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        events.emplace_back("alloc:" + std::to_string(bytes));
        return 0xa000u;
    }
    void CopyObject(std::uint32_t destination,
                    std::uint32_t source) override {
        events.emplace_back("copy:" + std::to_string(destination) +
                            ":" + std::to_string(source));
        words[destination] = words[source];
    }
    void AssignNestedArray(std::uint32_t destination,
                           std::uint32_t source) override {
        events.emplace_back("assign:" + std::to_string(destination) +
                            ":" + std::to_string(source));
    }
    void CleanupGroup(std::uint32_t address) override {
        events.emplace_back("cleanup:" + std::to_string(address));
    }
    void Free(std::uint32_t address) override {
        events.emplace_back("free:" + std::to_string(address));
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        return words[address];
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        words[address] = value;
    }
};

pal4::MinimapObjectCollectionState State(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t capacity) {
    pal4::MinimapObjectCollectionState state;
    state.WriteDword(4u, begin);
    state.WriteDword(8u, end);
    state.WriteDword(12u, capacity);
    return state;
}

void TestInitialAllocation() {
    auto state = State(0u, 0u, 0u);
    Runtime runtime;
    Check(pal4::MinimapObjectCollection::InsertGroups(
              0x5000u, state, 0u, 1u, 0x9000u,
              runtime) == 0u &&
              state.ReadDword(4u) == 0xa000u &&
              state.ReadDword(8u) == 0xa014u &&
              state.ReadDword(12u) == 0xa014u &&
              runtime.words[0xa000u] == 9u &&
              runtime.events == std::vector<std::string>{
                  "alloc:20", "copy:40960:36864", "free:0"},
          "empty collection allocates one record and returns old null begin");
}

void TestReallocation() {
    auto state = State(0x6000u, 0x6028u, 0x6028u);
    Runtime runtime;
    Check(pal4::MinimapObjectCollection::InsertGroups(
              0x5000u, state, 0x6014u, 1u, 0x9000u,
              runtime) == 0x6000u &&
              state.ReadDword(4u) == 0xa000u &&
              state.ReadDword(8u) == 0xa03cu &&
              state.ReadDword(12u) == 0xa050u &&
              runtime.words[0xa000u] == 1u &&
              runtime.words[0xa014u] == 9u &&
              runtime.words[0xa028u] == 2u,
          "reallocation doubles capacity and inserts between copied groups");
    Check(runtime.events == std::vector<std::string>{
              "alloc:80", "copy:40960:24576",
              "copy:40980:36864", "copy:41000:24596",
              "cleanup:24576", "cleanup:24596", "free:24576"},
          "new prefix, insertion and suffix copy precede old cleanup");
}

void TestInPlaceShortSuffix() {
    auto state = State(0x6000u, 0x603cu, 0x608cu);
    Runtime runtime;
    Check(pal4::MinimapObjectCollection::InsertGroups(
              0x5000u, state, 0x6028u, 2u, 0x9000u,
              runtime) == 0x5000u &&
              state.ReadDword(8u) == 0x6064u &&
              runtime.words[0x6050u] == 3u &&
              runtime.words[0x603cu] == 9u &&
              runtime.words[0x6028u] == 9u,
          "short suffix copies beyond end and overwrites insertion slot");
    Check(runtime.events == std::vector<std::string>{
              "copy:24656:24616", "copy:24636:36864",
              "assign:24620:36868"},
          "short suffix follows deep-copy then nested assignment order");
}

void TestInPlaceLongSuffixAndNoop() {
    auto state = State(0x6000u, 0x603cu, 0x6050u);
    Runtime runtime;
    Check(pal4::MinimapObjectCollection::InsertGroups(
              0x5000u, state, 0x6000u, 1u, 0x9000u,
              runtime) == 20u &&
              state.ReadDword(8u) == 0x6050u &&
              runtime.words[0x6000u] == 9u &&
              runtime.words[0x6014u] == 1u &&
              runtime.words[0x6028u] == 2u &&
              runtime.words[0x603cu] == 3u,
          "long suffix shifts backward and inserts one source record");
    Check(runtime.events == std::vector<std::string>{
              "copy:24636:24616", "assign:24620:24600",
              "assign:24600:24580", "assign:24580:36868"},
          "long suffix uses tail copy, reverse assignments, then source");
    runtime.events.clear();
    Check(pal4::MinimapObjectCollection::InsertGroups(
              0x5000u, state, 0x6000u, 0u, 0x9000u,
              runtime) == 0u && runtime.events.empty() &&
              state.ReadDword(8u) == 0x6050u,
          "zero insertion leaves collection unchanged");
}

}  // namespace

int main() {
    TestInitialAllocation();
    TestReallocation();
    TestInPlaceShortSuffix();
    TestInPlaceLongSuffixAndNoop();
    return EXIT_SUCCESS;
}
