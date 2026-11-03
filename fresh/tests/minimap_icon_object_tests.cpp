#include "pal4/minimap_icon_object.h"

#include <array>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IMinimapIconRuntime {
public:
    pal4::MinimapIconState state;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::uint32_t lookup_result = 0x7000u;
    bool throw_on_second_string = false;

    pal4::MinimapIconState& Access(std::uint32_t address) override {
        Check(address == 0x5000u, "icon address");
        return state;
    }
    void ConstructString(std::uint32_t address) override {
        events.emplace_back("construct:" + std::to_string(address));
        if (throw_on_second_string && address == 0x5014u) {
            throw std::runtime_error("string allocation");
        }
    }
    int CleanupString(std::uint32_t address) override {
        events.emplace_back("cleanup:" + std::to_string(address));
        return address == 0x5008u ? 29 : 17;
    }
    void InitializeString(std::uint32_t address,
                          std::string_view value) override {
        Check(address == 0x5014u && value == "Inn.png",
              "stored icon filename");
        events.emplace_back("init-name");
    }
    int DestroyTexture(std::uint32_t texture) override {
        events.emplace_back("destroy-texture:" + std::to_string(texture));
        return 47;
    }
    void ProcessPackageScripts() override {
        events.emplace_back("package-enter");
    }
    void CleanupPackageScripts() override {
        events.emplace_back("package-leave");
    }
    std::uint32_t LookupTexture(std::string_view path) override {
        Check(path == "gamedata\\ui\\minimapIcon\\Inn.png",
              "minimap icon path");
        events.emplace_back("lookup");
        return lookup_result;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        events.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    int Free(std::uint32_t address) override {
        Check(address == 0x5000u, "object free address");
        events.emplace_back("free");
        return 71;
    }
};

class DrawRuntime final : public pal4::IMinimapIconDrawRuntime {
public:
    pal4::MinimapIconState state;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    bool intersects = true;
    std::uint32_t intersection_alpha = 128u;

    DrawRuntime() {
        state.WriteDword(0x14u, 0x8000u);
        state.WriteDword(0x20u, 0x7000u);
        state.WriteDword(0x24u, 10u);
        state.WriteDword(0x28u, 20u);
        state.WriteDword(0x2cu, 30u);
        state.WriteDword(0x30u, 40u);
        state.WriteDword(0x34u, 0x55u);
        words[0x7000u] = 0x7100u;
        words[0x710cu] = 31u;
        words[0x7110u] = 41u;
    }
    pal4::MinimapIconState& Access(std::uint32_t address) override {
        Check(address == 0x5000u, "draw icon address");
        events.emplace_back("access");
        return state;
    }
    bool Intersects(std::uint32_t address,
                    const std::array<std::int32_t, 4>& bounds) override {
        Check(address == 0x5000u &&
                  bounds == std::array<std::int32_t, 4>{3, 4, 100, 100},
              "intersection arguments");
        events.emplace_back("intersects");
        return intersects;
    }
    std::uint32_t IntersectionAlpha(
        std::uint32_t address,
        const std::array<std::int32_t, 4>& bounds) override {
        Check(address == 0x5000u &&
                  bounds == std::array<std::int32_t, 4>{3, 4, 100, 100},
              "intersection alpha arguments");
        events.emplace_back("alpha");
        return intersection_alpha;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        events.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    void LogMissingRaster(std::uint32_t name_pointer) override {
        Check(name_pointer == 0x8000u, "missing raster log name");
        events.emplace_back("log");
    }
    void ReloadTexture(std::uint32_t address,
                       std::uint32_t name_pointer,
                       std::uint32_t property) override {
        Check(address == 0x5000u && name_pointer == 0x8000u &&
                  property == 0x55u,
              "retry arguments");
        events.emplace_back("reload");
    }
    void DrawTransformed(
        std::uint32_t address, std::uint32_t context,
        const std::array<std::uint32_t, 4>& rectangle,
        std::uint32_t alpha) override {
        Check(address == 0x5000u && context == 0x6000u &&
                  rectangle ==
                      std::array<std::uint32_t, 4>{107u, 216u, 30u, 40u} &&
                  alpha == 66u,
              "transformed draw uses old dimensions and provided alpha");
        events.emplace_back("transformed");
    }
    void DrawPlain(
        std::uint32_t context,
        const std::array<std::uint32_t, 4>& rectangle,
        std::uint32_t texture,
        const std::array<std::uint8_t, 3>& rgb,
        std::uint32_t alpha) override {
        Check(context == 0x6000u &&
                  rectangle ==
                      std::array<std::uint32_t, 4>{107u, 216u, 30u, 40u} &&
                  texture == 0x7000u &&
                  rgb == std::array<std::uint8_t, 3>{255u, 255u, 255u} &&
                  alpha == 128u,
              "plain draw uses old dimensions, white and clipping alpha");
        events.emplace_back("plain");
    }
};

class FloatConverter final : public pal4::IMinimapIconFloatConverter {
public:
    std::vector<double> inputs;
    std::int32_t FloatToInt(double value) override {
        inputs.push_back(value);
        return static_cast<std::int32_t>(std::nearbyint(value));
    }
};

void TestConstructorAndExceptionCleanup() {
    Runtime runtime;
    runtime.state.Bytes().fill(0xa5u);
    Check(pal4::MinimapIconObject::Construct(0x5000u, runtime) == 0x5000u &&
              runtime.events == std::vector<std::string>{
                  "construct:20488", "construct:20500"},
          "string construction order and return address");
    Check(runtime.state.ReadDword(0u) == 0xffu &&
              runtime.state.ReadDword(4u) == 0u &&
              runtime.state.Bytes()[0x44u] == 0u &&
              runtime.state.Bytes()[0x45u] == 0xa5u &&
              runtime.state.ReadDword(8u) == 0xa5a5a5a5u,
          "constructor writes exact header and preserves string bytes");
    for (std::uint32_t offset = 0x20u; offset <= 0x40u; offset += 4u) {
        Check(runtime.state.ReadDword(offset) == 0u,
              "constructor clears observed numeric fields");
    }
    Runtime failing;
    failing.throw_on_second_string = true;
    try {
        (void)pal4::MinimapIconObject::Construct(0x5000u, failing);
        Check(false, "second string failure must propagate");
    } catch (const std::runtime_error&) {
        Check(failing.events == std::vector<std::string>{
                  "construct:20488", "construct:20500", "cleanup:20488"},
              "first string cleaned after second constructor failure");
    }
}

void TestTextureAndDestruction() {
    Runtime runtime;
    Check(pal4::MinimapIconObject::ReleaseTexture(0x5000u, runtime) == 0 &&
              runtime.events.empty(), "null texture is a no-op");
    runtime.state.WriteDword(0x20u, 0x7000u);
    Check(pal4::MinimapIconObject::ReleaseTexture(0x5000u, runtime) == 47 &&
              runtime.state.ReadDword(0x20u) == 0u &&
              runtime.events ==
                  std::vector<std::string>{"destroy-texture:28672"},
          "texture release returns RenderWare result and clears field");
    runtime.events.clear();
    runtime.state.WriteDword(0x20u, 0x7000u);
    Check(pal4::MinimapIconObject::Destruct(0x5000u, runtime) == 29 &&
              runtime.events == std::vector<std::string>{
                  "destroy-texture:28672", "cleanup:20500",
                  "cleanup:20488"},
          "destructor releases texture and strings in reverse order");
    runtime.events.clear();
    runtime.state.WriteDword(0x20u, 0x7000u);
    Check(pal4::MinimapIconObject::Destroy(0x5000u, runtime) == 71 &&
              runtime.events == std::vector<std::string>{
                  "destroy-texture:28672", "cleanup:20500",
                  "cleanup:20488", "free"},
          "destroy wrapper's second release sees cleared texture");
}

void TestLoadSuccessAndFailure() {
    Runtime runtime;
    runtime.state.WriteDword(0x20u, 0x6000u);
    runtime.words[0x7000u] = 0x7100u;
    runtime.words[0x710cu] = 64u;
    runtime.words[0x7110u] = 96u;
    Check(pal4::MinimapIconObject::LoadTexture(
              0x5000u, "Inn.png", 0x1234u, runtime) &&
              runtime.state.ReadDword(0x20u) == 0x7000u &&
              runtime.state.ReadDword(0x2cu) == 64u &&
              runtime.state.ReadDword(0x30u) == 96u &&
              runtime.state.ReadDword(0x34u) == 0x1234u,
          "loaded raster dimensions and property stored");
    Check(runtime.events == std::vector<std::string>{
              "destroy-texture:24576", "package-enter", "lookup",
              "read:28672", "read:28940", "read:28944",
              "init-name", "package-leave"},
          "successful loading follows package and texture call order");
    runtime.events.clear();
    runtime.lookup_result = 0x7200u;
    Check(!pal4::MinimapIconObject::LoadTexture(
              0x5000u, "Inn.png", 0x9999u, runtime) &&
              runtime.state.ReadDword(0x20u) == 0x7200u &&
              runtime.state.ReadDword(0x2cu) == 64u &&
              runtime.state.ReadDword(0x34u) == 0x1234u,
          "null raster retains texture pointer and prior dimensions");
    Check(runtime.events == std::vector<std::string>{
              "destroy-texture:28672", "package-enter", "lookup",
              "read:29184", "package-leave"},
          "failed loading skips stored-name initialization");
    runtime.events.clear();
    runtime.lookup_result = 0u;
    Check(!pal4::MinimapIconObject::LoadTexture(
              0x5000u, "Inn.png", 0u, runtime) &&
              runtime.events == std::vector<std::string>{
                  "destroy-texture:29184", "package-enter",
                  "lookup", "package-leave"},
          "missing texture skips raster read");
}

void TestCoordinateAndTripletFields() {
    pal4::MinimapIconState state;
    state.Bytes().fill(0xa5u);
    state.WriteDword(0x2cu, 5u);
    state.WriteDword(0x30u, 7u);
    Check(pal4::MinimapIconObject::SetCenteredCoordinates(
              state, 10, 20) == 3 &&
              state.ReadDword(0x24u) == 8u &&
              state.ReadDword(0x28u) == 17u &&
              state.ReadDword(0x34u) == 0xa5a5a5a5u,
          "centered position uses truncated odd half dimensions");
    state.WriteDword(0x2cu, std::bit_cast<std::uint32_t>(-5));
    state.WriteDword(0x30u, std::bit_cast<std::uint32_t>(-7));
    Check(pal4::MinimapIconObject::SetCenteredCoordinates(
              state, 10, 20) == -3 &&
              state.ReadDword(0x24u) == 12u &&
              state.ReadDword(0x28u) == 23u,
          "negative odd dimensions divide toward zero");
    Check(pal4::MinimapIconObject::CopyTriplet(
              state, {11u, 22u, 33u}) == 33u &&
              state.ReadDword(0x38u) == 11u &&
              state.ReadDword(0x3cu) == 22u &&
              state.ReadDword(0x40u) == 33u &&
              state.Bytes()[0x44u] == 0xa5u,
          "three DWORD copy returns final value and preserves adjacent byte");
}

void TestDrawBranches() {
    constexpr std::array<std::int32_t, 4> bounds{3, 4, 100, 100};
    DrawRuntime runtime;
    Check(pal4::MinimapIconObject::Draw(
              0x5000u, 0x6000u, 100, 200, bounds,
              false, 77u, runtime) &&
              runtime.state.ReadDword(0x2cu) == 31u &&
              runtime.state.ReadDword(0x30u) == 41u &&
              runtime.events == std::vector<std::string>{
                  "access", "intersects", "alpha", "read:28672",
                  "read:28940", "read:28944", "plain"},
          "clipped plain drawing updates dimensions after geometry capture");
    DrawRuntime transformed;
    transformed.state.Bytes()[0x44u] = 1u;
    Check(pal4::MinimapIconObject::Draw(
              0x5000u, 0x6000u, 100, 200, bounds,
              true, 66u, transformed) &&
              transformed.events == std::vector<std::string>{
                  "access", "read:28672", "read:28940",
                  "read:28944", "transformed"},
          "bypass mode forwards alpha to transformed drawing");
}

void TestDrawShortCircuitsAndReload() {
    constexpr std::array<std::int32_t, 4> bounds{3, 4, 100, 100};
    DrawRuntime missing_texture;
    missing_texture.state.WriteDword(0x20u, 0u);
    Check(!pal4::MinimapIconObject::Draw(
              0x5000u, 0x6000u, 100, 200, bounds,
              false, 77u, missing_texture) &&
              missing_texture.events == std::vector<std::string>{"access"},
          "missing texture exits before clipping");
    DrawRuntime outside;
    outside.intersects = false;
    Check(!pal4::MinimapIconObject::Draw(
              0x5000u, 0x6000u, 100, 200, bounds,
              false, 77u, outside) &&
              outside.events ==
                  std::vector<std::string>{"access", "intersects"},
          "non-intersecting rectangle exits before alpha calculation");
    DrawRuntime zero_alpha;
    zero_alpha.intersection_alpha = 0u;
    Check(!pal4::MinimapIconObject::Draw(
              0x5000u, 0x6000u, 100, 200, bounds,
              false, 77u, zero_alpha) &&
              zero_alpha.events ==
                  std::vector<std::string>{"access", "intersects", "alpha"},
          "zero clipping alpha exits before raster read");
    DrawRuntime missing_raster;
    missing_raster.words[0x7000u] = 0u;
    Check(!pal4::MinimapIconObject::Draw(
              0x5000u, 0x6000u, 100, 200, bounds,
              true, 66u, missing_raster) &&
              missing_raster.events == std::vector<std::string>{
                  "access", "read:28672", "log", "reload"} &&
              missing_raster.state.ReadDword(0x2cu) == 30u,
          "missing raster logs and retries without drawing or size update");
    pal4::MinimapIconDrawStub stub;
    Check(!pal4::MinimapIconObject::Draw(
              0u, 0u, 0, 0, bounds, true, 1u, stub),
          "RenderWare draw stub is linkable");
}

void TestRectangleGeometry() {
    pal4::MinimapIconState state;
    state.WriteDword(0x20u, 0x7000u);
    state.WriteDword(0x24u, 10u);
    state.WriteDword(0x28u, 20u);
    state.WriteDword(0x2cu, 30u);
    state.WriteDword(0x30u, 40u);
    Check(pal4::MinimapIconObject::ContainsPoint(state, 10, 20) &&
              pal4::MinimapIconObject::ContainsPoint(state, 40, 60) &&
              !pal4::MinimapIconObject::ContainsPoint(state, 9, 20) &&
              !pal4::MinimapIconObject::ContainsPoint(state, 10, 19) &&
              !pal4::MinimapIconObject::ContainsPoint(state, 41, 20) &&
              !pal4::MinimapIconObject::ContainsPoint(state, 10, 61),
          "point hit test includes all four edges only");
    Check(pal4::MinimapIconObject::FitsWithin(
              state, {10, 20, 30, 40}) &&
              pal4::MinimapIconObject::FitsWithin(
                  state, {0, 0, 100, 100}) &&
              !pal4::MinimapIconObject::FitsWithin(
                  state, {11, 20, 30, 40}) &&
              !pal4::MinimapIconObject::FitsWithin(
                  state, {10, 21, 30, 40}) &&
              !pal4::MinimapIconObject::FitsWithin(
                  state, {10, 20, 29, 40}) &&
              !pal4::MinimapIconObject::FitsWithin(
                  state, {10, 20, 30, 39}),
          "rectangle check requires complete containment");
    state.WriteDword(0x20u, 0u);
    Check(!pal4::MinimapIconObject::FitsWithin(
              state, {0, 0, 100, 100}),
          "missing texture fails containment regardless of geometry");
    state.WriteDword(0x24u, 0x7fffffffu);
    state.WriteDword(0x2cu, 1u);
    Check(!pal4::MinimapIconObject::ContainsPoint(
              state, 0x7fffffff, 20),
          "right edge uses observed 32-bit wraparound arithmetic");
}

void TestClippingAlpha() {
    pal4::MinimapIconState state;
    state.WriteDword(0u, 255u);
    state.WriteDword(4u, 55u);
    state.WriteDword(0x2cu, 20u);
    state.WriteDword(0x30u, 20u);
    constexpr std::array<std::int32_t, 4> bounds{0, 0, 100, 100};
    FloatConverter converter;
    state.WriteDword(0x24u, 40u);
    state.WriteDword(0x28u, 40u);
    Check(pal4::MinimapIconObject::CalculateClipAlpha(
              state, bounds, converter) == 255 &&
              converter.inputs.back() == 255.0,
          "interior rectangle keeps base alpha");
    state.WriteDword(0x24u, 5u);
    Check(pal4::MinimapIconObject::CalculateClipAlpha(
              state, bounds, converter) == 105 &&
              converter.inputs.back() == 105.0,
          "left edge applies horizontal attenuation");
    state.WriteDword(0x24u, 75u);
    Check(pal4::MinimapIconObject::CalculateClipAlpha(
              state, bounds, converter) == 105,
          "right edge applies horizontal attenuation");
    state.WriteDword(0x24u, 40u);
    state.WriteDword(0x28u, 5u);
    Check(pal4::MinimapIconObject::CalculateClipAlpha(
              state, bounds, converter) == 105,
          "top edge applies vertical attenuation");
    state.WriteDword(0x28u, 75u);
    Check(pal4::MinimapIconObject::CalculateClipAlpha(
              state, bounds, converter) == 105,
          "bottom edge applies vertical attenuation");
    state.WriteDword(0x24u, 5u);
    state.WriteDword(0x28u, 10u);
    Check(pal4::MinimapIconObject::CalculateClipAlpha(
              state, bounds, converter) == 105,
          "larger directional penalty determines alpha");
    const auto call_count = converter.inputs.size();
    state.WriteDword(4u, 255u);
    Check(pal4::MinimapIconObject::CalculateClipAlpha(
              state, bounds, converter) == 255 &&
              converter.inputs.size() == call_count,
          "equal base and minimum returns without CRT conversion");
}

}  // namespace

int main() {
    TestConstructorAndExceptionCleanup();
    TestTextureAndDestruction();
    TestLoadSuccessAndFailure();
    TestCoordinateAndTripletFields();
    TestDrawBranches();
    TestDrawShortCircuitsAndReload();
    TestRectangleGeometry();
    TestClippingAlpha();
    return EXIT_SUCCESS;
}
