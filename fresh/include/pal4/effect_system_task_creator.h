#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace pal4 {

class IEffectSystemTaskCreateRuntime {
public:
    virtual ~IEffectSystemTaskCreateRuntime() = default;
    [[nodiscard]] virtual std::optional<std::string> StringAttribute(
        std::uint32_t document, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t FindFactory(
        std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeFactory(
        std::uint32_t factory) = 0;
    [[nodiscard]] virtual bool InitializeTask(
        std::uint32_t task, std::uint32_t document) = 0;
    virtual void DeleteTask(std::uint32_t task, std::uint32_t flags) = 0;
};

class EffectSystemTaskCreator final {
public:
    static constexpr std::string_view kClassAttribute = "Attr_Class_Name";

    // IDA 0x6413d0: resolve a task factory from an XML class attribute.
    [[nodiscard]] static std::uint32_t CreateFromDocument(
        std::uint32_t document, IEffectSystemTaskCreateRuntime& runtime);
};

}  // namespace pal4
