#include "pal4/minimap_manager.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <new>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IMinimapManagerSnapshotRuntime {
public:
    pal4::MinimapManagerObjectState manager;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;
    std::vector<std::uint32_t> allocations{
        0x8000u, 0x9000u, 0xa000u, 0x9010u, 0xa010u,
        0x9020u, 0xa020u};
    std::size_t allocation_index = 0u;

    pal4::MinimapManagerObjectState& AccessManager(
        std::uint32_t address) override {
        Check(address == 0x1000u, "manager address");
        events.emplace_back("manager");
        return manager;
    }
    std::uint32_t Allocate(std::uint32_t bytes_requested) override {
        Check(allocation_index < allocations.size(), "allocation count");
        events.emplace_back("alloc:" + std::to_string(bytes_requested));
        return allocations[allocation_index++];
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
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    void EncodeGridMask(std::uint32_t grid,
                        std::uint32_t mask) override {
        events.emplace_back("mask:" + std::to_string(grid));
        Check(words[mask + 4u] == 0u && words[mask + 8u] == 0u &&
                  words[mask + 12u] == 0u && bytes[mask] == 0x5au,
              "mask record initialized before encoding");
    }
};

std::uint32_t Capture(Runtime& runtime) {
    return pal4::MinimapManagerObject::CaptureGridData(
        0x1000u, 0x33u, 0x5au, runtime);
}

void TestNonMaskMode() {
    Runtime runtime;
    runtime.manager.WriteDword(0x18u, 0x5000u);
    runtime.manager.WriteDword(0x1cu, 0x5008u);
    runtime.manager.WriteDword(0x24u, 0x2222u);
    runtime.manager.WriteDword(0x50u, 1u);
    runtime.words[0x5000u] = 0x1111u;
    runtime.words[0x5004u] = 0x2222u;
    Check(Capture(runtime) == 0x8000u &&
              runtime.words[0x8000u] ==
                  pal4::MinimapCollection::kVtableAddress &&
              runtime.bytes[0x8004u] == 0x33u &&
              runtime.words[0x8014u] == 1u &&
              runtime.words[0x8008u] == 0u &&
              runtime.events ==
                  std::vector<std::string>{"alloc:24", "manager"},
          "non-mask mode captures selected index without mask allocation");
}

void TestThreeMasksAndGrowth() {
    Runtime runtime;
    runtime.manager.WriteDword(0x18u, 0x5000u);
    runtime.manager.WriteDword(0x1cu, 0x500cu);
    runtime.manager.WriteDword(0x24u, 0x3333u);
    runtime.manager.WriteDword(0x50u, 2u);
    runtime.words[0x5000u] = 0x1111u;
    runtime.words[0x5004u] = 0x2222u;
    runtime.words[0x5008u] = 0x3333u;
    Check(Capture(runtime) == 0x8000u &&
              runtime.words[0x8014u] == 2u &&
              runtime.words[0x8008u] == 0xa020u &&
              runtime.words[0x800cu] == 0xa02cu &&
              runtime.words[0x8010u] == 0xa030u,
          "three masks captured with capacity four");
    Check(runtime.words[0xa020u] == 0x9000u &&
              runtime.words[0xa024u] == 0x9010u &&
              runtime.words[0xa028u] == 0x9020u,
          "growth copies prior masks and appends new mask");
    Check(runtime.events == std::vector<std::string>{
              "alloc:24", "manager", "alloc:16", "mask:4369",
              "alloc:4", "free:0", "alloc:16", "mask:8738",
              "alloc:8", "free:40960", "alloc:16", "mask:13107",
              "alloc:16", "free:40976"},
          "allocation, encoding and free order for 1-2-4 growth");
}

void TestEmptyListAndMissingSelection() {
    Runtime runtime;
    runtime.manager.WriteDword(0x50u, 2u);
    Check(Capture(runtime) == 0x8000u &&
              runtime.words[0x8014u] == 0xffffffffu &&
              runtime.words[0x8008u] == 0u &&
              runtime.events ==
                  std::vector<std::string>{"alloc:24", "manager"},
          "empty grid list preserves missing-index sentinel");
    Runtime missing;
    missing.manager.WriteDword(0x18u, 0x5000u);
    missing.manager.WriteDword(0x1cu, 0x5004u);
    missing.manager.WriteDword(0x24u, 0x9999u);
    missing.manager.WriteDword(0x50u, 1u);
    missing.words[0x5000u] = 0x1111u;
    Check(Capture(missing) == 0x8000u &&
              missing.words[0x8014u] == 0xffffffffu,
          "unselected grid records index minus one");
}

void TestAllocationFailure() {
    Runtime runtime;
    runtime.allocations[0] = 0u;
    try {
        (void)Capture(runtime);
        Check(false, "null collection allocation should throw");
    } catch (const std::bad_alloc&) {
        Check(runtime.events ==
                  std::vector<std::string>{"alloc:24"},
              "allocation failure stops before object access");
    }
}

}  // namespace

int main() {
    TestNonMaskMode();
    TestThreeMasksAndGrowth();
    TestEmptyListAndMissingSelection();
    TestAllocationFailure();
    return EXIT_SUCCESS;
}
