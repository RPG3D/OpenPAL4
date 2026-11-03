#include "pal4/minimap_manager.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <tuple>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IMinimapManagerUpdateRuntime {
public:
    pal4::MinimapManagerObjectState state;
    std::vector<std::string> calls;
    std::array<float, 2> point{7.25f, 9.75f};
    std::int32_t width = 100;
    std::int32_t height = 100;
    std::uint32_t pixels = 0x1234u;
    bool radius_changed = false;
    bool position_changed = false;
    std::int32_t coordinate_x = 0;
    std::int32_t coordinate_y = 0;
    std::int32_t texture_x = 0;
    std::int32_t texture_y = 0;

    Runtime() {
        state.WriteDword(0x10u, 0x2000u);
        state.WriteDword(0x24u, 0x3000u);
        state.WriteDword(0x28u, 0x4000u);
        state.WriteDword(0x34u, 5u);
        state.WriteDword(0x38u, 7u);
        state.WriteDword(0x50u, 2u);
    }
    pal4::MinimapManagerObjectState& Access(std::uint32_t address) override {
        Check(address == 0x1000u, "manager address");
        calls.emplace_back("access");
        return state;
    }
    std::uint32_t GridPixelBuffer(std::uint32_t grid) override {
        Check(grid == 0x3000u, "grid pixel address");
        calls.emplace_back("pixels");
        return pixels;
    }
    std::array<float, 2> WorldToGrid(
        std::uint32_t grid, const std::array<float, 3>& world) override {
        Check(grid == 0x3000u &&
                  world == std::array<float, 3>{11.0f, 22.0f, 33.0f},
              "world conversion arguments");
        calls.emplace_back("world-to-grid");
        return point;
    }
    std::int32_t GridWidth(std::uint32_t grid) override {
        Check(grid == 0x3000u, "width grid");
        calls.emplace_back("width");
        return width;
    }
    std::int32_t GridHeight(std::uint32_t grid) override {
        Check(grid == 0x3000u, "height grid");
        calls.emplace_back("height");
        return height;
    }
    std::int32_t FloatToInt(float value) override {
        calls.emplace_back(value == point[0] ? "ftol-x" : "ftol-y");
        return static_cast<std::int32_t>(std::nearbyint(value));
    }
    void SetCoordinates(std::uint32_t object,
                        std::int32_t x, std::int32_t y) override {
        Check(object == 0x2000u, "coordinate object");
        calls.emplace_back("set-coordinates");
        coordinate_x = x;
        coordinate_y = y;
    }
    void CopyThreeDwords(
        std::uint32_t object,
        const std::array<std::uint32_t, 3>& data) override {
        Check(object == 0x2000u &&
                  data == std::array<std::uint32_t, 3>{1u, 2u, 3u},
              "three dword copy arguments");
        calls.emplace_back("copy-three");
    }
    bool ApplyRadius(std::uint32_t grid,
                     std::int32_t x, std::int32_t y) override {
        Check(grid == 0x3000u && x == coordinate_x && y == coordinate_y,
              "radius uses converted coordinates");
        calls.emplace_back("radius");
        return radius_changed;
    }
    bool UpdateTexturePosition(std::uint32_t texture,
                               std::int32_t x, std::int32_t y) override {
        Check(texture == 0x4000u, "texture address");
        calls.emplace_back("position");
        texture_x = x;
        texture_y = y;
        return position_changed;
    }
    void RefreshTexture(std::uint32_t texture) override {
        Check(texture == 0x4000u, "refresh texture address");
        calls.emplace_back("refresh");
    }
};

void Update(Runtime& runtime) {
    pal4::MinimapManagerObject::UpdateGridDisplay(
        0x1000u, {11.0f, 22.0f, 33.0f}, {1u, 2u, 3u}, runtime);
}

void TestGuards() {
    Runtime runtime;
    runtime.state.WriteDword(0x10u, 0u);
    Update(runtime);
    Check(runtime.calls == std::vector<std::string>{"access"},
          "missing coordinate object returns first");
    runtime.calls.clear();
    runtime.state.WriteDword(0x10u, 0x2000u);
    runtime.state.WriteDword(0x24u, 0u);
    Update(runtime);
    Check(runtime.calls == std::vector<std::string>{"access"},
          "missing grid skips pixel lookup");
    runtime.calls.clear();
    runtime.state.WriteDword(0x24u, 0x3000u);
    runtime.pixels = 0u;
    Update(runtime);
    Check(runtime.calls == std::vector<std::string>{"access", "pixels"},
          "missing pixels skips conversion");
}

void TestBounds() {
    Runtime runtime;
    for (const auto [x, y, last] :
         {std::tuple{-0.01f, 5.0f, "world-to-grid"},
          std::tuple{5.0f, -0.01f, "world-to-grid"},
          std::tuple{100.0f, 5.0f, "width"},
          std::tuple{5.0f, 100.0f, "height"},
          std::tuple{std::numeric_limits<float>::quiet_NaN(), 5.0f,
                     "world-to-grid"}}) {
        runtime.point = {x, y};
        runtime.calls.clear();
        Update(runtime);
        Check(runtime.calls.back() == last && runtime.calls.size() <= 5u,
              "invalid point exits at failing bound");
    }
    runtime.point = {0.0f, 0.0f};
    runtime.calls.clear();
    Update(runtime);
    Check(runtime.calls.back() == "position" &&
              runtime.coordinate_x == 0 && runtime.coordinate_y == 0,
          "zero coordinates are valid");
    runtime.point = {99.0f, 99.0f};
    runtime.calls.clear();
    Update(runtime);
    Check(runtime.calls.back() == "position" &&
              runtime.coordinate_x == 99 && runtime.coordinate_y == 99,
          "last grid cell is valid");
}

void TestOrderAndOffsets() {
    Runtime runtime;
    runtime.position_changed = true;
    Update(runtime);
    Check(runtime.calls == std::vector<std::string>{
              "access", "pixels", "world-to-grid", "width", "height",
              "ftol-y", "ftol-x", "set-coordinates", "copy-three",
              "ftol-x", "ftol-y", "radius", "position", "refresh"},
          "conversion and refresh order");
    Check(runtime.coordinate_x == 7 && runtime.coordinate_y == 10 &&
              runtime.texture_x == 5 && runtime.texture_y == 7,
          "coordinates and odd region half widths");
    runtime.state.WriteDword(0x34u, std::bit_cast<std::uint32_t>(-5));
    runtime.state.WriteDword(0x38u, std::bit_cast<std::uint32_t>(-7));
    Update(runtime);
    Check(runtime.texture_x == 9 && runtime.texture_y == 13,
          "negative odd region sizes truncate toward zero");
}

void TestRefreshConditions() {
    Runtime runtime;
    Update(runtime);
    Check(runtime.calls.back() == "position", "unchanged texture skips refresh");
    runtime.calls.clear();
    runtime.radius_changed = true;
    Update(runtime);
    Check(runtime.calls.back() == "refresh", "radius change refreshes texture");
    runtime.calls.clear();
    runtime.radius_changed = false;
    runtime.position_changed = true;
    runtime.state.WriteDword(0x50u, 1u);
    Update(runtime);
    Check(runtime.calls.back() == "refresh" &&
              std::find(runtime.calls.begin(), runtime.calls.end(), "radius") ==
                  runtime.calls.end(),
          "other modes skip radius and refresh changed position");
    runtime.calls.clear();
    runtime.state.WriteDword(0x28u, 0u);
    runtime.state.WriteDword(0x50u, 2u);
    runtime.radius_changed = true;
    Update(runtime);
    Check(runtime.calls.back() == "radius", "missing texture still updates grid");
}

void TestConversionWrapper() {
    Runtime runtime;
    const std::array<float, 3> world{11.0f, 22.0f, 33.0f};
    std::array<float, 2> converted{-1.0f, -1.0f};
    Check(pal4::MinimapManagerObject::ConvertGridCoordinates(
              0x1000u, world, converted, runtime) == &world &&
              converted == runtime.point &&
              runtime.calls ==
                  std::vector<std::string>{"access", "world-to-grid"},
          "grid wrapper forwards conversion and returns input pointer");
    runtime.calls.clear();
    runtime.state.WriteDword(0x24u, 0u);
    Check(pal4::MinimapManagerObject::ConvertGridCoordinates(
              0x1000u, world, converted, runtime) == nullptr &&
              converted == runtime.point &&
              runtime.calls == std::vector<std::string>{"access"},
          "missing grid leaves output untouched");
}

}  // namespace

int main() {
    TestGuards();
    TestBounds();
    TestOrderAndOffsets();
    TestRefreshConditions();
    TestConversionWrapper();
    return EXIT_SUCCESS;
}
