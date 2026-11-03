#include "pal4/game_world_cleanup.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IGameWorldCleanupRuntime {
public:
    void FreeExternal(std::uint32_t address) override {
        events.push_back("external");
        external_frees.push_back(address);
    }
    void CleanupDictionary(pal4::GameWorldObjectState&,
                           std::uint32_t word_offset) override {
        events.push_back("dictionary");
        dictionaries.push_back(word_offset);
    }
    void RemoveCamera(std::uint32_t world,
                      std::uint32_t camera) override {
        Check(world == 4000 && camera == 1100,
              "camera detaches from current world");
        events.push_back("camera");
    }
    void DestroyLight(std::uint32_t light) override {
        events.push_back("light");
        lights.push_back(light);
    }
    std::uint32_t GetClumpWorld(std::uint32_t clump) override {
        events.push_back("get_world");
        return clump == 1800 ? 0u : 4000u;
    }
    void RemoveClump(std::uint32_t world,
                     std::uint32_t clump) override {
        Check(world == 4000, "clump detaches from its world");
        events.push_back("remove_clump");
        removed_clumps.push_back(clump);
    }
    void DestroyClump(std::uint32_t clump) override {
        events.push_back("destroy_clump");
        destroyed_clumps.push_back(clump);
    }
    void DestroyWorld(std::uint32_t world) override {
        Check(world == 4000, "cleanup destroys current world");
        events.push_back("destroy_world");
    }
    std::string ReadText(std::uint32_t address) override {
        events.push_back("read_text");
        return strings.at(address);
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        events.push_back("allocate");
        sizes.push_back(bytes);
        return 5000u + static_cast<std::uint32_t>(sizes.size() - 1u) * 100u;
    }
    void WriteText(std::uint32_t destination,
                   std::string_view text) override {
        events.push_back("write_text");
        strings[destination] = std::string(text);
    }
    void RemoveTextureByName(std::uint32_t context,
                             std::string_view name) override {
        Check(context == 77, "texture removal receives context");
        events.push_back("remove_texture");
        removed_names.emplace_back(name);
    }
    std::int32_t Free(std::uint32_t address) override {
        events.push_back("free");
        frees.push_back(address);
        return 0x77;
    }

    std::vector<std::string> events;
    std::vector<std::uint32_t> external_frees;
    std::vector<std::uint32_t> dictionaries;
    std::vector<std::uint32_t> lights;
    std::vector<std::uint32_t> removed_clumps;
    std::vector<std::uint32_t> destroyed_clumps;
    std::vector<std::uint32_t> sizes;
    std::vector<std::uint32_t> frees;
    std::vector<std::string> removed_names;
    std::map<std::uint32_t, std::string> strings{
        {1000u, "base\\scene\\variant\\"}, {3000u, "variant"}};
};

void PopulatedWorld() {
    pal4::GameWorldObjectState state;
    state.words[1] = 1000;
    state.words[2] = 2000;
    state.words[3] = 3000;
    state.words[4] = 4000;
    state.words[5] = 500;
    state.words[6] = 600;
    state.words[11] = 1100;
    state.words[12] = 1;
    state.words[13] = 1300;
    state.words[18] = 1800;
    state.words[28] = 2800;
    state.words[33] = 3300;
    state.words[42] = 4200;
    state.words[43] = 4300;
    for (const auto offset : {45u, 46u, 47u, 48u})
        state.words[offset] = 123;
    Runtime runtime;
    Check(pal4::GameWorldCleanup::Cleanup(state, 77, runtime) == 0x77,
          "cleanup forwards final persistent-buffer free result");
    Check(runtime.external_frees == std::vector<std::uint32_t>({4200, 4300}) &&
              runtime.dictionaries ==
                  std::vector<std::uint32_t>({7, 14, 19, 24, 29, 34}) &&
              runtime.lights == std::vector<std::uint32_t>({500, 600}),
          "external handles, dictionaries and lights follow native order");
    Check(runtime.destroyed_clumps ==
              std::vector<std::uint32_t>({1300, 1800, 2800, 3300}) &&
              runtime.removed_clumps ==
                  std::vector<std::uint32_t>({1300, 2800, 3300}),
          "clumps are detached conditionally and destroyed in slot order");
    const std::string prefix = "base\\scene\\variant\\variant";
    constexpr std::array<std::string_view, 6> suffixes{
        ".bsp", "_trans.dff", "_water.dff", "_clip.dff",
        "_clipNA.dff", "_sky.dff"};
    Check(runtime.removed_names.size() == suffixes.size() &&
              runtime.sizes.size() == suffixes.size(),
          "six world resource names are processed");
    for (std::uint32_t index = 0; index < suffixes.size(); ++index) {
        const auto path = prefix + std::string(suffixes[index]);
        const auto expected = "Accelerate" +
                              path.substr(path.find('\\'),
                                          path.find_last_of('.') - path.find('\\')) +
                              "_d3d9.txd";
        Check(runtime.removed_names[index] == expected &&
                  runtime.sizes[index] == path.size() + 1u &&
                  runtime.strings.at(5000u + index * 100u) == path &&
                  runtime.frees[index] == 5000u + index * 100u,
              "resource path allocation, processing and free agree");
    }
    Check(runtime.frees.size() == 9u && runtime.frees[6] == 1000u &&
              runtime.frees[7] == 2000u && runtime.frees[8] == 3000u,
          "three persistent strings free after resource names");
    for (const auto offset : {1u, 2u, 3u, 4u, 5u, 6u, 11u, 12u,
                              13u, 18u, 28u, 33u, 42u, 43u,
                              45u, 46u, 47u, 48u})
        Check(state.words[offset] == 0u, "cleanup clears observed fields");
    Check(runtime.events.front() == "external" &&
              runtime.events.back() == "free",
          "cleanup begins with external handles and ends with string free");
}

void SparseWorld() {
    pal4::GameWorldObjectState state;
    state.words[11] = 1100;
    state.words[12] = 1;
    state.words[3] = 3000;
    Runtime runtime;
    Check(pal4::GameWorldCleanup::Cleanup(state, 77, runtime) == 0x77 &&
              runtime.sizes.empty() && runtime.removed_names.empty() &&
              runtime.frees == std::vector<std::uint32_t>({3000}) &&
              state.words[11] == 0 && state.words[12] == 0,
          "missing world skips resource names and camera detachment");
    state.words[3] = 0;
    Runtime empty_runtime;
    Check(pal4::GameWorldCleanup::Cleanup(state, 77, empty_runtime) == 0,
          "empty cleanup returns zero");
}

}  // namespace

int main() {
    PopulatedWorld();
    SparseWorld();
}
