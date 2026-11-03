#pragma once

#include <cstdint>

namespace pal4 {

class ISystemSettingDialogCallbackRuntime {
public:
    virtual ~ISystemSettingDialogCallbackRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class SystemSettingDialogCallback final {
public:
    static constexpr std::uint32_t kBaseVtable = 0x8435d0u;
    static constexpr std::uint32_t kVtable = 0x843c08u;

    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t storage, std::uint32_t function,
        std::uint32_t context, std::uint32_t owner,
        ISystemSettingDialogCallbackRuntime& runtime);
    [[nodiscard]] static std::uint32_t Clone(
        std::uint32_t source, ISystemSettingDialogCallbackRuntime& runtime);
};

}  // namespace pal4
