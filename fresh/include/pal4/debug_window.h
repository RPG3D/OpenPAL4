#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct DebugWindowAccessState {
    std::uint8_t guard{};
    std::uint32_t instance{};
};

class IDebugWindowAccessRuntime {
public:
    virtual ~IDebugWindowAccessRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetSingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t Register(
        std::uint32_t manager, std::string_view name,
        std::uint32_t id, std::uint32_t factory,
        std::uint32_t destroy) = 0;
};

class DebugWindowAccess final {
public:
    static constexpr std::uint32_t kGuardAddress = 0x8e40e0u;
    static constexpr std::uint32_t kInstanceAddress = 0x8e40dcu;
    static constexpr std::uint32_t kFactoryAddress = 0x418cf0u;
    static constexpr std::uint32_t kDestroyAddress = 0x418d50u;

    [[nodiscard]] static std::uint32_t Get(
        DebugWindowAccessState& state, IDebugWindowAccessRuntime& runtime);
};

class DebugWindowState final {
public:
    static constexpr std::uint32_t kBytes = 0x2cu;
    [[nodiscard]] std::array<std::uint8_t, kBytes>& Bytes() noexcept {
        return bytes_;
    }
    [[nodiscard]] std::uint32_t ReadDword(std::uint32_t offset) const;
    void WriteDword(std::uint32_t offset, std::uint32_t value);

private:
    std::array<std::uint8_t, kBytes> bytes_{};
};

class IDebugWindowRuntime {
public:
    virtual ~IDebugWindowRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual DebugWindowState& Access(std::uint32_t address) = 0;
    virtual void ConstructString(std::uint32_t address) = 0;
    virtual void ReallocateString(std::uint32_t address,
                                  std::uint32_t capacity) = 0;
    virtual void CleanupString(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyWindow(
        std::uint32_t handle) = 0;
    virtual void KillTimer(std::uint32_t handle, std::uint32_t timer_id) = 0;
};

class DebugWindowObject final {
public:
    [[nodiscard]] static std::uint32_t Create(IDebugWindowRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t address, IDebugWindowRuntime& runtime);
    static void Destruct(std::uint32_t address, IDebugWindowRuntime& runtime);
    static void Destroy(std::uint32_t address, IDebugWindowRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyWindows(
        std::uint32_t address, IDebugWindowRuntime& runtime);
};

}  // namespace pal4
