#pragma once

#include <cstdint>

namespace pal4 {

class ISystemToolbarRegistrationRuntime {
public:
    virtual ~ISystemToolbarRegistrationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void InsertOrReplace(std::uint32_t tree,
                                 std::uint32_t key,
                                 std::uint32_t handler) = 0;
    virtual void ReleaseHandler(std::uint32_t handler,
                                std::uint32_t flags) = 0;
};

class SystemToolbarRegistration final {
public:
    [[nodiscard]] static std::uint32_t GetMainHandler(
        ISystemToolbarRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterMainHandler(
        ISystemToolbarRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetButton1Handler(
        ISystemToolbarRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterButton1Handler(
        ISystemToolbarRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetButton2Handler(
        ISystemToolbarRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterButton2Handler(
        ISystemToolbarRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetButton3Handler(
        ISystemToolbarRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterButton3Handler(
        ISystemToolbarRegistrationRuntime& runtime);
};

}  // namespace pal4
