#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IScriptObjectVisibilityRuntime {
public:
    virtual ~IScriptObjectVisibilityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ObjectManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindByName(
        std::uint32_t manager, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    virtual void RemoveAnimatedObject(std::uint32_t arena,
                                      std::uint32_t object) = 0;
    virtual void SetObjectState(std::uint32_t object,
                                std::uint32_t state) = 0;
    virtual void AddAnimatedObject(std::uint32_t arena,
                                   std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t MinimapManager() = 0;
    virtual void RefreshDialog(std::uint32_t minimap) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t LogVisibility(
        std::uint32_t source_line, std::string_view name,
        std::uint32_t arena_word8, std::uint32_t arena_word_e0,
        std::uint8_t visible) = 0;
};

class ScriptObjectVisibility final {
public:
    // IDA 0x5d4640, giSetObjectVisible.
    [[nodiscard]] static std::uint32_t SetVisible(
        const char* name, std::uint8_t visible,
        IScriptObjectVisibilityRuntime& runtime);
};

}  // namespace pal4
