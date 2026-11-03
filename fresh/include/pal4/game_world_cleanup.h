#pragma once

#include "pal4/game_world_object.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IGameWorldCleanupRuntime {
public:
    virtual ~IGameWorldCleanupRuntime() = default;
    virtual void FreeExternal(std::uint32_t address) = 0;
    virtual void CleanupDictionary(GameWorldObjectState& state,
                                   std::uint32_t word_offset) = 0;
    virtual void RemoveCamera(std::uint32_t world,
                              std::uint32_t camera) = 0;
    virtual void DestroyLight(std::uint32_t light) = 0;
    [[nodiscard]] virtual std::uint32_t GetClumpWorld(
        std::uint32_t clump) = 0;
    virtual void RemoveClump(std::uint32_t world,
                             std::uint32_t clump) = 0;
    virtual void DestroyClump(std::uint32_t clump) = 0;
    virtual void DestroyWorld(std::uint32_t world) = 0;
    [[nodiscard]] virtual std::string ReadText(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void WriteText(std::uint32_t destination,
                           std::string_view text) = 0;
    virtual void RemoveTextureByName(std::uint32_t context,
                                     std::string_view name) = 0;
    [[nodiscard]] virtual std::int32_t Free(std::uint32_t address) = 0;
};

class GameWorldCleanup final {
public:
    [[nodiscard]] static std::int32_t Cleanup(
        GameWorldObjectState& state, std::uint32_t context,
        IGameWorldCleanupRuntime& runtime);
};

}  // namespace pal4
