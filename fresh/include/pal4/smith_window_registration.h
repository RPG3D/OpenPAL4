#pragma once

#include <cstdint>

namespace pal4 {

class ISmithWindowRegistrationRuntime {
public:
    virtual ~ISmithWindowRegistrationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t RegisterFactory(
        std::uint32_t manager, std::uint32_t name,
        std::uint32_t factory) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void InsertOrReplace(std::uint32_t tree,
                                 std::uint32_t key,
                                 std::uint32_t handler) = 0;
    virtual void DestroyHandler(std::uint32_t handler,
                                std::uint32_t flags) = 0;
};

class SmithWindowRegistration final {
public:
    // IDA 0x496ac0.
    [[nodiscard]] static std::uint32_t InitializeThunk(
        ISmithWindowRegistrationRuntime& runtime);
    // IDA 0x496ad0.
    [[nodiscard]] static std::uint32_t Initialize(
        ISmithWindowRegistrationRuntime& runtime);
    // IDA 0x496b40.
    [[nodiscard]] static std::uint32_t RegisterUIThunk(
        ISmithWindowRegistrationRuntime& runtime);
    // IDA 0x496b50.
    [[nodiscard]] static std::uint32_t RegisterUI(
        ISmithWindowRegistrationRuntime& runtime);
    // IDA 0x496b70.
    [[nodiscard]] static std::uint32_t RegisterEventHandlerThunk(
        ISmithWindowRegistrationRuntime& runtime);
    // IDA 0x496b80.
    [[nodiscard]] static std::uint32_t RegisterEventHandler(
        ISmithWindowRegistrationRuntime& runtime);
};

}  // namespace pal4
