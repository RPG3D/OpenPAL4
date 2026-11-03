#pragma once

#include <cstdint>
#include <optional>

namespace pal4 {

class IRoleStateWindowButtonStateRuntime {
public:
    virtual ~IRoleStateWindowButtonStateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t DeleteObject(
        std::uint32_t object, std::uint32_t flags) = 0;
};

class RoleStateWindowButtonState final {
public:
    static constexpr std::uint32_t kVtable = 0x843ac8u;

    [[nodiscard]] static std::optional<std::uint32_t> Release(
        std::uint32_t slot, IRoleStateWindowButtonStateRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t slot, std::uint32_t first,
        std::uint32_t second,
        IRoleStateWindowButtonStateRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, std::uint32_t first,
        std::uint32_t second,
        IRoleStateWindowButtonStateRuntime& runtime);
};

}  // namespace pal4
