#include "pal4/game_world_initializer.h"

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

class Runtime final : public pal4::IGameWorldInitializeRuntime {
public:
    void FreeExternal(std::uint32_t) override {
        events.push_back("external_free");
    }
    void CleanupDictionary(pal4::GameWorldObjectState&,
                           std::uint32_t) override {
        events.push_back("cleanup_dict");
    }
    void RemoveCamera(std::uint32_t, std::uint32_t) override {
        events.push_back("remove_camera");
    }
    void DestroyLight(std::uint32_t) override {
        events.push_back("destroy_light");
    }
    std::uint32_t GetClumpWorld(std::uint32_t) override {
        events.push_back("get_clump_world");
        return 0;
    }
    void RemoveClump(std::uint32_t, std::uint32_t) override {
        events.push_back("remove_clump");
    }
    void DestroyClump(std::uint32_t) override {
        events.push_back("destroy_clump");
    }
    void DestroyWorld(std::uint32_t) override {
        events.push_back("destroy_world");
    }
    std::string ReadText(std::uint32_t address) override {
        events.push_back("read_text");
        return strings.at(address);
    }
    void RemoveTextureByName(std::uint32_t,
                             std::string_view) override {
        events.push_back("remove_texture");
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        events.push_back("alloc");
        allocation_sizes.push_back(bytes);
        const auto index = static_cast<std::uint32_t>(allocation_sizes.size() - 1u);
        return index == failing_allocation ? 0u : 1000u + index * 100u;
    }
    void WriteText(std::uint32_t destination, std::string_view text) override {
        Check(destination != 0u, "successful path writes have storage");
        events.push_back("text");
        strings[destination] = std::string(text);
    }
    std::int32_t Free(std::uint32_t address) override {
        events.push_back("free");
        freed.push_back(address);
        return static_cast<std::int32_t>(address);
    }
    std::uint32_t OpenDataStream(std::uint32_t kind, std::uint32_t mode,
                                 std::string_view path) override {
        Check(kind == 2u && mode == 1u &&
                  path == "base\\scene\\variant\\world.flr",
              "world material stream uses current base path");
        events.push_back("open_flr");
        return 0x7000u;
    }
    void ReadData(std::uint32_t stream, pal4::GameWorldObjectState& state,
                  std::uint32_t word_offset, std::uint32_t bytes) override {
        Check(stream == 0x7000u && bytes == 4u,
              "world material read uses four-byte fields");
        events.push_back("read_flr");
        material_offsets.push_back(word_offset);
        state.words[word_offset] = 1u;
    }
    std::uint32_t CloseDataStream(std::uint32_t stream,
                                  std::uint32_t flags) override {
        Check(stream == 0x7000u && flags == 0u,
              "world material stream closes without flags");
        events.push_back("close_flr");
        return 0u;
    }
    void ReallocateImage(std::uint32_t base_path) override {
        Check(base_path == 1200u, "image setup receives base path");
        events.push_back("image");
    }
    void LoadDictionary(std::uint32_t path) override {
        events.push_back("dict");
        dictionaries.push_back(strings.at(path));
    }
    std::uint32_t LoadAtomic(std::uint32_t path,
                             std::uint32_t context) override {
        Check(context == 77u, "world load receives context");
        events.push_back("atomic");
        atomic_path = strings.at(path);
        return world_result;
    }
    std::uint32_t LoadClump(std::uint32_t path,
                            std::uint32_t context) override {
        Check(context == 77u, "clump load receives context");
        events.push_back("clump");
        clump_paths.push_back(strings.at(path));
        const auto index = static_cast<std::uint32_t>(clump_paths.size() - 1u);
        return index == missing_clump ? 0u : 0xa000u + index * 0x100u;
    }
    void SetCurrentDictionary(pal4::GameWorldObjectState&,
                              std::uint32_t word_offset,
                              std::uint32_t object) override {
        events.push_back("current_dict");
        current_dictionaries.push_back({word_offset, object});
    }
    void SetWorldMaterialFlags(std::uint32_t world,
                               std::uint32_t flags) override {
        Check(world == 0x9000u && flags == 1u,
              "world receives its own material flags");
        events.push_back("world_flags");
    }
    void SetClumpMaterialFlags(std::uint32_t clump,
                               std::uint32_t flags) override {
        events.push_back("clump_flags");
        clump_flags.push_back({clump, flags});
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        events.push_back("read");
        return memory.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        events.push_back("write");
        memory[address] = value;
    }
    std::uint32_t ForAllWorldSectors(
        std::uint32_t world, pal4::WorldSectorCallback callback,
        std::uint32_t user_data) override {
        Check(world == 0x9000u &&
                  callback == pal4::WorldSectorCallback::GatherLight &&
                  user_data == 0u,
              "initializer traverses loaded world with light callback");
        events.push_back("lights");
        return world;
    }
    void ForAllSectorMeshes(std::uint32_t,
                            pal4::WorldMeshCallback,
                            std::uint32_t) override {}
    void ProcessSkyAtomicGeometry(std::uint32_t sky_clump,
                                   pal4::GameWorldObjectState&,
                                   std::uint32_t word_offset) override {
        Check(sky_clump == 0xa400u && word_offset == 37u,
              "sky atomic processing receives the expected object facet");
        events.push_back("sky_geometry");
    }

    std::uint32_t failing_allocation = 0xffffffffu;
    std::uint32_t missing_clump = 0xffffffffu;
    std::uint32_t world_result = 0x9000u;
    std::vector<std::uint32_t> allocation_sizes;
    std::vector<std::uint32_t> freed;
    std::vector<std::uint32_t> material_offsets;
    std::vector<std::string> events;
    std::vector<std::string> dictionaries;
    std::vector<std::string> clump_paths;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> current_dictionaries;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> clump_flags;
    std::map<std::uint32_t, std::uint32_t> memory{{0x9008u, 0x20u}};
    std::map<std::uint32_t, std::string> strings;
    std::string atomic_path;
};

pal4::GameWorldObjectState NewState() {
    pal4::GameWorldObjectState state;
    for (const auto index : {10u, 17u, 22u, 27u, 32u, 41u})
        state.words[index] = 1u;
    return state;
}

void SuccessfulLoad() {
    auto state = NewState();
    Runtime runtime;
    Check(pal4::GameWorldInitializer::Initialize(
              state, "base\\", "scene", "variant", 77u, runtime),
          "complete world initialization succeeds");
    Check(runtime.events.front() == "cleanup_dict" &&
              runtime.allocation_sizes.size() == 15u,
          "direct cleanup precedes all fifteen allocations");
    Check(runtime.material_offsets ==
              std::vector<std::uint32_t>({10, 17, 22, 27, 32, 41}),
          "initializer invokes rebuilt world material reader in field order");
    const std::string base = "base\\scene\\variant\\";
    const std::string prefix = base + "variant";
    Check(runtime.strings.at(state.words[2]) == "scene" &&
              runtime.strings.at(state.words[3]) == "variant" &&
              runtime.strings.at(state.words[1]) == base,
          "three persistent paths contain the observed strings");
    Check(runtime.allocation_sizes[0] == 6u &&
              runtime.allocation_sizes[1] == 8u &&
              runtime.allocation_sizes[2] == base.size() + 1u &&
              runtime.allocation_sizes[3] == prefix.size() + 5u,
          "base and first asset allocations retain native sizes");
    Check(runtime.dictionaries == std::vector<std::string>{
              prefix + ".uva", prefix + "_trans.uva",
              prefix + "_clip.uva", prefix + "_clipNA.uva",
              prefix + "_water.uva", prefix + "_sky.uva"},
          "six dictionaries load in IDA order");
    Check(runtime.atomic_path == prefix + ".bsp" &&
              runtime.clump_paths == std::vector<std::string>{
                  prefix + "_trans.dff", prefix + "_clip.dff",
                  prefix + "_clipNA.dff", prefix + "_water.dff",
                  prefix + "_sky.dff"},
          "world and five clumps use the observed resource suffixes");
    Check(state.words[4] == 0x9000u && state.words[13] == 0xa000u &&
              state.words[18] == 0xa100u && state.words[23] == 0xa200u &&
              state.words[28] == 0xa300u && state.words[33] == 0xa400u &&
              state.words[12] == 1u,
          "world/clump fields and completion flag are stored");
    Check(runtime.memory.at(0x9008u) == (0x20u | 0x45u),
          "world material bits are ORed into its native flag word");
    Check(runtime.current_dictionaries ==
              std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {7, 0x9000}, {14, 0xa000}, {19, 0xa100},
                  {24, 0xa200}, {29, 0xa300}, {34, 0xa400}},
          "dictionary slots follow loaded world and clumps");
    Check(runtime.clump_flags.size() == 5u &&
              runtime.clump_flags[4] == std::pair<std::uint32_t, std::uint32_t>{0xa400, 1},
          "all loaded clumps receive their material flags");
    const std::array<std::uint32_t, 12> free_indices{
        4, 3, 5, 6, 9, 10, 11, 12, 7, 8, 13, 14};
    for (std::uint32_t index = 0; index < free_indices.size(); ++index)
        Check(runtime.freed[index] == 1000u + free_indices[index] * 100u,
              "temporary buffers free in native order");
    Check(runtime.events.back() == "free" &&
              runtime.events[21] == "text",
          "resource writes follow all allocations");
}

void FailurePaths() {
    auto state = NewState();
    Runtime base_failure;
    base_failure.failing_allocation = 0u;
    Check(!pal4::GameWorldInitializer::Initialize(
              state, "base\\", "scene", "variant", 77u, base_failure) &&
              base_failure.allocation_sizes.size() == 3u &&
              base_failure.strings.empty() && base_failure.freed.empty(),
          "first required allocation failure still performs three allocations");

    state = NewState();
    Runtime asset_failure;
    asset_failure.failing_allocation = 3u;
    Check(!pal4::GameWorldInitializer::Initialize(
              state, "base\\", "scene", "variant", 77u, asset_failure) &&
              asset_failure.allocation_sizes.size() == 15u &&
              asset_failure.strings.empty() && asset_failure.freed.empty(),
          "asset allocation failure returns after all temporary allocations");

    state = NewState();
    Runtime world_failure;
    world_failure.world_result = 0u;
    Check(!pal4::GameWorldInitializer::Initialize(
              state, "base\\", "scene", "variant", 77u, world_failure) &&
              state.words[4] == 0u && state.words[12] == 0u &&
              world_failure.clump_paths.empty() && world_failure.freed.empty(),
          "missing mandatory world returns before optional clumps or cleanup");

    state = NewState();
    Runtime optional_failure;
    optional_failure.missing_clump = 2u;
    Check(pal4::GameWorldInitializer::Initialize(
              state, "base\\", "scene", "variant", 77u, optional_failure) &&
              state.words[23] == 0u && state.words[12] == 1u &&
              optional_failure.current_dictionaries.size() == 5u &&
              optional_failure.freed.size() == 12u,
          "missing optional clump skips its setup and still completes");
}

}  // namespace

int main() {
    SuccessfulLoad();
    FailurePaths();
}
