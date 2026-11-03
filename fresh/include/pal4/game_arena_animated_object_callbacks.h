#pragma once

#include <cstdint>

namespace pal4 {

enum class GameArenaAnimatedObjectCall : std::uint8_t {
    CleanupObject,
    AddClump,
    Virtual72,
    ProcessAtomics,
    ProcessAtomicsOptional,
    ProcessAtomicsFinal,
    Virtual140,
    GetCameraSubsystem,
    AddCameraClump,
    Virtual156,
    Virtual76,
    ResetAtomicCallbacks,
    ResetMaterialCallbacks,
    ProcessAtomicsWrapper,
    RemoveClump,
    RemoveCameraClump,
};

class IGameArenaAnimatedObjectRuntime {
public:
    virtual ~IGameArenaAnimatedObjectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Call(
        GameArenaAnimatedObjectCall operation, std::uint32_t first = 0u,
        std::uint32_t second = 0u) = 0;
};

class GameArenaAnimatedObjectRuntimeStub final
    : public IGameArenaAnimatedObjectRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t Call(GameArenaAnimatedObjectCall,
                                      std::uint32_t,
                                      std::uint32_t) override { return 0u; }
};

class GameArenaAnimatedObjectCallbacks final {
public:
    // IDA 0x51b990 and 0x51ba60.
    [[nodiscard]] static std::uint32_t Enter(
        std::uint32_t object, std::uint32_t world,
        IGameArenaAnimatedObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t Leave(
        std::uint32_t object, std::uint32_t world,
        IGameArenaAnimatedObjectRuntime& runtime);
};

}  // namespace pal4
