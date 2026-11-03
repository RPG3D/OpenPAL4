#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

struct ScriptFrameAttributeLookup {
    std::uint32_t position{};
    std::uint32_t end{};
};

class IScriptFrameStateRuntime {
public:
    virtual ~IScriptFrameStateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t InitializeDocument() = 0;
    [[nodiscard]] virtual std::uint32_t DestroyDocument(
        std::uint32_t document) = 0;
    [[nodiscard]] virtual std::uint32_t ResolveFrame() = 0;
    virtual void BeginFrameName(std::string_view name) = 0;
    virtual void EndFrameName() = 0;
    [[nodiscard]] virtual ScriptFrameAttributeLookup FindAttribute(
        std::uint32_t document, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t CreateFrameAttribute(
        std::string_view name, std::uint32_t frame) = 0;
    virtual void AddAttribute(std::uint32_t document,
                              std::uint32_t attribute) = 0;
    virtual void DestroyAttribute(std::uint32_t attribute) = 0;
    virtual void SetExistingAttribute(std::uint32_t value_address,
                                      std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t Game() = 0;
    virtual void InitializeStateAndUi(std::uint32_t game, std::uint32_t state,
                                      std::uint32_t document) = 0;
};

class ScriptFrameStateDocument final {
public:
    [[nodiscard]] static std::uint32_t Enter(
        IScriptFrameStateRuntime& runtime);
};

}  // namespace pal4
