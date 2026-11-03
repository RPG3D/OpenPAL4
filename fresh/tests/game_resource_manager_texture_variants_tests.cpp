#include "pal4/game_resource_manager.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}

class Runtime final : public pal4::GameResourceManagerRuntimeStub {
public:
    std::uint32_t Free(std::uint32_t value) override {
        events.push_back("free"); freed.push_back(value); return 0;
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        events.push_back("allocate"); sizes.push_back(size); return next++;
    }
    void StoreString(std::uint32_t address, std::string_view name) override {
        events.push_back("store"); strings[address] = std::string(name);
    }
    std::string ReadString(std::uint32_t address) override {
        return strings[address];
    }
    void PrepareTextureErrorString() override { events.push_back("prepare"); }
    std::uint32_t ReadTexture(std::string_view name) override {
        events.push_back("read"); read_name = std::string(name); return texture;
    }
    void SetTextureBytes(std::uint32_t value, std::uint8_t a,
                         std::uint8_t b) override {
        events.push_back("bytes");
        Check(value == texture && a == 2 && b == 17, "variant texture flags");
    }
    void ApplyMaterialPipeline4(std::uint32_t clump,
        std::array<std::uint32_t, 3>& args) override {
        events.push_back("pipeline4");
        Check(clump == 0x700 && args ==
              std::array<std::uint32_t, 3>{4, texture, 3},
              "pipeline 4 argument order");
        args[1] = pipeline_texture;
    }
    void ApplyMaterialPipeline5(std::uint32_t clump,
        std::array<std::uint32_t, 6>& args) override {
        events.push_back("pipeline5");
        Check(clump == 0x700 && args ==
              std::array<std::uint32_t, 6>{
                  2, texture, 4, 5, expected_first, expected_second},
              "pipeline 5 argument order and Im3D slots");
        args[1] = pipeline_texture;
    }
    void ApplyMaterialPipeline6(std::uint32_t clump) override {
        events.push_back("pipeline6"); Check(clump == 0x700, "cleanup clump");
    }
    void DestroyTexture(std::uint32_t value) override {
        events.push_back("destroy"); destroyed.push_back(value);
    }
    std::uint32_t CreateIm3D() override {
        events.push_back("create_im3d"); return next_im3d++;
    }
    std::uint32_t CheckTexturePathTarget(std::uint32_t clump,
                                         std::uint32_t target) override {
        events.push_back("check_path");
        Check(clump == 0x700 && target == 0x800, "texture path target");
        return path_result;
    }
    std::uint32_t OpenDataStream(std::uint32_t type, std::uint32_t mode,
                                  std::string_view path) override {
        Check(type == 2 && mode == 1 && path == "_new.eff",
              "effect stream path");
        events.push_back("effects"); return 0;
    }

    std::vector<std::string> events;
    std::vector<std::uint32_t> freed, sizes, destroyed;
    std::map<std::uint32_t, std::string> strings;
    std::string read_name;
    std::uint32_t next{0x100}, next_im3d{0x740};
    std::uint32_t texture{0x500}, pipeline_texture{0x600};
    std::uint32_t expected_first{0x740}, expected_second{0x741};
    std::uint32_t path_result{1};
};

void Texture2() {
    pal4::GameResourceManager manager;
    (void)manager.Construct();
    manager.words[5] = 0x700;
    manager.words[86] = 0x50;
    Runtime runtime;
    Check(manager.LoadTexture2("detail", 3, 4, runtime) &&
          manager.words[86] == 0x100 && manager.words[78] == 1 &&
          runtime.freed == std::vector<std::uint32_t>{0x50} &&
          runtime.sizes == std::vector<std::uint32_t>{7} &&
          runtime.strings[0x100] == "detail" &&
          runtime.destroyed == std::vector<std::uint32_t>{0x600} &&
          runtime.events == std::vector<std::string>{
              "free", "allocate", "store", "prepare", "read", "bytes",
              "pipeline4", "destroy"},
          "variant 2 load and mutable pipeline result");
    runtime.events.clear();
    runtime.texture = 0;
    runtime.pipeline_texture = 0;
    Check(!manager.LoadTexture2("detail", 3, 4, runtime) &&
          manager.words[78] == 0 &&
          runtime.events == std::vector<std::string>{
              "free", "allocate", "store", "prepare", "read", "pipeline4"},
          "variant 2 missing texture clears result flag");
}

void Texture3() {
    pal4::GameResourceManager manager;
    (void)manager.Construct();
    manager.words[5] = 0x700;
    manager.words[88] = 0x80;
    Runtime runtime;
    Check(manager.LoadTexture3(2, "glow", 4, 5, runtime) &&
          manager.words[88] == 0x100 && manager.words[89] == 2 &&
          manager.words[74] == 0x740 && manager.words[75] == 0x741 &&
          manager.words[81] == 1 && runtime.read_name == "glow" &&
          runtime.destroyed == std::vector<std::uint32_t>{0x600},
          "variant 3 creates two Im3D objects and stores state");
    Check(runtime.events == std::vector<std::string>{
              "free", "allocate", "store", "prepare", "read", "bytes",
              "create_im3d", "create_im3d", "pipeline5", "destroy"},
          "variant 3 creation and pipeline order");
    runtime.events.clear();
    runtime.expected_second = 0x740;
    runtime.texture = 0;
    runtime.pipeline_texture = 0;
    Check(!manager.LoadTexture3(2, "glow", 4, 5, runtime) &&
          manager.words[81] == 0 &&
          runtime.events == std::vector<std::string>{
              "free", "allocate", "store", "prepare", "read", "pipeline5"},
          "existing second slot passes first Im3D and skips creation");
}

void CleanupAndReload() {
    pal4::GameResourceManager manager;
    (void)manager.Construct();
    Runtime runtime;
    manager.words[85] = 0x85;
    Check(!manager.CleanupTexture(runtime) && manager.words[85] == 0x85 &&
          runtime.events.empty(), "cleanup requires clump");
    manager.words[5] = 0x700;
    manager.words[76] = 1;
    manager.words[85] = 0x85;
    manager.words[86] = 0x86;
    Check(manager.CleanupTexture(runtime) && manager.words[76] == 0 &&
          manager.words[85] == 0 && manager.words[86] == 0 &&
          runtime.events == std::vector<std::string>{
              "pipeline6", "free", "free"},
          "cleanup pipeline precedes embedded config frees");
    runtime.events.clear();
    manager.words[4] = 0x44;
    Check(!manager.LoadTexture4("", 0x800, runtime) &&
          manager.words[4] == 0 &&
          runtime.events == std::vector<std::string>{"free"},
          "empty name returns after prior-name free");
    runtime.events.clear();
    Check(!manager.LoadTexture4("new", 0, runtime) &&
          runtime.events.empty(), "zero target skips path check");
    runtime.path_result = 0;
    Check(!manager.LoadTexture4("new", 0x800, runtime) &&
          runtime.events == std::vector<std::string>{"check_path"},
          "failed path check returns without allocation");
    runtime.events.clear();
    runtime.path_result = 1;
    manager.words[85] = 0x87;
    Check(manager.LoadTexture4("new", 0x800, runtime) &&
          manager.words[4] != 0 && runtime.strings[manager.words[4]] == "new" &&
          manager.words[85] == 0 &&
          runtime.events == std::vector<std::string>{
              "check_path", "allocate", "store", "pipeline6", "free",
              "allocate", "store", "effects", "free"},
          "successful path reload cleans texture then loads effect data");
}

}  // namespace

int main() {
    Texture2();
    Texture3();
    CleanupAndReload();
}
