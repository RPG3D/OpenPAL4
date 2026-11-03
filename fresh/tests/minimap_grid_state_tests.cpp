#include "pal4/bitset_bounds_error.h"
#include "pal4/minimap_grid_state.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IMinimapGridEffectsRuntime {
public:
    std::vector<std::uint32_t> destroyed;
    std::vector<std::uint32_t> reads;
    std::unordered_map<std::uint32_t, std::uint32_t> values;
    std::unordered_map<std::uint32_t, std::uint8_t> pixels;
    std::vector<std::pair<std::uint32_t, std::uint8_t>> pixel_writes;
    std::vector<double> opacity_inputs;
    std::uint32_t destroy_result = 0x55u;

    std::uint32_t DestroyPluginObject(std::uint32_t address) override {
        destroyed.push_back(address);
        return destroy_result;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        reads.push_back(address);
        return values.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return pixels[address];
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        pixels[address] = value;
        pixel_writes.emplace_back(address, value);
    }
    std::int32_t ConvertOpacity(double value) override {
        opacity_inputs.push_back(value);
        return static_cast<std::int32_t>(value);
    }
};

void TestCleanupAndInitialization() {
    Runtime runtime;
    pal4::MinimapGridState state;
    const std::array<std::uint8_t, 24> data{
        1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u,
        9u, 10u, 11u, 12u, 13u, 14u, 15u, 16u,
        17u, 18u, 19u, 20u, 21u, 22u, 23u, 24u};
    state.SetState(0u, 0x99u);
    Check(state.CleanupThunk(runtime) == 0u &&
              state.Flags() == 0u && runtime.destroyed.empty(),
          "null grid cleanup clears second DWORD without external call");
    state.SetState(0x2000u, 0x77u);
    Check(state.Cleanup(runtime) == 0x55u &&
              state.GridAddress() == 0u && state.Flags() == 0u &&
              runtime.destroyed == std::vector<std::uint32_t>{0x2000u},
          "non-null grid cleanup forwards destroy result and clears fields");
    state.SetState(0x3000u, 0x88u);
    Check(state.Initialize(0x4000u, 0xaau, data, runtime) == 0x4000u &&
              state.GridAddress() == 0x4000u &&
              state.Flags() == 0xaau && state.Data() == data &&
              runtime.destroyed ==
                  std::vector<std::uint32_t>{0x2000u, 0x3000u},
          "initializer cleans prior grid before copying 24 bytes");
    const auto saved_data = state.Data();
    Check(state.Cleanup(runtime) == 0x55u &&
              state.Data() == saved_data &&
              runtime.destroyed.back() == 0x4000u,
          "cleanup preserves copied data bytes");
}

void TestGridFieldAccessors() {
    Runtime runtime;
    pal4::MinimapGridState state;
    Check(state.PixelBuffer(runtime) == 0u &&
              state.Stride(runtime) == 0u &&
              state.ReadField12(runtime) == 0u &&
              state.Width(runtime) == 0u &&
              state.Height(runtime) == 0u && runtime.reads.empty(),
          "all accessors short-circuit a null grid pointer");
    state.SetState(0x5000u, 0u);
    runtime.values = {{0x5014u, 1u}, {0x5010u, 2u},
                      {0x500cu, 3u}, {0x5004u, 4u}, {0x5008u, 5u}};
    Check(state.PixelBuffer(runtime) == 1u &&
              state.Stride(runtime) == 2u &&
              state.ReadField12(runtime) == 3u &&
              state.Width(runtime) == 4u &&
              state.Height(runtime) == 5u &&
              runtime.reads ==
                  std::vector<std::uint32_t>{0x5014u, 0x5010u, 0x500cu,
                                             0x5004u, 0x5008u},
          "grid accessors use observed field offsets");
}

void TestRadiusEffect() {
    Runtime runtime;
    pal4::MinimapGridState state;
    Check(state.ApplyRadius(1, 1, runtime) == 0u &&
              runtime.reads.empty() && runtime.pixel_writes.empty(),
          "null grid short-circuits radius effect");
    state.SetState(0x5000u, 0u);
    runtime.values = {{0x5004u, 64u}, {0x5008u, 64u},
                      {0x5010u, 256u}, {0x5014u, 0x10000u}};
    Check(state.ApplyRadius(-1, 32, runtime) == 0u &&
              state.ApplyRadius(32, -1, runtime) == 0u &&
              state.ApplyRadius(64, 32, runtime) == 0u &&
              state.ApplyRadius(32, 64, runtime) == 0u &&
              runtime.pixel_writes.empty(),
          "negative and upper-bound coordinates leave pixels unchanged");

    const auto pixel = [](std::uint32_t x, std::uint32_t y) {
        return 0x10000u + y * 256u + x * 4u + 3u;
    };
    runtime.pixels[pixel(32u, 32u)] = 255u;
    runtime.pixels[pixel(57u, 32u)] = 200u;
    Check(state.ApplyRadius(32, 32, runtime) == 1u &&
              runtime.pixels[pixel(32u, 32u)] == 254u &&
              runtime.pixels[pixel(52u, 32u)] == 253u &&
              runtime.pixels[pixel(57u, 32u)] == 200u &&
              runtime.pixels[pixel(32u, 57u)] == 126u &&
              runtime.pixels[pixel(2u, 32u)] == 0u &&
              runtime.pixels.find(pixel(62u, 32u)) == runtime.pixels.end() &&
              runtime.pixels.find(pixel(32u, 62u)) == runtime.pixels.end(),
          "radius thresholds, alpha maximum, exclusive end and center override");
    Check(!runtime.opacity_inputs.empty() &&
              runtime.pixel_writes.back() ==
                  std::pair<std::uint32_t, std::uint8_t>{
                      pixel(32u, 32u), 0xfeu},
          "fading uses CRT conversion and center is written last");

    runtime.pixel_writes.clear();
    Check(state.ApplyRadius(0, 0, runtime) == 1u &&
              runtime.pixels[pixel(0u, 0u)] == 0xfeu &&
              runtime.pixels.find(pixel(30u, 0u)) == runtime.pixels.end(),
          "top-left effect clips without visiting the exclusive radius edge");
}

void TestOpaqueMaskEncoding() {
    Runtime runtime;
    pal4::MinimapGridState state;
    std::vector<std::uint32_t> words{0x1234u};
    Check(state.EncodeOpaqueMask(words, runtime) == 0u &&
              words == std::vector<std::uint32_t>{0x1234u},
          "null grid does not append mask data");

    state.SetState(0x5000u, 0u);
    runtime.values = {{0x5004u, 33u}, {0x5008u, 2u},
                      {0x5010u, 136u}, {0x5014u, 0x10000u}};
    const auto alpha = [](std::uint32_t x, std::uint32_t y) {
        return 0x10000u + y * 136u + x * 4u + 3u;
    };
    runtime.pixels[alpha(0u, 0u)] = 0xfeu;
    runtime.pixels[alpha(31u, 0u)] = 0xffu;
    runtime.pixels[alpha(32u, 0u)] = 0xfdu;
    runtime.pixels[alpha(32u, 1u)] = 0xfeu;
    Check(state.EncodeOpaqueMask(words, runtime) == 1u &&
              words == std::vector<std::uint32_t>{
                           0x1234u, 0x80000001u, 0u, 0u, 1u},
          "alpha threshold, row stride, 32-bit chunks and append order");
    Check(runtime.reads == std::vector<std::uint32_t>{
                               0x5008u, 0x5004u, 0x5010u, 0x5014u,
                               0x5010u, 0x5014u},
          "encoder reads dimensions once and pixel storage for each row");

    runtime.reads.clear();
    runtime.values[0x5008u] = 0u;
    Check(state.EncodeOpaqueMask(words, runtime) == 1u &&
              words.size() == 5u && runtime.reads ==
                  std::vector<std::uint32_t>{0x5008u, 0x5004u},
          "zero height succeeds without reading pixel storage");
}

void TestOpaqueMaskApplication() {
    Runtime runtime;
    pal4::MinimapGridState state;
    Check(state.ApplyOpaqueMask({1u}, runtime) == 0u &&
              runtime.pixel_writes.empty(),
          "null grid rejects mask application");

    state.SetState(0x5000u, 0u);
    runtime.values = {{0x5004u, 3u}, {0x5008u, 2u},
                      {0x5010u, 12u}, {0x5014u, 0x10000u}};
    const auto alpha = [](std::uint32_t x, std::uint32_t y) {
        return 0x10000u + y * 12u + x * 4u + 3u;
    };
    Check(state.ApplyOpaqueMask({0b101u, 0b010u}, runtime) == 1u &&
              runtime.pixels[alpha(0u, 0u)] == 0xfeu &&
              runtime.pixels[alpha(2u, 0u)] == 0xfeu &&
              runtime.pixels[alpha(1u, 1u)] == 0xfeu,
          "set mask bits invoke the existing radius operation at each center");

    runtime.pixel_writes.clear();
    Check(state.ApplyOpaqueMask({0u}, runtime) == 0u &&
              runtime.pixel_writes.empty(),
          "input exhaustion at the next row returns failure");
    runtime.values[0x5008u] = 0u;
    Check(state.ApplyOpaqueMask({}, runtime) == 1u,
          "zero height succeeds with empty input");

    runtime.values[0x5008u] = 1u;
    runtime.values[0x5004u] = 33u;
    runtime.values[0x5010u] = 136u;
    Check(state.ApplyOpaqueMask({0u, 1u}, runtime) == 1u &&
              runtime.pixels[0x10000u + 32u * 4u + 3u] == 0xfeu,
          "mask application carries column position into the second word");
    runtime.values[0x5008u] = 2u;
    Check(state.ApplyOpaqueMask({0u, 1u, 0u, 1u}, runtime) == 1u &&
              runtime.pixels[0x10000u + 136u + 32u * 4u + 3u] == 0xfeu &&
              state.ApplyOpaqueMask({0u, 1u, 0u}, runtime) == 0u,
          "wide mask rows restart after the prior row's last word");
}

void TestWorldToGridCoordinates() {
    Runtime runtime;
    pal4::MinimapGridState state;
    std::array<std::uint8_t, 24> data{};
    const float origin_x = 10.0f;
    const float origin_z = -4.0f;
    std::memcpy(data.data() + 12u, &origin_x, sizeof(origin_x));
    std::memcpy(data.data() + 20u, &origin_z, sizeof(origin_z));
    (void)state.Initialize(0u, 2u, data, runtime);
    const std::array<float, 3> position{16.0f, 999.0f, 2.0f};
    std::array<float, 2> output{};
    Check(&state.WorldToGridCoordinates(output, position) == &position &&
              output == std::array<float, 2>{3.0f, 3.0f},
          "world X/Z offsets and signed grid scale produce two coordinates");
    state.SetState(0u, static_cast<std::uint32_t>(-2));
    Check(&state.WorldToGridCoordinates(output, position) == &position &&
              output == std::array<float, 2>{-3.0f, -3.0f},
          "scale uses the signed integer at object offset four");
}

void TestBitsetBoundsError() {
    try {
        pal4::BitsetBoundsError::ThrowInvalidPosition();
        Check(false, "invalid bitset position must throw");
    } catch (const std::out_of_range& error) {
        Check(std::string_view(error.what()) ==
                  "invalid bitset<N> position",
              "bitset exception type and IDA string literal");
    }
}

}  // namespace

int main() {
    TestCleanupAndInitialization();
    TestGridFieldAccessors();
    TestRadiusEffect();
    TestOpaqueMaskEncoding();
    TestOpaqueMaskApplication();
    TestWorldToGridCoordinates();
    TestBitsetBoundsError();
    return EXIT_SUCCESS;
}
