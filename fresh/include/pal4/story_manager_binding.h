#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

class IStoryManagerBindingRuntime {
public:
    virtual ~IStoryManagerBindingRuntime() = default;
    [[nodiscard]] virtual std::uint8_t RegisterHandler(
        std::uint32_t system, std::string_view name,
        std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t FindTable(
        std::uint32_t system, std::string_view name) = 0;
    virtual void LogError(std::string_view message) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
};

class StoryManagerBinding final {
public:
    explicit StoryManagerBinding(std::uint32_t system) : system_(system) {}

    // IDA 0x4fadb0: signed-ID lower-bound search in the Story key index.
    [[nodiscard]] std::uint32_t FindStory(
        std::int32_t id, IStoryManagerBindingRuntime& runtime) const;
    // IDA 0x4fae20: register Story, StoryDesc and GameQuestion factories.
    [[nodiscard]] std::uint8_t RegisterHandlers(
        IStoryManagerBindingRuntime& runtime) const;
    // IDA 0x4fae60: resolve and require all three tables.
    [[nodiscard]] bool VerifyTables(IStoryManagerBindingRuntime& runtime);
    [[nodiscard]] const std::array<std::uint32_t, 3>& Handles() const {
        return handles_;
    }

private:
    std::uint32_t system_;
    std::array<std::uint32_t, 3> handles_{};
};

}  // namespace pal4
