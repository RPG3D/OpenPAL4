#pragma once

#include <cstdint>

namespace pal4 {

enum class GameArenaClumpCall : std::uint8_t {
    GetCameraSubsystem,
    AddCameraClump,
    RemoveCameraClump,
    GetMovementAdapter,
    GetResourceProperty,
    SetClumpProperties,
    RestoreRenderCallback,
};

class IGameArenaClumpRuntime {
public:
    virtual ~IGameArenaClumpRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Call(
        GameArenaClumpCall operation, std::uint32_t first = 0u,
        std::uint32_t second = 0u, std::uint32_t third = 0u,
        std::uint32_t fourth = 0u, std::uint32_t fifth = 0u,
        std::uint32_t sixth = 0u) = 0;
};

class GameArenaClumpRuntimeStub final : public IGameArenaClumpRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t Call(GameArenaClumpCall, std::uint32_t,
                                      std::uint32_t, std::uint32_t,
                                      std::uint32_t, std::uint32_t,
                                      std::uint32_t) override { return 0u; }
};

class GameArenaClumpCallbacks final {
public:
    // IDA 0x51b870 and 0x51b920.
    [[nodiscard]] static std::uint32_t Enter(
        std::uint32_t object, std::uint32_t world,
        IGameArenaClumpRuntime& runtime);
    [[nodiscard]] static std::uint32_t Leave(
        std::uint32_t object, std::uint32_t world,
        IGameArenaClumpRuntime& runtime);
};

}  // namespace pal4
