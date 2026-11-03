#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

class ISkillManagerBindingRuntime {
public:
    virtual ~ISkillManagerBindingRuntime() = default;
    [[nodiscard]] virtual std::uint8_t RegisterHandler(
        std::uint32_t system, std::string_view table,
        std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t FindTable(
        std::uint32_t system, std::string_view table) = 0;
    [[nodiscard]] virtual std::uint32_t FindPropertyString(
        std::uint32_t system, std::string_view table) = 0;
    virtual void LogError(std::string_view message) = 0;
};

class SkillManagerBinding final {
public:
    explicit SkillManagerBinding(std::uint32_t system) : system_(system) {}

    // IDA 0x4f3b40: register Skill, Stunt and Magic handlers.
    [[nodiscard]] std::uint8_t RegisterHandlers(
        ISkillManagerBindingRuntime& runtime) const;
    // IDA 0x4f3b80: resolve and require Skill, Stunt, Magic and WuLing.
    [[nodiscard]] bool VerifyTables(ISkillManagerBindingRuntime& runtime);
    [[nodiscard]] const std::array<std::uint32_t, 4>& Handles() const {
        return handles_;
    }

private:
    std::uint32_t system_;
    std::array<std::uint32_t, 4> handles_{};
};

}  // namespace pal4
