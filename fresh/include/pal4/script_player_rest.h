#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IScriptPlayerRestRuntime {
public:
    virtual ~IScriptPlayerRestRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindPlayer(std::uint32_t manager,
                                                    std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t GetStat(std::uint32_t bank,
                                                std::uint32_t slot) = 0;
    virtual void SetStat(std::uint32_t bank, std::uint32_t slot,
                         std::int32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t AudioManager() = 0;
    [[nodiscard]] virtual std::uint32_t PlayMusic(std::uint32_t manager,
                                                  std::string_view path,
                                                  std::uint32_t mode) = 0;
};

class ScriptPlayerRest final {
public:
    // IDA 0x5d7c70, giPlayerTakeARest.
    [[nodiscard]] static std::uint32_t RestoreParty(
        IScriptPlayerRestRuntime& runtime);
};

}  // namespace pal4
