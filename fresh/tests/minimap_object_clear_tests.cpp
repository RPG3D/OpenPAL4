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

class Runtime final : public pal4::IMinimapObjectClearRuntime {
public:
    std::map<std::uint32_t, std::uint32_t> words{
        {0x6008u, 0x7000u}, {0x600cu, 0x7008u},
        {0x601cu, 0u}, {0x6020u, 0u},
        {0x6030u, 0x7100u}, {0x6034u, 0x7104u},
        {0x7000u, 0x8000u}, {0x7004u, 0x8100u},
        {0x7100u, 0x8200u}};
    std::vector<std::string> events;

    std::uint32_t ReadDword(std::uint32_t address) override {
        events.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    void DestroyIcon(std::uint32_t icon) override {
        events.emplace_back("destroy:" + std::to_string(icon));
    }
    int CleanupGroup(std::uint32_t group) override {
        events.emplace_back("cleanup:" + std::to_string(group));
        return group == 0x6028u ? 77 : 11;
    }
};

void TestSeveralGroups() {
    pal4::MinimapObjectCollectionState state;
    state.WriteDword(4u, 0x6000u);
    state.WriteDword(8u, 0x603cu);
    state.WriteDword(12u, 0x6050u);
    state.WriteDword(16u, 3u);
    Runtime runtime;
    Check(pal4::MinimapObjectCollection::ClearIcons(
              state, runtime) == 77 &&
              state.ReadDword(4u) == 0x6000u &&
              state.ReadDword(8u) == 0x6000u &&
              state.ReadDword(12u) == 0x6050u &&
              state.ReadDword(16u) == 0u,
          "clear leaves backing array and capacity but resets end and count");
    Check(runtime.events == std::vector<std::string>{
              "read:24584", "read:24588", "read:28672",
              "destroy:32768", "read:24584", "read:24588",
              "read:28676", "destroy:33024", "read:24584",
              "read:24588", "read:24604", "read:24624",
              "read:24628", "read:28928", "destroy:33280",
              "read:24624", "read:24628", "cleanup:24576",
              "cleanup:24596", "cleanup:24616"},
          "all icons are destroyed before any group is cleaned");
}

void TestEmptyCollection() {
    pal4::MinimapObjectCollectionState state;
    state.WriteDword(4u, 0x6000u);
    state.WriteDword(8u, 0x6000u);
    state.WriteDword(16u, 5u);
    Runtime runtime;
    Check(pal4::MinimapObjectCollection::ClearIcons(
              state, runtime) == 0 &&
              state.ReadDword(16u) == 0u && runtime.events.empty(),
          "empty but allocated collection returns zero and resets count");
    state.WriteDword(4u, 0u);
    state.WriteDword(8u, 0u);
    Check(pal4::MinimapObjectCollection::ClearIcons(
              state, runtime) == 0,
          "null collection returns zero");
}

}  // namespace

int main() {
    TestSeveralGroups();
    TestEmptyCollection();
    return EXIT_SUCCESS;
}
