#include "pal4/role_state_window_level_text.h"

#include <array>
#include <cstdio>
#include <string>
#include <string_view>

namespace pal4 {
namespace {

class LevelFormatter final : public IStringBufferFormatRuntime {
public:
    LevelFormatter(IStringBufferRuntime& memory, const std::string& text)
        : memory_(memory), text_(text) {}

    [[nodiscard]] std::int32_t Vsprintf(
        std::uint32_t destination, std::uint32_t,
        std::uint32_t) override {
        const auto length = static_cast<std::int32_t>(text_.size());
        for (std::int32_t i = 0; i < length; ++i) {
            memory_.WriteByte(destination + static_cast<std::uint32_t>(i),
                              static_cast<std::uint8_t>(text_[i]));
        }
        memory_.WriteByte(destination + static_cast<std::uint32_t>(length),
                          0u);
        return length;
    }

private:
    IStringBufferRuntime& memory_;
    const std::string& text_;
};

}  // namespace

std::string RoleStateWindowLevelText::FormatText(
    const std::int32_t value) {
    if (value <= 0 || value >= 100) return "0";
    const auto tens = static_cast<std::int32_t>(
        static_cast<double>(value) * 0.10000000149011612);
    const auto ones = value % 10;
    std::string_view pattern;
    if (tens <= 0) pattern = "%d\n \n";
    else if (ones != 0) pattern = "%d\n0\n%d\n \n ";
    else pattern = "%d\n\n%d\n \n ";
    std::array<char, 64> text{};
    std::snprintf(text.data(), text.size(), pattern.data(),
                  tens <= 0 ? ones : tens, tens <= 0 ? 0 : ones);
    return text.data();
}

StringBuffer& RoleStateWindowLevelText::FormatInto(
    StringBuffer& output, const std::int32_t value,
    IRoleStateWindowLevelRuntime& runtime) {
    if (value <= 0 || value >= 100) {
        return output.InitializeFromCString(
            runtime.ZeroLiteralAddress(), runtime);
    }

    const auto text = FormatText(value);
    LevelFormatter formatter(runtime, text);
    StringBuffer temporary;
    static_cast<void>(temporary.InitializeEmpty(runtime));
    try {
        static_cast<void>(temporary.FormatVariadic(0u, 0u,
                                                    runtime, formatter));
        static_cast<void>(output.CopyConstruct(temporary, runtime));
    } catch (...) {
        static_cast<void>(temporary.Cleanup(runtime));
        throw;
    }
    static_cast<void>(temporary.Cleanup(runtime));
    return output;
}

}  // namespace pal4
