#include "pal4/minimap_object_collection.h"
#include "pal4/minimap_icon_object.h"

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

class Runtime final : public pal4::IMinimapObjectHitRuntime {
public:
    std::map<std::uint32_t, std::uint32_t> words{
        {0x6000u, 0x4000u}, {0x6008u, 0x7000u},
        {0x600cu, 0x7008u}, {0x7000u, 0x8000u},
        {0x7004u, 0x8100u}};
    pal4::MinimapIconState first;
    pal4::MinimapIconState second;
    std::vector<std::string> events;

    Runtime() {
        first.WriteDword(0x24u, 10u);
        first.WriteDword(0x28u, 20u);
        first.WriteDword(0x2cu, 30u);
        first.WriteDword(0x30u, 40u);
        second.WriteDword(0x24u, 50u);
        second.WriteDword(0x28u, 60u);
        second.WriteDword(0x2cu, 20u);
        second.WriteDword(0x30u, 20u);
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        events.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t) override {
        Check(false, "point lookup does not read names");
        return 0u;
    }
    const pal4::MinimapIconState& IconState(
        std::uint32_t icon) override {
        events.emplace_back("icon:" + std::to_string(icon));
        Check(icon == 0x8000u || icon == 0x8100u,
              "icon pointer comes from group array");
        return icon == 0x8000u ? first : second;
    }
};

pal4::MinimapObjectCollectionState State() {
    pal4::MinimapObjectCollectionState state;
    state.WriteDword(4u, 0x6000u);
    state.WriteDword(8u, 0x6014u);
    return state;
}

void TestFirstAndLaterHit() {
    const auto state = State();
    Runtime runtime;
    Check(pal4::MinimapObjectCollection::FindIconAtPoint(
              state, 10, 20, 0x4000u, runtime) == 0x8000u &&
              runtime.events == std::vector<std::string>{
                  "read:24576", "read:24584", "read:24588",
                  "read:28672", "icon:32768"},
          "first icon includes its left and top edge");
    runtime.events.clear();
    Check(pal4::MinimapObjectCollection::FindIconAtPoint(
              state, 70, 80, 0x4000u, runtime) == 0x8100u &&
              runtime.events == std::vector<std::string>{
                  "read:24576", "read:24584", "read:24588",
                  "read:28672", "icon:32768", "read:24588",
                  "read:28676", "icon:33024"},
          "scan continues until inclusive second-icon corner hit");
}

void TestMissingPaths() {
    auto state = State();
    Runtime runtime;
    Check(pal4::MinimapObjectCollection::FindIconAtPoint(
              state, 41, 61, 0x4000u, runtime) == 0u,
          "point outside both icon rectangles returns zero");
    runtime.events.clear();
    Check(pal4::MinimapObjectCollection::FindIconAtPoint(
              state, 10, 20, 0x9999u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{"read:24576"},
          "missing group does not inspect icons");
    runtime.words[0x6008u] = 0u;
    runtime.events.clear();
    Check(pal4::MinimapObjectCollection::FindIconAtPoint(
              state, 10, 20, 0x4000u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{
                  "read:24576", "read:24584"},
          "null icon array exits before reading its end");
    state.WriteDword(4u, 0u);
    runtime.events.clear();
    Check(pal4::MinimapObjectCollection::FindIconAtPoint(
              state, 10, 20, 0x4000u, runtime) == 0u &&
              runtime.events.empty(),
          "null group array exits immediately");
}

}  // namespace

int main() {
    TestFirstAndLaterHit();
    TestMissingPaths();
    return EXIT_SUCCESS;
}
