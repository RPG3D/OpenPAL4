#pragma once

#include <cstdint>

namespace pal4 {

class IRoleSelectorRegistrationRuntime {
public:
    virtual ~IRoleSelectorRegistrationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t Construct(
        std::uint32_t object, std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t RegisterFactory(
        std::uint32_t manager, std::uint32_t name,
        std::uint32_t factory) = 0;
};

class RoleSelectorRegistration final {
public:
    // IDA 0x4b62b0/0x4b62c0.
    [[nodiscard]] static std::uint32_t InitializeNameThunk(
        IRoleSelectorRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeName(
        IRoleSelectorRegistrationRuntime& runtime);
    // IDA 0x4b62d0.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t name, IRoleSelectorRegistrationRuntime& runtime);
    // IDA 0x4b6330/0x4b6340.
    [[nodiscard]] static std::uint32_t RegisterFactoryThunk(
        IRoleSelectorRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterFactory(
        IRoleSelectorRegistrationRuntime& runtime);
};

}  // namespace pal4
