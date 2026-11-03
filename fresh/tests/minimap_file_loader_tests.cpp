#include "pal4/minimap_file_loader.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IMinimapFileRuntime {
public:
    struct InitializedGrid {
        std::uint32_t object;
        std::uint32_t image;
        std::uint32_t scale;
        std::array<std::uint8_t, 24> metadata;
    };

    std::vector<std::string> events;
    std::vector<std::uint8_t> file_bytes;
    std::vector<std::string> image_names;
    std::vector<InitializedGrid> initialized;
    std::uint32_t file_handle = 0x7000u;
    std::uint32_t missing_image_index = 0xffffffffu;
    std::uint32_t next_grid = 0x9000u;
    std::size_t read_cursor = 0u;

    void EnterPackageScope() override { events.emplace_back("enter-pk"); }
    void LeavePackageScope() override { events.emplace_back("leave-pk"); }
    void InitializeXmlDocument() override {
        events.emplace_back("init-xml");
    }
    void DestroyXmlDocument() override {
        events.emplace_back("destroy-xml");
    }
    std::string BasePath() override {
        events.emplace_back("base-path");
        return "C:\\PAL4\\";
    }
    std::uint32_t OpenRead(const std::string& path) override {
        events.emplace_back("open:" + path);
        return file_handle;
    }
    void InitializeFileStream(std::uint32_t handle) override {
        Check(handle == file_handle, "file-stream handle");
        events.emplace_back("init-stream");
    }
    void ReadBytes(std::uint32_t handle, void* destination,
                   std::uint32_t count) override {
        Check(handle == file_handle &&
                  read_cursor + count <= file_bytes.size(),
              "file read bounds");
        events.emplace_back("read:" + std::to_string(count));
        std::memcpy(destination, file_bytes.data() + read_cursor, count);
        read_cursor += count;
    }
    void SetImageBasePath(const std::string& path) override {
        events.emplace_back("image-base:" + path);
    }
    std::uint32_t FindImagePath(const std::string& name) override {
        image_names.push_back(name);
        events.emplace_back("find-image:" + name);
        if (image_names.size() - 1u == missing_image_index) return 0u;
        return 0xa000u + static_cast<std::uint32_t>(image_names.size());
    }
    std::uint32_t AllocateGridObject(std::uint32_t bytes) override {
        Check(bytes == 32u, "grid object allocation size");
        events.emplace_back("allocate-grid");
        const auto result = next_grid;
        next_grid += 0x100u;
        return result;
    }
    void ConstructGridObject(std::uint32_t object) override {
        Check(object != 0u, "construct non-null grid");
        events.emplace_back("construct-grid");
    }
    void InitializeGridObject(
        std::uint32_t object, std::uint32_t image,
        std::uint32_t scale,
        const std::array<std::uint8_t, 24>& metadata) override {
        events.emplace_back("initialize-grid");
        initialized.push_back({object, image, scale, metadata});
    }
    void CloseFile(std::uint32_t handle) override {
        Check(handle == file_handle, "close file handle");
        events.emplace_back("close");
    }
};

void AppendDword(std::vector<std::uint8_t>& bytes, std::uint32_t value) {
    for (std::uint32_t shift = 0u; shift < 32u; shift += 8u)
        bytes.push_back(static_cast<std::uint8_t>(value >> shift));
}

void AppendRecord(Runtime& runtime, std::uint32_t scale,
                  std::uint8_t fill) {
    AppendDword(runtime.file_bytes, scale);
    runtime.file_bytes.insert(runtime.file_bytes.end(), 24u, fill);
}

void TestSuccessfulLoad() {
    Runtime runtime;
    AppendDword(runtime.file_bytes, 2u);
    AppendRecord(runtime, 40u, 0x11u);
    AppendRecord(runtime, 80u, 0x22u);
    pal4::MinimapLayerCollection output{0u, {0xdeadbeefu}};
    Check(&pal4::MinimapFileLoader::Load(
              output, "scenedata", "q01", 0x5au, runtime) == &output &&
              output.header_byte == 0x5au &&
              output.grid_objects ==
                  std::vector<std::uint32_t>{0x9000u, 0x9100u} &&
              runtime.read_cursor == 60u,
          "two layer records replace prior output and preserve input byte");
    Check(runtime.image_names == std::vector<std::string>{
              "MiniMap_0.png", "MiniMap_1.png"} &&
              runtime.initialized.size() == 2u &&
              runtime.initialized[0].image == 0xa001u &&
              runtime.initialized[0].scale == 40u &&
              runtime.initialized[0].metadata.front() == 0x11u &&
              runtime.initialized[1].image == 0xa002u &&
              runtime.initialized[1].scale == 80u &&
              runtime.initialized[1].metadata.back() == 0x22u,
          "image naming, grid scale and all 24 record bytes");
    Check(runtime.events.front() == "enter-pk" &&
              runtime.events[1] == "init-xml" &&
              runtime.events[3] ==
                  "open:C:\\PAL4\\scenedata\\q01\\MiniMap.df" &&
              runtime.events[5] ==
                  "image-base:C:\\PAL4\\scenedata\\q01\\" &&
              runtime.events[runtime.events.size() - 3u] == "close" &&
              runtime.events[runtime.events.size() - 2u] == "leave-pk" &&
              runtime.events.back() == "destroy-xml",
          "file path and lifecycle order");
}

void TestMissingFileAndMissingImage() {
    Runtime absent;
    absent.file_handle = 0u;
    pal4::MinimapLayerCollection output{};
    (void)pal4::MinimapFileLoader::Load(output, "folder", "scene", 7u,
                                        absent);
    Check(output.header_byte == 7u && output.grid_objects.empty() &&
              absent.events.size() == 6u &&
              absent.events[3] ==
                  "open:C:\\PAL4\\folder\\scene\\MiniMap.df" &&
              absent.events[4] == "leave-pk" &&
              absent.events[5] == "destroy-xml",
          "missing data file returns an empty layer collection");

    Runtime missing;
    AppendDword(missing.file_bytes, 3u);
    AppendRecord(missing, 10u, 1u);
    AppendRecord(missing, 20u, 2u);
    AppendRecord(missing, 30u, 3u);
    missing.missing_image_index = 1u;
    (void)pal4::MinimapFileLoader::Load(output, "folder", "scene", 9u,
                                        missing);
    Check(output.header_byte == 9u &&
              output.grid_objects == std::vector<std::uint32_t>{0x9000u} &&
              missing.image_names.size() == 2u &&
              missing.read_cursor == 60u &&
              std::find(missing.events.begin(), missing.events.end(),
                        "close") == missing.events.end() &&
              missing.events[missing.events.size() - 2u] == "leave-pk",
          "missing second image preserves first object and skips file close");
}

void TestZeroAndNegativeCounts() {
    for (const auto count : {0u, 0xffffffffu}) {
        Runtime runtime;
        AppendDword(runtime.file_bytes, count);
        pal4::MinimapLayerCollection output{};
        (void)pal4::MinimapFileLoader::Load(output, "f", "s", 0u,
                                            runtime);
        Check(output.grid_objects.empty() && runtime.read_cursor == 4u &&
                  runtime.image_names.empty() &&
                  std::find(runtime.events.begin(), runtime.events.end(),
                            "close") != runtime.events.end(),
              "nonpositive count skips records and closes the file");
    }
}

}  // namespace

int main() {
    TestSuccessfulLoad();
    TestMissingFileAndMissingImage();
    TestZeroAndNegativeCounts();
    return EXIT_SUCCESS;
}
