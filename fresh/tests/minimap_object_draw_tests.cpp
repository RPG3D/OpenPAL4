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

class Runtime final : public pal4::IMinimapObjectDrawRuntime {
public:
    std::map<std::uint32_t, std::uint32_t> words{
        {0x6008u, 0x7000u}, {0x600cu, 0x7008u},
        {0x601cu, 0x7100u}, {0x6020u, 0x7104u},
        {0x7000u, 0x8000u}, {0x7004u, 0x8100u},
        {0x7100u, 0x8200u}};
    std::vector<std::string> events;
    bool shorten_on_first_draw = false;

    std::uint32_t ReadDword(std::uint32_t address) override {
        events.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    void DrawIcon(std::uint32_t icon, std::uint32_t context,
                  std::int32_t x, std::int32_t y,
                  std::uint32_t bounds_address,
                  bool bypass_intersection,
                  std::uint32_t alpha) override {
        Check(context == 0x9000u && x == -12 && y == 34 &&
                  bounds_address == 0xa000u &&
                  !bypass_intersection && alpha == 0u,
              "draw forwards all caller arguments and two zero flags");
        events.emplace_back("draw:" + std::to_string(icon));
        if (shorten_on_first_draw && icon == 0x8000u) {
            words[0x600cu] = 0x7004u;
        }
    }
};

pal4::MinimapObjectCollectionState State() {
    pal4::MinimapObjectCollectionState state;
    state.WriteDword(4u, 0x6000u);
    state.WriteDword(8u, 0x6028u);
    return state;
}

void TestEmptyCollection() {
    pal4::MinimapObjectCollectionState state;
    Runtime runtime;
    Check(pal4::MinimapObjectCollection::DrawIcons(
              state, 0x9000u, -12, 34, 0xa000u, runtime) == 0u &&
              runtime.events.empty(),
          "null group array draws nothing");
    state.WriteDword(4u, 0x6000u);
    state.WriteDword(8u, 0x6000u);
    Check(pal4::MinimapObjectCollection::DrawIcons(
              state, 0x9000u, -12, 34, 0xa000u, runtime) == 0u &&
              runtime.events.empty(),
          "allocated empty group array draws nothing");
}

void TestDrawingOrderAndDynamicBounds() {
    auto state = State();
    Runtime runtime;
    Check(pal4::MinimapObjectCollection::DrawIcons(
              state, 0x9000u, -12, 34, 0xa000u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{
                  "read:24584", "read:24588", "read:28672",
                  "draw:32768", "read:24584", "read:24588",
                  "read:28676", "draw:33024", "read:24584",
                  "read:24588", "read:24604", "read:24608",
                  "read:28928", "draw:33280", "read:24604",
                  "read:24608"},
          "draw visits every icon in group order and re-reads bounds");
    Runtime changed;
    changed.shorten_on_first_draw = true;
    Check(pal4::MinimapObjectCollection::DrawIcons(
              state, 0x9000u, -12, 34, 0xa000u, changed) == 0u &&
              changed.events.size() == 12u &&
              changed.events[3] == "draw:32768" &&
              changed.events[9] == "draw:33280",
          "draw callback changes to group end stop its later icons");
}

}  // namespace

int main() {
    TestEmptyCollection();
    TestDrawingOrderAndDynamicBounds();
    return EXIT_SUCCESS;
}
