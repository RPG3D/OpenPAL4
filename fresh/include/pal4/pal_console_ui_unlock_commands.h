#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IPalConsoleUiUnlockRuntime {
public:
    virtual ~IPalConsoleUiUnlockRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GameInstance() = 0;
    virtual void SetGameDataEntry(std::uint32_t game,
                                  std::uint32_t entry,
                                  std::int32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Console() = 0;
    [[nodiscard]] virtual std::int32_t AddOutputLine(
        std::uint32_t console, std::string_view format) = 0;
};

class PalConsoleUiUnlockCommands final {
public:
    // IDA 0x5bd240 / 0x5bd300 / 0x5bd370.
    static void OpenSystemUi(const char* command,
                             IPalConsoleUiUnlockRuntime& runtime);
    static void OpenMagicUi(const char* command,
                            IPalConsoleUiUnlockRuntime& runtime);
    static void OpenStuntUi(const char* command,
                            IPalConsoleUiUnlockRuntime& runtime);
};

}  // namespace pal4
