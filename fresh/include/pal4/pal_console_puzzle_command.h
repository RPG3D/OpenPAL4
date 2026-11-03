#pragma once

#include "pal4/pal_console_arena_load_command.h"

#include <cstdint>

namespace pal4 {

class IPalConsolePuzzleRuntime : public IPalConsoleArenaLoadRuntime {
public:
    virtual void ClearConsole(std::uint32_t console) = 0;
};

class PalConsolePuzzleCommand final {
public:
    // IDA 0x5bc920 / 0x5bcb80.
    [[nodiscard]] static std::int32_t Execute(
        const char* command, IPalConsolePuzzleRuntime& runtime);
    [[nodiscard]] static std::int32_t StartJigsaw(
        const char* command, IPalConsolePuzzleRuntime& runtime);
};

}  // namespace pal4
