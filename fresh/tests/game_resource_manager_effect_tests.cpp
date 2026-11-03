#include "pal4/game_resource_manager.h"

#include <algorithm>
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
    std::uint32_t Allocate(std::uint32_t size) override {
        events.push_back("alloc"); sizes.push_back(size); return next++;
    }
    std::uint32_t Free(std::uint32_t address) override {
        events.push_back("free"); freed.push_back(address); return 0;
    }
    void StoreString(std::uint32_t address, std::string_view value) override {
        strings[address] = std::string(value);
    }
    std::string ReadString(std::uint32_t address) override {
        return strings[address];
    }
    std::uint32_t OpenDataStream(std::uint32_t type, std::uint32_t mode,
                                  std::string_view path) override {
        events.push_back("open");
        Check(type == 2 && mode == 1 && path == "res/model_cloth.eff",
              "effect path and stream mode");
        return stream;
    }
    void ReadStreamWord(std::uint32_t source,
                        std::uint32_t* destination) override {
        Check(source == stream && word_index < values.size(),
              "effect word read count");
        if (word_index < 8) {
            const std::size_t indices[] = {81, 89, 77, 76, 78, 82, 83, 84};
            Check(destination == manager + indices[word_index],
                  "effect field read order");
        }
        *destination = values[word_index++];
        events.push_back("word");
    }
    void ReadStreamBuffer(std::uint32_t source, std::uint32_t destination,
                          std::uint32_t size) override {
        Check(source == stream && buffer_index < 3 && size == 5,
              "effect buffer read size");
        strings[destination] = names[buffer_index++];
        events.push_back("buffer");
    }
    void TerminateBuffer(std::uint32_t address,
                         std::uint32_t index) override {
        Check(index == 4 && strings.count(address) != 0,
              "effect buffer terminator");
        events.push_back("terminate");
    }
    std::uint32_t CloseDataStream(std::uint32_t source) override {
        Check(source == stream, "effect stream close");
        events.push_back("close"); return 0;
    }
    void AddAndProcessScript8() override { events.push_back("script_add"); }
    void CleanupAllScripts() override { events.push_back("script_cleanup"); }
    void PrepareTextureErrorString() override { events.push_back("prepare"); }
    std::uint32_t ReadTexture(std::string_view name) override {
        events.push_back("texture_read");
        return name == "base" ? 0x501 : name == "mask" ? 0x502 : 0x503;
    }
    void SetTextureBytes(std::uint32_t, std::uint8_t,
                         std::uint8_t) override {}
    std::uint32_t CreateIm3D() override { return next_im3d++; }
    void ApplyMaterialPipeline5(std::uint32_t clump,
        std::array<std::uint32_t, 6>& args) override {
        Check(clump == 0x700 && args[0] == 0x22 && args[1] == 0x503 &&
              args[2] == 5 && args[3] == 6, "effect pipeline 5 arguments");
        pipeline_order.push_back(5); events.push_back("pipeline5");
    }
    void ApplyMaterialPipeline2(std::uint32_t clump,
        std::array<std::uint32_t, 8>& args) override {
        Check(clump == 0x700 && args[0] == 0 && args[1] == 0x502 &&
              args[2] == 0x33 && args[3] == 0x55 && args[4] == 1 &&
              args[5] == 0x3f800000u &&
              args[6] == 0x501 && args[7] == 0,
              "effect pipeline 2 arguments");
        pipeline_order.push_back(2); events.push_back("pipeline2");
    }
    void ApplyMaterialPipeline(std::uint32_t clump,
        std::array<std::uint32_t, 3>& args) override {
        Check(clump == 0x700 && args[0] == 0x501 &&
              args[1] == 0x3f800000u, "effect pipeline 1 arguments");
        pipeline_order.push_back(1); events.push_back("pipeline1");
    }
    void ApplyMaterialPipeline4(std::uint32_t clump,
        std::array<std::uint32_t, 3>& args) override {
        Check(clump == 0x700 && args[0] == 0 && args[1] == 0x502 &&
              args[2] == 0x44, "effect pipeline 4 arguments");
        pipeline_order.push_back(4); events.push_back("pipeline4");
    }
    void DestroyTexture(std::uint32_t texture) override {
        destroyed.push_back(texture);
    }

    std::uint32_t* manager{};
    std::uint32_t stream{0x900}, next{0x100}, next_im3d{0x740};
    std::vector<std::uint32_t> values{
        1, 0x22, 1, 1, 1, 0x33, 0x44, 0x55, 5, 5, 5};
    std::vector<std::string> names{"base", "mask", "glow"};
    std::size_t word_index{}, buffer_index{};
    std::map<std::uint32_t, std::string> strings;
    std::vector<std::uint32_t> sizes, freed, destroyed, pipeline_order;
    std::vector<std::string> events;
};

void Setup(pal4::GameResourceManager& manager, Runtime& runtime) {
    (void)manager.Construct();
    manager.words[5] = 0x700;
    manager.words[2] = 0x20;
    manager.words[3] = 0x30;
    manager.words[4] = 0x40;
    runtime.strings[0x20] = "res/";
    runtime.strings[0x30] = "model";
    runtime.strings[0x40] = "cloth";
    runtime.manager = manager.words.data();
}

void GatesAndOpenFailure() {
    pal4::GameResourceManager manager;
    Runtime runtime;
    Setup(manager, runtime);
    manager.words[5] = 0;
    manager.LoadEffectData(runtime);
    Check(runtime.events.empty(), "effect load requires clump");
    manager.words[5] = 0x700;
    manager.words[4] = 0;
    manager.LoadEffectData(runtime);
    Check(runtime.events.empty(), "effect load requires texture name");
    manager.words[4] = 0x40;
    runtime.stream = 0;
    manager.LoadEffectData(runtime);
    Check(runtime.sizes == std::vector<std::uint32_t>{20} &&
          runtime.freed == std::vector<std::uint32_t>{0x100} &&
          runtime.events == std::vector<std::string>{"alloc", "open", "free"},
          "missing effect stream frees path and stops");
}

void FullEffect() {
    pal4::GameResourceManager manager;
    Runtime runtime;
    Setup(manager, runtime);
    manager.LoadEffectData(runtime);
    Check(runtime.word_index == 11 && runtime.buffer_index == 3 &&
          runtime.sizes.size() == 9 && runtime.sizes[0] == 20 &&
          runtime.sizes[1] == 5 && runtime.sizes[2] == 5 &&
          runtime.sizes[3] == 5, "eight fields and three length-prefixed names");
    Check(runtime.pipeline_order == std::vector<std::uint32_t>{5, 2, 1, 4} &&
          runtime.destroyed ==
              std::vector<std::uint32_t>{0x503, 0x502, 0x501, 0x501, 0x502},
          "effect pipeline dispatch and texture cleanup order");
    Check(manager.words[81] == 1 && manager.words[77] == 1 &&
          manager.words[76] == 1 && manager.words[78] == 1 &&
          manager.words[89] == 0x22 && manager.words[82] == 0x33 &&
          manager.words[83] == 0x44 && manager.words[84] == 0x55,
          "effect fields survive texture dispatch");
    Check(runtime.freed == std::vector<std::uint32_t>{
              0x100, 0x105, 0x106, 0x101, 0x102, 0x103} &&
          runtime.events.back() == "free" &&
          std::find(runtime.events.begin(), runtime.events.end(),
                    "script_cleanup") != runtime.events.end(),
          "script cleanup precedes temporary name frees");
    const auto close = std::find(runtime.events.begin(), runtime.events.end(),
                                 "close");
    Check(close != runtime.events.end() &&
          *(close + 1) == "script_add", "stream closes before script processing");
}

void EmptyNames() {
    pal4::GameResourceManager manager;
    Runtime runtime;
    Setup(manager, runtime);
    runtime.values = {1, 0x22, 1, 1, 1, 0x33, 0x44, 0x55,
                      0, 0xffffffffu, 0};
    manager.LoadEffectData(runtime);
    Check(runtime.buffer_index == 0 && runtime.pipeline_order.empty() &&
          runtime.sizes == std::vector<std::uint32_t>{20} &&
          runtime.events.end() !=
              std::find(runtime.events.begin(), runtime.events.end(),
                        "script_cleanup"),
          "nonpositive string lengths skip allocations and dispatch");
}

}  // namespace

int main() {
    GatesAndOpenFailure();
    FullEffect();
    EmptyNames();
}
