#include "pal4/game_world_asset_loader.h"

#include <cstdlib>
#include <iostream>
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

constexpr std::string_view kPath = "root\\scene\\hero.dff";
constexpr std::string_view kName = "Accelerate\\scene\\hero_d3d9.txd";

class Runtime final : public pal4::IGameWorldAssetRuntime {
public:
    void ReallocateImage(std::uint32_t image) override {
        Check(image == 0x1100u, "nonzero image reaches reallocation");
        events.push_back("image");
    }
    std::uint32_t GetCurrentDictionary() override {
        events.push_back("get_current");
        return 0x2200u;
    }
    bool TextureExists(std::uint32_t dictionary,
                       std::string_view name) override {
        Check(dictionary == 0x3300u && name == kName,
              "texture lookup uses rebuilt resource name");
        events.push_back("exists");
        return existing;
    }
    void SelectTexture(std::uint32_t dictionary,
                       std::string_view name) override {
        Check(dictionary == 0x3300u && name == kName,
              "texture selection uses root dictionary and resource name");
        events.push_back("select");
    }
    std::uint32_t CreateDictionary() override {
        events.push_back("create");
        return created;
    }
    void SetCurrentDictionary(std::uint32_t dictionary) override {
        Check(dictionary == created || dictionary == 0x2200u,
              "current dictionary changes to created or previous");
        events.push_back(dictionary == 0x2200u ? "restore" : "set_created");
    }
    std::uint32_t OpenStream(std::uint32_t kind, std::uint32_t mode,
                             std::string_view path) override {
        Check(kind == 2u && mode == 1u && path == kPath,
              "data stream opens requested path with native type and mode");
        events.push_back("open");
        return stream;
    }
    bool FindChunk(std::uint32_t stream_handle, std::uint32_t chunk_id,
                   std::uint32_t first, std::uint32_t second) override {
        Check(stream_handle == 0x4400u && first == 0u && second == 0u &&
                  chunk_id == expected_chunk,
              "chunk lookup receives asset-specific type and zero arguments");
        events.push_back("find");
        return has_chunk;
    }
    std::uint32_t ReadClump(std::uint32_t stream_handle) override {
        Check(stream_handle == 0x4400u, "clump read uses open stream");
        events.push_back("read_clump");
        return asset;
    }
    std::uint32_t ReadAtomic(std::uint32_t stream_handle) override {
        Check(stream_handle == 0x4400u, "atomic read uses open stream");
        events.push_back("read_atomic");
        return asset;
    }
    void DestroyDictionary(std::uint32_t dictionary) override {
        Check(dictionary == created, "failure destroys newly created dictionary");
        events.push_back("destroy");
    }
    void RemoveTexture(std::uint32_t dictionary,
                       std::string_view name) override {
        Check(dictionary == 0x3300u && name == kName,
              "existing texture is removed by rebuilt name on failure");
        events.push_back("remove");
    }
    bool AddTexture(std::uint32_t dictionary, std::string_view name,
                    std::uint32_t created_dictionary) override {
        Check(dictionary == 0x3300u && name == kName &&
                  created_dictionary == created,
              "successful new dictionary is attached under resource name");
        events.push_back("add");
        return add_succeeds;
    }
    void CloseStream(std::uint32_t stream_handle,
                     std::uint32_t flags) override {
        Check(stream_handle == 0x4400u && flags == 0u,
              "stream closes with zero flags");
        events.push_back("close");
    }

    bool existing = false;
    bool has_chunk = true;
    bool add_succeeds = true;
    std::uint32_t stream = 0x4400u;
    std::uint32_t created = 0x5500u;
    std::uint32_t asset = 0x6600u;
    std::uint32_t expected_chunk = 16u;
    std::vector<std::string> events;
};

void ClumpPaths() {
    Runtime missing_dictionary;
    Check(pal4::GameWorldAssetLoader::LoadClump(
              kPath, 0x1100u, 0u, missing_dictionary) == 0u &&
              missing_dictionary.events ==
                  std::vector<std::string>{"image"},
          "image setup precedes required dictionary gate");

    Runtime created;
    Check(pal4::GameWorldAssetLoader::LoadClump(
              kPath, 0x1100u, 0x3300u, created) == 0x6600u &&
              created.events == std::vector<std::string>{
                  "image", "get_current", "exists", "create", "set_created",
                  "open", "find", "read_clump", "add", "select", "close"},
          "new clump dictionary follows native successful call order");

    Runtime existing;
    existing.existing = true;
    Check(pal4::GameWorldAssetLoader::LoadClump(
              kPath, 0u, 0x3300u, existing) == 0x6600u &&
              existing.events == std::vector<std::string>{
                  "get_current", "exists", "select", "open", "find",
                  "read_clump", "close"},
          "existing texture is selected without creating another dictionary");

    Runtime missing_stream;
    missing_stream.stream = 0u;
    Check(pal4::GameWorldAssetLoader::LoadClump(
              kPath, 0u, 0x3300u, missing_stream) == 0u &&
              missing_stream.events == std::vector<std::string>{
                  "get_current", "exists", "create", "set_created", "open",
                  "destroy", "restore"},
          "stream-open failure destroys new dictionary and restores previous");

    Runtime missing_chunk;
    missing_chunk.has_chunk = false;
    Check(pal4::GameWorldAssetLoader::LoadClump(
              kPath, 0u, 0x3300u, missing_chunk) == 0u &&
              missing_chunk.events == std::vector<std::string>{
                  "get_current", "exists", "create", "set_created", "open",
                  "find", "close"},
          "missing chunk only closes stream and keeps native dictionary state");

    Runtime failed_read;
    failed_read.existing = true;
    failed_read.asset = 0u;
    Check(pal4::GameWorldAssetLoader::LoadClump(
              kPath, 0u, 0x3300u, failed_read) == 0u &&
              failed_read.events == std::vector<std::string>{
                  "get_current", "exists", "select", "open", "find",
                  "read_clump", "remove", "restore", "close"},
          "failed read removes existing texture and restores dictionary");

    Runtime failed_add;
    failed_add.add_succeeds = false;
    Check(pal4::GameWorldAssetLoader::LoadClump(
              kPath, 0u, 0x3300u, failed_add) == 0x6600u &&
              failed_add.events == std::vector<std::string>{
                  "get_current", "exists", "create", "set_created", "open",
                  "find", "read_clump", "add", "close"},
          "failed dictionary attach still returns loaded clump");
}

void AtomicPaths() {
    Runtime atomic;
    atomic.expected_chunk = 11u;
    Check(pal4::GameWorldAssetLoader::LoadAtomic(
              kPath, 0u, 0x3300u, atomic) == 0x6600u &&
              atomic.events == std::vector<std::string>{
                  "get_current", "exists", "create", "set_created", "open",
                  "find", "read_atomic", "add", "select", "close"},
          "atomic loader uses chunk eleven and atomic stream reader");
    Runtime failed;
    failed.expected_chunk = 11u;
    failed.asset = 0u;
    Check(pal4::GameWorldAssetLoader::LoadAtomic(
              kPath, 0u, 0x3300u, failed) == 0u &&
              failed.events == std::vector<std::string>{
                  "get_current", "exists", "create", "set_created", "open",
                  "find", "read_atomic", "destroy", "restore", "close"},
          "atomic read failure destroys new dictionary before restoring");
}

}  // namespace

int main() {
    ClumpPaths();
    AtomicPaths();
}
