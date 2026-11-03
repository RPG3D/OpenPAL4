#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

struct GameArenaStringArgument {
    const char* value;
    std::uint32_t compare_area;
};

class IGameArenaStringRuntime {
public:
    virtual ~IGameArenaStringRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    [[nodiscard]] virtual std::string_view ReadText(std::uint32_t address) = 0;
};

class GameArenaStringMatcher final {
public:
    // IDA 0x5d3910.
    [[nodiscard]] static bool Matches(
        GameArenaStringArgument arena, GameArenaStringArgument area,
        IGameArenaStringRuntime& runtime);
};

class CrtExceptionFrameNoOp final {
public:
    // IDA 0x5d38e0 only creates and removes an SEH frame.
    static void Run() noexcept {}
};

}  // namespace pal4
