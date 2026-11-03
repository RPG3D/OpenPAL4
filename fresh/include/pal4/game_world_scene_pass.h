#pragma once

#include <cstdint>

namespace pal4 {

enum class GameWorldScenePassOperation : std::uint8_t {
    RenderClump,
    UpdateArenaCache,
    SortGeometry,
};

class IGameWorldScenePassRuntime {
public:
    virtual ~IGameWorldScenePassRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Call(
        GameWorldScenePassOperation operation, std::uint32_t first,
        std::uint32_t second = 0u) = 0;
};

class GameWorldScenePassRuntimeStub final : public IGameWorldScenePassRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t Call(GameWorldScenePassOperation,
                                     std::uint32_t,
                                     std::uint32_t) override { return 0u; }
};

class GameWorldScenePass final {
public:
    // IDA 0x51a1c0.
    [[nodiscard]] static std::uint32_t Process(
        std::uint32_t world, std::uint32_t context,
        IGameWorldScenePassRuntime& runtime);
    // IDA 0x51a220 and 0x51a230.
    [[nodiscard]] static std::uint32_t RenderPrimaryClump(
        std::uint32_t world, IGameWorldScenePassRuntime& runtime);
    [[nodiscard]] static std::uint32_t RenderSecondaryClump(
        std::uint32_t world, IGameWorldScenePassRuntime& runtime);
};

}  // namespace pal4
