#pragma once

#include "pal4/game_world_cleanup.h"
#include "pal4/game_world_light_gather.h"
#include "pal4/game_world_material_params.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class IGameWorldInitializeRuntime : public IGameWorldCleanupRuntime,
                                    public IGameWorldMaterialRuntime,
                                    public IGameWorldLightGatherRuntime {
public:
    virtual ~IGameWorldInitializeRuntime() = default;
    virtual void ReallocateImage(std::uint32_t base_path) = 0;
    virtual void LoadDictionary(std::uint32_t path) = 0;
    [[nodiscard]] virtual std::uint32_t LoadAtomic(
        std::uint32_t path, std::uint32_t context) = 0;
    [[nodiscard]] virtual std::uint32_t LoadClump(
        std::uint32_t path, std::uint32_t context) = 0;
    virtual void SetCurrentDictionary(GameWorldObjectState& state,
                                      std::uint32_t word_offset,
                                      std::uint32_t object) = 0;
    virtual void SetWorldMaterialFlags(std::uint32_t world,
                                       std::uint32_t flags) = 0;
    virtual void SetClumpMaterialFlags(std::uint32_t clump,
                                       std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void ProcessSkyAtomicGeometry(std::uint32_t sky_clump,
                                           GameWorldObjectState& state,
                                           std::uint32_t word_offset) = 0;
};

class GameWorldInitializer final {
public:
    [[nodiscard]] static bool Initialize(
        GameWorldObjectState& state, std::string_view base_directory,
        std::string_view scene_name, std::string_view variant_name,
        std::uint32_t context, IGameWorldInitializeRuntime& runtime);
};

}  // namespace pal4
