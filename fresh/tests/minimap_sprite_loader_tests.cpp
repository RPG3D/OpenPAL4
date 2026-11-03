#include "pal4/minimap_sprite_loader.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IMinimapSpriteRuntime {
public:
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> objects;
    std::vector<std::string> images;
    std::uint32_t minimap_calls{};
    std::uint32_t callback_result{};

    std::uint32_t GetMinimap() override {
        calls.emplace_back("minimap");
        return 0x1000u + ++minimap_calls;
    }
    void CleanupSection(std::uint32_t minimap,
                        std::uint32_t section) override {
        Check(minimap == 0x1001u && section == 1u, "cleanup arguments");
        calls.emplace_back("cleanup");
    }
    std::uint32_t GetObjectManager() override {
        calls.emplace_back("manager");
        return 0x2000u;
    }
    std::uint32_t IterateObjects(
        std::uint32_t manager,
        const std::function<std::uint32_t(std::uint32_t)>& callback,
        std::uint32_t context) override {
        Check(manager == 0x2000u && context == 0u, "iteration arguments");
        calls.emplace_back("iterate");
        for (auto object : objects) {
            callback_result = callback(object);
        }
        return 0x3000u;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    std::array<float, 3> ReadPosition(std::uint32_t address) override {
        Check(address == 0x4000u + 172u, "position offset");
        calls.emplace_back("position");
        return {2.0f, 3.0f, 4.0f};
    }
    std::array<float, 2> ConvertGridCoordinates(
        std::uint32_t minimap,
        const std::array<float, 3>& position) override {
        Check(minimap == 0x1000u + minimap_calls &&
                  position == std::array<float, 3>{2.0f, 3.0f, 4.0f},
              "grid conversion arguments");
        calls.emplace_back("grid");
        return {13.5f, 25.5f};
    }
    std::int32_t ConvertFloatToInt(float value) override {
        calls.emplace_back(value == 25.5f ? "y" : "x");
        return value == 25.5f ? 25 : 13;
    }
    void AddIcon(std::uint32_t minimap, std::uint32_t label,
                 std::uint32_t category, std::string_view image,
                 std::int32_t x, std::int32_t y) override {
        Check(minimap == 0x1000u + minimap_calls && label == 0x4567u &&
                  category == 1u && x == 13 && y == 25,
              "icon arguments");
        calls.emplace_back("icon");
        images.emplace_back(image);
    }
};

void PrepareObject(Runtime& runtime, std::uint32_t type) {
    runtime.words[0x4000u + 432u] = type;
    runtime.words[0x4000u + 388u] = 1u;
    runtime.words[0x4000u + 408u] = 1u;
    runtime.words[0x4000u + 4u] = 0x4567u;
}

void TestIconMappingsAndOrdering() {
    constexpr std::array<std::pair<std::uint32_t, std::string_view>, 8> icons{{
        {2u, "Grocer.PNG"}, {3u, "Entry.PNG"},
        {5u, "Drugstore.png"}, {6u, "Forger.png"},
        {7u, "Inn.png"}, {8u, "Remind.png"},
        {9u, "Restaurant.png"}, {10u, "SaveSpot.png"}}};
    for (const auto& [type, image] : icons) {
        Runtime runtime;
        PrepareObject(runtime, type);
        Check(pal4::MinimapSpriteLoader::LoadObjectIcon(0x4000u, runtime) ==
                  0x4000u, "callback returns original object");
        Check(runtime.images == std::vector<std::string>{std::string(image)},
              "icon filename and capitalization");
        Check(runtime.minimap_calls == 2u, "minimap fetched twice");
        const auto y = std::find(runtime.calls.begin(), runtime.calls.end(), "y");
        const auto x = std::find(runtime.calls.begin(), runtime.calls.end(), "x");
        const auto added = std::find(runtime.calls.begin(), runtime.calls.end(),
                                     "icon");
        Check(y < x && x < added, "Y conversion precedes X and insertion");
    }
}

void TestRejectedAndUnknownObjects() {
    for (const auto offset : {432u, 388u, 408u}) {
        Runtime runtime;
        PrepareObject(runtime, 2u);
        runtime.words[0x4000u + offset] = 0u;
        Check(pal4::MinimapSpriteLoader::LoadObjectIcon(0x4000u, runtime) ==
                  0x4000u && runtime.minimap_calls == 0u &&
                  runtime.images.empty(), "missing property skips conversion");
    }
    Runtime unknown;
    PrepareObject(unknown, 4u);
    Check(pal4::MinimapSpriteLoader::LoadObjectIcon(0x4000u, unknown) ==
              0x4000u && unknown.minimap_calls == 1u &&
              unknown.images.empty() &&
              std::find(unknown.calls.begin(), unknown.calls.end(), "grid") !=
                  unknown.calls.end(),
          "unknown type still converts coordinates");
}

void TestInitialization() {
    Runtime runtime;
    runtime.objects = {0x4000u};
    PrepareObject(runtime, 2u);
    Check(pal4::MinimapSpriteLoader::InitializeAndLoad(runtime) == 0x3000u &&
              runtime.callback_result == 0x4000u,
          "iteration result and callback result");
    Check(runtime.calls.size() >= 4u &&
              runtime.calls[0] == "minimap" &&
              runtime.calls[1] == "cleanup" &&
              runtime.calls[2] == "manager" &&
              runtime.calls[3] == "iterate",
          "initialization call order");

    Runtime thunk_runtime;
    Check(pal4::MinimapSpriteLoader::InitializeThunk(thunk_runtime) ==
              0x3000u &&
              thunk_runtime.calls == std::vector<std::string>{
                  "minimap", "cleanup", "manager", "iterate"},
          "tail thunk forwards to sprite initialization");
}

}  // namespace

int main() {
    TestIconMappingsAndOrdering();
    TestRejectedAndUnknownObjects();
    TestInitialization();
    return EXIT_SUCCESS;
}
