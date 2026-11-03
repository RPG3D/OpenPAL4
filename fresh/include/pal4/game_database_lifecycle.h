#pragma once

#include <cstdint>

namespace pal4 {

struct GameDatabaseSingletonState {
    std::uint8_t initialization_bits = 0u;
    std::uint32_t instance = 0u;
};

class IGameDatabaseSingletonRuntime {
public:
    virtual ~IGameDatabaseSingletonRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetSingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t Register(
        std::uint32_t manager, const char* name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t destructor) = 0;
};

class GameDatabaseSingleton final {
public:
    static constexpr std::uint32_t kGuardAddress = 0x8ef3a4u;
    static constexpr std::uint32_t kInstanceAddress = 0x8ef3a0u;
    static constexpr std::uint32_t kFactoryAddress = 0x50c7b0u;
    static constexpr std::uint32_t kDestructorAddress = 0x50c810u;

    explicit GameDatabaseSingleton(GameDatabaseSingletonState& state)
        : state_(state) {}

    // IDA 0x50c770.
    [[nodiscard]] std::uint32_t GetInstance(
        IGameDatabaseSingletonRuntime& runtime);

    // IDA 0x50f9f0 forwards to GameDB_GetInstance_Internal.
    [[nodiscard]] std::uint32_t GetInstanceAlias(
        IGameDatabaseSingletonRuntime& runtime);

private:
    GameDatabaseSingletonState& state_;
};

class IGameDatabaseLifecycleRuntime {
public:
    virtual ~IGameDatabaseLifecycleRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void InitializeComponent(std::uint32_t address,
                                     std::uint32_t initializer) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeManager(
        std::uint32_t address) = 0;
    virtual void SetEffectGlobal(std::uint32_t callback) = 0;
    virtual void SetGameDatabaseGlobal(std::uint32_t callback) = 0;
    virtual void BindManager(std::uint32_t component,
                             std::uint32_t manager) = 0;
    virtual void SetPropertyManager(std::uint32_t property,
                                    std::uint32_t manager) = 0;
    virtual void DestroyManager(std::uint32_t address) = 0;
    virtual void DestroyComponent(std::uint32_t address) = 0;
};

class GameDatabaseLifecycle final {
public:
    static constexpr std::uint32_t kObjectBytes = 0x14cu;
    static constexpr std::uint32_t kManagerBytes = 0x20u;
    static constexpr std::uint32_t kAssertionLoggerSlot = 0x90f948u;

    // IDA 0x5ed4d0: store the GameDB assertion logger callback.
    [[nodiscard]] static std::uint32_t SetAssertionLoggerCallback(
        std::uint32_t& slot, std::uint32_t callback);

    // IDA 0x50c7b0 and 0x50c810.
    [[nodiscard]] static std::uint32_t CreateInstance(
        IGameDatabaseLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyInstance(
        std::uint32_t object, IGameDatabaseLifecycleRuntime& runtime);

    // IDA 0x50c830 and 0x50ca50.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IGameDatabaseLifecycleRuntime& runtime);
    static void Destruct(std::uint32_t object,
                         IGameDatabaseLifecycleRuntime& runtime);
};

class GlobalSystemAccess final {
public:
    static constexpr std::uint32_t kObjectAddress = 0x90f148u;
    // IDA 0x5ed4e0: return the address of the static object.
    [[nodiscard]] static constexpr std::uint32_t GetInstanceAddress() {
        return kObjectAddress;
    }
};

}  // namespace pal4
