#pragma once

#include <cstdint>

namespace pal4 {

class ISystemSettingRegistrationRuntime {
public:
    virtual ~ISystemSettingRegistrationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void InsertOrReplace(std::uint32_t tree,
                                 std::uint32_t key,
                                 std::uint32_t handler) = 0;
    virtual void ReleaseHandler(std::uint32_t handler,
                                std::uint32_t flags) = 0;
};

class SystemSettingRegistration final {
public:
    static constexpr std::uint32_t kNameSource = 0x8b06acu;
    static constexpr std::uint32_t kEventKey = 0x8b06a8u;
    static constexpr std::uint32_t kStorage = 0x8e7970u;
    static constexpr std::uint32_t kHandlerVtable = 0x843494u;

    [[nodiscard]] static std::uint32_t GetHandler(
        ISystemSettingRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterHandler(
        ISystemSettingRegistrationRuntime& runtime);
};

}  // namespace pal4
