#pragma once

#include <cstdint>

namespace pal4 {

struct GameArenaSingletonState {
    std::uint8_t guard = 0u;
    std::uint32_t instance = 0u;
};

class IGameArenaSingletonRuntime {
public:
    virtual ~IGameArenaSingletonRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetSingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t Register(
        std::uint32_t manager, const char* name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t destructor) = 0;
};

class GameArenaManagerSingleton final {
public:
    static constexpr std::uint32_t kGuardAddress = 0x8f2d84u;
    static constexpr std::uint32_t kInstanceAddress = 0x8f2d80u;
    static constexpr std::uint32_t kFactoryAddress = 0x5184a0u;
    static constexpr std::uint32_t kDestructorAddress = 0x7de5d0u;

    explicit GameArenaManagerSingleton(GameArenaSingletonState& state)
        : state_(state) {}

    // IDA 0x518460.
    [[nodiscard]] std::uint32_t GetInstance(
        IGameArenaSingletonRuntime& runtime);

private:
    GameArenaSingletonState& state_;
};

enum class GameArenaLifecycleCall : std::uint8_t {
    ConstructWorld,
    InitializeText,
    InitializeConfig,
    ConstructNameNode,
    InitializeCache,
    InitializeObjectList,
    InitializeDefaultValues,
    FreeMemory,
    DestroyScript,
    DestroyCache,
    ResetNameNode,
    DestroyConfig,
    ReleaseRange,
    CleanupMemoryBlock,
    CleanupText,
    DestroyWorld,
    ReleaseArena,
};

class IGameArenaLifecycleRuntime {
public:
    virtual ~IGameArenaLifecycleRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Call(
        GameArenaLifecycleCall operation, std::uint32_t address,
        std::uint32_t auxiliary = 0u) = 0;
};

class GameArenaTextPair final {
public:
    static constexpr std::uint32_t kBytes = 36u;

    // IDA 0x518750 and 0x5187a0.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t address, IGameArenaLifecycleRuntime& runtime);
    static void Destroy(std::uint32_t address,
                        IGameArenaLifecycleRuntime& runtime);
};

class GameArenaManagerLifecycle final {
public:
    static constexpr std::uint32_t kObjectBytes = 0x2b8u;
    static constexpr std::uint32_t kVtableAddress = 0x844894u;

    // IDA 0x5184a0, 0x518500 and 0x5187f0.
    [[nodiscard]] static std::uint32_t Create(
        IGameArenaLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IGameArenaLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, IGameArenaLifecycleRuntime& runtime);
    // IDA 0x519950 forwards to the later ReleaseArena implementation.
    [[nodiscard]] static std::uint32_t ForwardRelease(
        std::uint32_t object, IGameArenaLifecycleRuntime& runtime);
};

}  // namespace pal4
