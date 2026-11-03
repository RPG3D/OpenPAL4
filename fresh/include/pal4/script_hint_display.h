#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IScriptHintDisplayRuntime {
public:
    virtual ~IScriptHintDisplayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyStringAddress() = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t EncodedSize(
        std::string_view utf8) = 0;
    [[nodiscard]] virtual std::u32string Encode(
        std::string_view utf8, std::uint32_t encoded_size) = 0;
    [[nodiscard]] virtual std::uint32_t UiFrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t CreateHintWindow(
        std::uint32_t frames) = 0;
    virtual void ShowHint(std::uint32_t window,
                          std::u32string_view text, float x, float y,
                          float duration, std::uint32_t parent,
                          std::uint32_t flag) = 0;
};

class ScriptHintDisplay final {
public:
    // IDA 0x5d5d80, giShowHint.
    static void Show(std::uint32_t text_payload, float x, float y,
                     IScriptHintDisplayRuntime& runtime);
};

}  // namespace pal4
