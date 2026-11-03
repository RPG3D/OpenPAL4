#include "pal4/game_resource_manager.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}

class Runtime final : public pal4::GameResourceManagerRuntimeStub {
public:
    std::uint32_t Free(std::uint32_t value) override {
        calls.push_back("free"); freed = value; return value + 1;
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        calls.push_back("allocate"); allocated_size = size; return 0x200;
    }
    void StoreString(std::uint32_t address, std::string_view value) override {
        calls.push_back("store");
        Check(address == 0x200 && value == "stone", "saved texture name");
    }
    void PrepareTextureErrorString() override { calls.push_back("prepare"); }
    std::uint32_t ReadTexture(std::string_view name) override {
        calls.push_back("read"); Check(name == "stone", "read name");
        return read_result;
    }
    void SetTextureBytes(std::uint32_t texture, std::uint8_t first,
                         std::uint8_t second) override {
        calls.push_back("bytes");
        Check(texture == read_result && first == 2 && second == 17,
              "texture flags at +80/+81");
    }
    void ApplyMaterialPipeline(std::uint32_t clump,
        std::array<std::uint32_t, 3>& arguments) override {
        calls.push_back("pipeline");
        Check(clump == 0x700 && arguments ==
              std::array<std::uint32_t, 3>{read_result, 4, 0x730},
              "material pipeline arguments");
        arguments[0] = pipeline_result;
    }
    void DestroyTexture(std::uint32_t value) override {
        calls.push_back("destroy"); destroyed = value;
    }
    void ApplyMaterialPipeline3(std::uint32_t clump, std::uint32_t middle,
                                std::uint32_t texture) override {
        calls.push_back("pipeline3");
        Check(clump == 0x700 && middle == 0 && texture == expected_texture,
              "set texture pipeline arguments");
    }

    std::vector<std::string> calls;
    std::uint32_t freed{}, allocated_size{}, destroyed{};
    std::uint32_t read_result{0x500}, pipeline_result{0x600};
    std::uint32_t expected_texture{};
};

class DualRuntime final : public pal4::GameResourceManagerRuntimeStub {
public:
    std::uint32_t Free(std::uint32_t value) override {
        calls.push_back("free"); freed.push_back(value); return 0;
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        calls.push_back("allocate"); sizes.push_back(size);
        return static_cast<std::uint32_t>(0x200 + sizes.size() - 1);
    }
    void StoreString(std::uint32_t address, std::string_view name) override {
        calls.push_back("store");
        Check((address == 0x200 && name == "diffuse") ||
              (address == 0x201 && name == "mask"), "dual texture names");
    }
    void PrepareTextureErrorString() override { calls.push_back("prepare"); }
    std::uint32_t ReadTexture(std::string_view name) override {
        calls.push_back("read");
        return name == "diffuse" ? 0x501u : second_texture;
    }
    void SetTextureBytes(std::uint32_t texture, std::uint8_t first,
                         std::uint8_t second) override {
        calls.push_back("bytes");
        Check(first == 2 &&
              ((texture == 0x501 && second == 17) ||
               (texture == 0x502 && second == (variant ? 51 : 17))),
              "dual texture bytes");
    }
    void ApplyMaterialPipeline2(std::uint32_t clump,
        std::array<std::uint32_t, 8>& args) override {
        calls.push_back("pipeline2");
        Check(clump == 0x700 && args ==
              std::array<std::uint32_t, 8>{
                  variant, second_texture, 6, 4, 5, 7, 0x501, 0x730},
              "eight-word material pipeline order");
        if (mutate) { args[1] = 0x602; args[6] = 0x601; }
    }
    void DestroyTexture(std::uint32_t texture) override {
        calls.push_back("destroy"); destroyed.push_back(texture);
    }
    std::vector<std::string> calls;
    std::vector<std::uint32_t> freed, sizes, destroyed;
    std::uint32_t variant{1}, second_texture{0x502};
    bool mutate{true};
};

void LoadBranches() {
    pal4::GameResourceManager manager;
    (void)manager.Construct();
    Runtime runtime;
    manager.words[10] = 0;
    Check(!manager.LoadTexture("stone", 4, runtime) && runtime.calls.empty(),
          "load gate skips all texture work");
    manager.words[10] = 1;
    manager.words[5] = 0x700;
    manager.words[73] = 0x730;
    manager.words[85] = 0x100;
    Check(manager.LoadTexture("stone", 4, runtime) &&
          manager.words[85] == 0x200 && manager.words[76] == 1 &&
          runtime.freed == 0x100 && runtime.allocated_size == 6 &&
          runtime.destroyed == 0x600 &&
          runtime.calls == std::vector<std::string>{
              "free", "allocate", "store", "prepare", "read", "bytes",
              "pipeline", "destroy"},
          "loaded texture pipeline uses mutable return slot and call order");
    runtime.calls.clear();
    runtime.read_result = 0;
    runtime.pipeline_result = 0;
    Check(manager.LoadTexture("stone", 4, runtime) &&
          manager.words[76] == 1 &&
          runtime.calls == std::vector<std::string>{
              "free", "allocate", "store", "prepare", "read", "pipeline"},
          "missing texture still applies pipeline and marks loaded");
}

void SetBranches() {
    pal4::GameResourceManager manager;
    (void)manager.Construct();
    Runtime runtime;
    manager.words[5] = 0x700;
    manager.words[76] = 1;
    Check(!manager.SetTexture(0, runtime) && manager.words[76] == 0 &&
          manager.words[77] == 0 && runtime.calls.empty(),
          "zero argument clears load flag before slot gate");
    manager.words[76] = 1;
    Check(!manager.SetTexture(0x888, runtime) && manager.words[76] == 1 &&
          manager.words[77] == 0, "nonzero argument with empty slot returns false");
    manager.words[77] = 0x100;
    runtime.expected_texture = 0x888;
    Check(manager.SetTexture(0x888, runtime) && manager.words[77] == 0x888 &&
          runtime.calls == std::vector<std::string>{"pipeline3"},
          "set texture updates slot and forwards pipeline");
    runtime.expected_texture = 0;
    Check(manager.SetTexture(0, runtime) && manager.words[76] == 0 &&
          manager.words[77] == 0 && runtime.calls.size() == 2,
          "zero texture passes pipeline when old slot exists");
}

void DualTextureBranches() {
    pal4::GameResourceManager manager;
    (void)manager.Construct();
    DualRuntime runtime;
    manager.words[10] = 0;
    Check(!manager.LoadTextures("diffuse", "mask", 1, 4, 5, 6, 7,
                                runtime) && runtime.calls.empty(),
          "dual load disabled gate");
    manager.words[10] = 1;
    manager.words[5] = 0x700;
    manager.words[73] = 0x730;
    manager.words[85] = 0x11;
    manager.words[86] = 0x22;
    Check(manager.LoadTextures("diffuse", "mask", 1, 4, 5, 6, 7,
                               runtime) && manager.words[85] == 0x200 &&
          manager.words[86] == 0x201 && manager.words[77] == 1 &&
          runtime.freed == std::vector<std::uint32_t>{0x11, 0x22} &&
          runtime.sizes == std::vector<std::uint32_t>{8, 5} &&
          runtime.destroyed == std::vector<std::uint32_t>{0x602, 0x601},
          "dual load stores names and destroys modified slots in order");
    Check(runtime.calls == std::vector<std::string>{
              "free", "free", "allocate", "store", "allocate", "store",
              "prepare", "read", "bytes", "read", "bytes", "pipeline2",
              "destroy", "destroy"}, "dual load call order");
    DualRuntime missing;
    missing.variant = 0;
    missing.second_texture = 0;
    missing.mutate = false;
    manager.words[85] = 0;
    manager.words[86] = 0;
    Check(manager.LoadTextures("diffuse", "mask", 0, 4, 5, 6, 7,
                               missing) && missing.destroyed ==
              std::vector<std::uint32_t>{0x501} &&
          missing.calls == std::vector<std::string>{
              "allocate", "store", "allocate", "store", "prepare",
              "read", "bytes", "read", "pipeline2", "destroy"},
          "missing second texture skips its flags and destruction");
}

}  // namespace

int main() {
    LoadBranches();
    SetBranches();
    DualTextureBranches();
}
