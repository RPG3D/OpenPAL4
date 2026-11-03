#pragma once

#include <cstdint>

namespace pal4 {

struct BackgroundMusicSingletonState {
    std::uint8_t initialization_bits = 0u;
    std::uint32_t instance = 0u;
};

class IBackgroundMusicSingletonRuntime {
public:
    virtual ~IBackgroundMusicSingletonRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetSingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t Register(
        std::uint32_t manager, const char* name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t destructor) = 0;
};

class BackgroundMusicSingleton final {
public:
    static constexpr std::uint32_t kGuardAddress = 0x8efab4u;
    static constexpr std::uint32_t kInstanceAddress = 0x8efab0u;
    static constexpr std::uint32_t kFactoryAddress = 0x513cd0u;
    static constexpr std::uint32_t kDestructorAddress = 0x513d30u;

    explicit BackgroundMusicSingleton(BackgroundMusicSingletonState& state)
        : state_(state) {}

    // IDA 0x513c90.
    [[nodiscard]] std::uint32_t GetInstance(
        IBackgroundMusicSingletonRuntime& runtime);

private:
    BackgroundMusicSingletonState& state_;
};

class IBackgroundMusicLifecycleRuntime {
public:
    virtual ~IBackgroundMusicLifecycleRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
};

class BackgroundMusicPlayerLifecycle final {
public:
    static constexpr std::uint32_t kObjectBytes = 0x40u;

    // IDA 0x513cd0, 0x513d30, 0x513d50 and 0x513df0.
    [[nodiscard]] static std::uint32_t Create(
        IBackgroundMusicLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, IBackgroundMusicLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupResources(
        std::uint32_t object, IBackgroundMusicLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IBackgroundMusicLifecycleRuntime& runtime);
};

}  // namespace pal4
