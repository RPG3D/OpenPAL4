#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IScriptNightTimeQueryRuntime {
public:
    virtual ~IScriptNightTimeQueryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    [[nodiscard]] virtual std::uint32_t ExtractTail(
        std::uint32_t source, std::uint32_t count) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeString(
        std::string_view value) = 0;
    [[nodiscard]] virtual std::int32_t CompareCaseInsensitive(
        std::uint32_t left, std::uint32_t right) = 0;
    virtual void DestroyString(std::uint32_t string) = 0;
};

class ScriptNightTimeQuery final {
public:
    // IDA 0x5d7ce0, giIsNightTime.
    [[nodiscard]] static bool IsNightTime(IScriptNightTimeQueryRuntime& runtime);
};

}  // namespace pal4
