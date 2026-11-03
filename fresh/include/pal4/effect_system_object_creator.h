#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace pal4 {

class IEffectSystemObjectCreateRuntime {
public:
    virtual ~IEffectSystemObjectCreateRuntime() = default;
    [[nodiscard]] virtual std::optional<std::string> StringAttribute(
        std::uint32_t document, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t FindFactory(
        std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeFactory(
        std::uint32_t factory) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual bool InitializeObject(
        std::uint32_t object, std::uint32_t document) = 0;
    virtual void DestroyObject(std::uint32_t object) = 0;
};

class EffectSystemObjectCreator final {
public:
    static constexpr std::string_view kClassAttribute =
        "Attr_Class_Name";

    // IDA 0x63e610: instantiate the class named by an XML document.
    [[nodiscard]] static std::uint32_t CreateFromDocument(
        std::uint32_t document, std::uint32_t owner,
        std::uint32_t context,
        IEffectSystemObjectCreateRuntime& runtime);
};

}  // namespace pal4
