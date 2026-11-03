#pragma once

#include <cstdint>

namespace pal4 {

class IPropertyWindowCallbackCloneRuntime {
public:
    virtual ~IPropertyWindowCallbackCloneRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class PropertyWindowCallbackClone final {
public:
    [[nodiscard]] static std::uint32_t CloneUseItem(
        std::uint32_t source, IPropertyWindowCallbackCloneRuntime& runtime);
    [[nodiscard]] static std::uint32_t CloneDescription(
        std::uint32_t source, IPropertyWindowCallbackCloneRuntime& runtime);

private:
    [[nodiscard]] static std::uint32_t Clone(
        std::uint32_t source, std::uint32_t base_vtable,
        std::uint32_t final_vtable,
        IPropertyWindowCallbackCloneRuntime& runtime);
};

}  // namespace pal4
