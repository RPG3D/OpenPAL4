#pragma once

#include "pal4/game_world_object.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IGameWorldMaterialRuntime {
public:
    virtual ~IGameWorldMaterialRuntime() = default;
    [[nodiscard]] virtual std::string ReadText(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t OpenDataStream(
        std::uint32_t kind, std::uint32_t mode,
        std::string_view path) = 0;
    virtual void ReadData(std::uint32_t stream, GameWorldObjectState& state,
                          std::uint32_t word_offset, std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t CloseDataStream(
        std::uint32_t stream, std::uint32_t flags) = 0;
};

class GameWorldMaterialParams final {
public:
    [[nodiscard]] static std::uint32_t Load(
        GameWorldObjectState& state, IGameWorldMaterialRuntime& runtime);
};

}  // namespace pal4
