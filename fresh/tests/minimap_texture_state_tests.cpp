#include "pal4/minimap_texture_state.h"

#include <algorithm>
#include <array>
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

class Runtime final : public pal4::IMinimapTextureRuntime {
public:
    std::vector<std::string> events;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::uint32_t pixels = 0x11u;
    std::uint32_t grid_depth = 0x77u;
    std::uint32_t image_depth = 0x77u;
    std::uint32_t image_pixels = 0x2000u;
    std::uint32_t image_stride = 5u;
    std::uint32_t grid_stride = 8u;
    std::int32_t grid_width = 6;
    std::int32_t grid_height = 3;
    std::uint32_t raster = 0x22u;
    std::uint32_t image = 0x33u;
    std::uint32_t allocated_image = 0x44u;
    std::uint32_t texture = 0x55u;
    std::uint32_t released_image = 0x66u;

    std::uint32_t GridPixelBuffer(std::uint32_t grid) override {
        Check(grid == 0x80u, "pixel-buffer grid argument");
        events.emplace_back("pixels");
        return pixels;
    }
    std::uint32_t GridField12(std::uint32_t grid) override {
        Check(grid == 0x80u, "grid field argument");
        events.emplace_back("field12");
        return grid_depth;
    }
    std::uint32_t GridStride(std::uint32_t grid) override {
        Check(grid == 0x80u, "grid stride argument");
        events.emplace_back("grid-stride");
        return grid_stride;
    }
    std::int32_t GridWidth(std::uint32_t grid) override {
        Check(grid == 0x80u, "grid width argument");
        events.emplace_back("grid-width");
        return grid_width;
    }
    std::int32_t GridHeight(std::uint32_t grid) override {
        Check(grid == 0x80u, "grid height argument");
        events.emplace_back("grid-height");
        return grid_height;
    }
    std::uint32_t ImageField(std::uint32_t value,
                             std::uint32_t offset) override {
        Check(value != 0u, "image field object");
        events.emplace_back("image-field:" + std::to_string(offset));
        if (offset == 0x0cu) return image_depth;
        if (offset == 0x10u) return image_stride;
        if (offset == 0x14u) return image_pixels;
        Check(false, "unexpected image offset");
        return 0u;
    }
    void ZeroBytes(std::uint32_t address, std::uint32_t size) override {
        events.emplace_back("zero:" + std::to_string(address));
        for (std::uint32_t i = 0; i < size; ++i) bytes[address + i] = 0u;
    }
    void CopyBytes(std::uint32_t destination, std::uint32_t source,
                   std::uint32_t size) override {
        events.emplace_back("copy:" + std::to_string(destination) + ":" +
                            std::to_string(source) + ":" +
                            std::to_string(size));
        for (std::uint32_t i = 0; i < size; ++i) {
            bytes[destination + i] = bytes[source + i];
        }
    }
    std::uint32_t CreateRaster(std::uint32_t width, std::uint32_t height,
                               std::uint32_t field,
                               std::uint32_t mode) override {
        Check(width == 12u && height == 34u && field == 0x77u &&
                  (mode == 0x504u || mode == 0x604u),
              "raster dimensions and mode");
        events.emplace_back("raster:" + std::to_string(mode));
        return raster;
    }
    std::uint32_t CreateImage(std::uint32_t width, std::uint32_t height,
                              std::uint32_t field) override {
        Check(width == 12u && height == 34u && field == 0x77u,
              "image dimensions and field");
        events.emplace_back("image");
        return image;
    }
    std::uint32_t AllocateImagePixels(std::uint32_t value) override {
        Check(value == image, "pixel allocation image argument");
        events.emplace_back("allocate-pixels");
        return allocated_image;
    }
    std::uint32_t CreateTexture(std::uint32_t argument) override {
        Check(argument == 0u, "texture creation argument");
        events.emplace_back("texture");
        return texture;
    }
    void SetTextureRaster(std::uint32_t value,
                          std::uint32_t raster_value) override {
        events.emplace_back("set-raster:" + std::to_string(value) + ":" +
                            std::to_string(raster_value));
    }
    std::uint32_t ReleaseImageData(std::uint32_t value) override {
        events.emplace_back("release-image:" + std::to_string(value));
        return released_image;
    }
    void DestroyImage(std::uint32_t value) override {
        events.emplace_back("destroy-image:" + std::to_string(value));
    }
    void DestroyRaster(std::uint32_t value) override {
        events.emplace_back("destroy-raster:" + std::to_string(value));
    }
    void DestroyTexture(std::uint32_t value) override {
        events.emplace_back("destroy-texture:" + std::to_string(value));
    }
    void SetRasterFromImage(std::uint32_t raster_value,
                            std::uint32_t image_value) override {
        events.emplace_back("commit:" + std::to_string(raster_value) +
                            ":" + std::to_string(image_value));
    }
};

constexpr std::array<std::uint32_t, 4> kDimensions{3u, 4u, 12u, 34u};

void TestSuccessfulCreationAndReset() {
    Runtime runtime;
    pal4::MinimapTextureState state;
    Check(state.CreateFromGrid(kDimensions, 0x80u, true, runtime),
          "valid grid creates resources");
    Check(state.Record().region == kDimensions &&
              state.Record().grid == 0x80u &&
              state.Record().raster == 0x22u &&
              state.Record().image == 0x44u &&
              state.Record().texture == 0x55u,
          "observed fields are stored");
    Check(runtime.events == std::vector<std::string>{
              "pixels", "field12", "raster:1284", "field12", "image",
              "allocate-pixels", "texture", "set-raster:85:34"},
          "resource creation order and 0x504 mode");
    runtime.events.clear();
    state.ResetThunk(runtime);
    Check(runtime.events == std::vector<std::string>{
              "release-image:68", "destroy-image:102",
              "destroy-raster:34", "set-raster:85:0",
              "destroy-texture:85"},
          "reset thunk releases image, raster, then texture");
    Check(state.Record().image == 0u && state.Record().raster == 0u &&
              state.Record().texture == 0u && state.Record().grid == 0x80u,
          "reset clears resource pointers but keeps grid");
}

void TestGuardsAndInitialization() {
    pal4::MinimapTextureRecord record{};
    record.enabled = 1u;
    record.region = {8u, 9u, 10u, 11u};
    record.grid = 0x90u;
    record.raster = 0x91u;
    record.image = 0x92u;
    record.texture = 0x93u;
    pal4::MinimapTextureState state(record);
    Runtime runtime;
    Check(!state.CreateFromGrid(kDimensions, 0u, false, runtime) &&
              runtime.events.empty() && state.Record().raster == 0x91u,
          "null grid returns before cleanup");
    runtime.pixels = 0u;
    auto& initialized = state.Initialize(kDimensions, 0x80u, false, runtime);
    Check(&initialized == &state && runtime.events ==
              std::vector<std::string>{"pixels"} &&
              state.Record().enabled == 0u &&
              state.Record().raster == 0u &&
              state.Record().image == 0u &&
              state.Record().texture == 0u &&
              state.Record().region == record.region &&
              state.Record().grid == record.grid,
          "initializer clears observed fields and ignores failed create");
}

void TestFailureStages() {
    for (int stage = 0; stage < 4; ++stage) {
        Runtime runtime;
        if (stage == 0) runtime.raster = 0u;
        if (stage == 1) runtime.image = 0u;
        if (stage == 2) runtime.allocated_image = 0u;
        if (stage == 3) runtime.texture = 0u;
        pal4::MinimapTextureState state;
        Check(!state.CreateFromGrid(kDimensions, 0x80u, false, runtime),
              "failed external allocation returns false");
        Check(state.Record().region == kDimensions &&
                  state.Record().grid == 0x80u &&
                  state.Record().texture == 0u,
              "failed creation retains dimensions and grid");
        Check(runtime.events[2] == "raster:1540",
              "false mode passes 0x604");
        if (stage == 0) {
            Check(runtime.events.size() == 3u,
                  "raster failure does not call cleanup again");
        } else if (stage == 1) {
            Check(runtime.events.back() == "destroy-raster:34" &&
                      state.Record().raster == 0u,
                  "image failure cleans raster");
        } else if (stage == 2) {
            Check(runtime.events.back() == "destroy-raster:34" &&
                      state.Record().image == 0u,
                  "pixel allocation failure replaces image pointer before cleanup");
        } else {
            Check(std::find(runtime.events.begin(), runtime.events.end(),
                            "set-raster:0:34") != runtime.events.end() &&
                      runtime.events.back() == "destroy-raster:34",
                  "null texture still receives raster assignment before cleanup");
        }
    }
}

void TestResetWithoutReturnedImage() {
    pal4::MinimapTextureRecord record{};
    record.image = 0x90u;
    record.raster = 0x91u;
    record.texture = 0x92u;
    Runtime runtime;
    runtime.released_image = 0u;
    pal4::MinimapTextureState state(record);
    state.Reset(runtime);
    Check(runtime.events == std::vector<std::string>{
              "release-image:144", "destroy-raster:145",
              "set-raster:146:0", "destroy-texture:146"},
          "null image release skips destroy and continues cleanup");
}

void TestPositionUpdate() {
    pal4::MinimapTextureRecord record{};
    record.region = {3u, 4u, 12u, 34u};
    pal4::MinimapTextureState state(record);
    Check(!state.UpdatePosition(3, 4) && state.Record().region == record.region,
          "same position does not change the region");
    Check(state.UpdatePosition(-2, 4) &&
              state.Record().region[0] == 0xfffffffeu &&
              state.Record().region[1] == 4u &&
              state.Record().region[2] == 12u &&
              state.Record().region[3] == 34u,
          "changed X stores signed bits and preserves dimensions");
    Check(state.UpdatePosition(-2, 8) && state.Record().region[1] == 8u,
          "changed Y also updates position");
}

void TestRefreshAndClipping() {
    Runtime runtime;
    runtime.grid_depth = 8u;
    runtime.image_depth = 8u;
    runtime.pixels = 0x1000u;
    for (std::uint32_t i = 0; i < 24u; ++i) {
        runtime.bytes[0x1000u + i] = static_cast<std::uint8_t>(i + 1u);
    }
    pal4::MinimapTextureRecord record{};
    record.region = {2u, 1u, 3u, 3u};
    record.grid = 0x80u;
    record.raster = 0x22u;
    record.image = 0x44u;
    pal4::MinimapTextureState state(record);
    Check(state.RefreshFromGrid(runtime) == 0u && state.Record().enabled == 1u,
          "refresh marks valid image as updated");
    Check(runtime.bytes[0x2000u] == 11u &&
              runtime.bytes[0x2001u] == 12u &&
              runtime.bytes[0x2002u] == 13u &&
              runtime.bytes[0x2003u] == 0u &&
              runtime.bytes[0x2005u] == 19u &&
              runtime.bytes[0x200au] == 0u,
          "visible rows copy and out-of-bounds rows stay zero");
    Check(runtime.events == std::vector<std::string>{
              "pixels", "field12", "image-field:12", "pixels",
              "image-field:16", "image-field:20", "grid-stride",
              "grid-height", "grid-width", "zero:8192",
              "copy:8192:4106:3", "zero:8197", "copy:8197:4114:3",
              "zero:8202"},
          "refresh queries layout then clears each row before copy");

    runtime.events.clear();
    runtime.bytes[0x2000u] = 0xffu;
    record.region = {0xfffffffeu, 0xffffffffu, 5u, 3u};
    pal4::MinimapTextureState negative(record);
    Check(negative.RefreshFromGrid(runtime) == 0u &&
              runtime.bytes[0x2000u] == 0u &&
              runtime.bytes[0x2002u] == 0u &&
              runtime.bytes[0x2007u] == 1u &&
              runtime.bytes[0x200cu] == 9u &&
              runtime.events[9] == "zero:8192" &&
              runtime.events[10] == "zero:8197" &&
              runtime.events[11] == "copy:8199:4096:3",
          "negative X shifts destination; negative Y row remains blank");
}

void TestRefreshGuards() {
    pal4::MinimapTextureRecord record{};
    record.region = {0u, 0u, 2u, 2u};
    Runtime runtime;
    pal4::MinimapTextureState empty(record);
    Check(empty.RefreshFromGrid(runtime) == 0u && runtime.events.empty(),
          "missing image exits first");
    record.image = 0x44u;
    pal4::MinimapTextureState no_grid(record);
    Check(no_grid.RefreshFromGrid(runtime) == 0x44u &&
              runtime.events.empty(), "missing grid returns image pointer");
    record.grid = 0x80u;
    pal4::MinimapTextureState no_raster(record);
    Check(no_raster.RefreshFromGrid(runtime) == 0u &&
              runtime.events.empty(), "missing raster exits before grid read");
    record.raster = 0x22u;
    runtime.pixels = 0u;
    pal4::MinimapTextureState no_pixels(record);
    Check(no_pixels.RefreshFromGrid(runtime) == 0u &&
              runtime.events == std::vector<std::string>{"pixels"},
          "missing pixel buffer exits before image access");

    runtime.events.clear();
    runtime.pixels = 0x1000u;
    runtime.grid_depth = 8u;
    runtime.image_depth = 32u;
    pal4::MinimapTextureState mismatch(record);
    Check(mismatch.RefreshFromGrid(runtime) == 8u &&
              mismatch.Record().enabled == 0u &&
              runtime.events == std::vector<std::string>{
                  "pixels", "field12", "image-field:12"},
          "depth mismatch preserves flag and returns grid depth");

    runtime.events.clear();
    runtime.image_depth = 8u;
    record.region[3] = 0u;
    pal4::MinimapTextureState no_rows(record);
    Check(no_rows.RefreshFromGrid(runtime) == 0x1000u &&
              no_rows.Record().enabled == 1u &&
              std::find_if(runtime.events.begin(), runtime.events.end(),
                           [](const std::string& event) {
                               return event.starts_with("zero:");
                           }) == runtime.events.end(),
          "zero rows sets flag without touching pixels");
}

void TestRasterCommit() {
    Runtime runtime;
    pal4::MinimapTextureRecord record{};
    record.enabled = 1u;
    record.image = 0x44u;
    record.raster = 0x22u;
    pal4::MinimapTextureState state(record);
    state.CommitRaster(runtime);
    Check(state.Record().enabled == 0u &&
              runtime.events == std::vector<std::string>{"commit:34:68"},
          "commit clears flag and forwards raster and image");
    state.CommitRaster(runtime);
    Check(runtime.events.size() == 1u,
          "cleared flag prevents repeated raster upload");
    for (int missing = 0; missing < 3; ++missing) {
        auto guarded = record;
        if (missing == 0) guarded.image = 0u;
        if (missing == 1) guarded.raster = 0u;
        if (missing == 2) guarded.enabled = 0u;
        pal4::MinimapTextureState unchanged(guarded);
        unchanged.CommitRaster(runtime);
        Check(unchanged.Record().enabled == guarded.enabled &&
                  runtime.events.size() == 1u,
              "each missing prerequisite skips raster upload");
    }
}

}  // namespace

int main() {
    TestSuccessfulCreationAndReset();
    TestGuardsAndInitialization();
    TestFailureStages();
    TestResetWithoutReturnedImage();
    TestPositionUpdate();
    TestRefreshAndClipping();
    TestRefreshGuards();
    TestRasterCommit();
    return EXIT_SUCCESS;
}
