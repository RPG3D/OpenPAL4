#pragma once

#include <cstdint>

namespace pal4 {

class IScriptPlayerStatusRuntime {
public:
    virtual ~IScriptPlayerStatusRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::int32_t index) = 0;
    virtual void WriteLevel(std::uint32_t state, std::uint32_t stat,
                            std::int32_t level) = 0;
    virtual void RefreshPlayerState(std::uint32_t state,
                                    std::int32_t level,
                                    std::uint32_t player_state) = 0;
    [[nodiscard]] virtual std::uint32_t BuildMatchingRecords(
        std::uint32_t collection, std::int32_t level,
        std::uint32_t player_state) = 0;
    [[nodiscard]] virtual std::uint32_t FreeRecords(
        std::uint32_t buffer) noexcept = 0;
};

class ScriptPlayerStatus final {
public:
    // IDA 0x5d5120, giPlayerGetLeader.
    [[nodiscard]] static std::int32_t GetLeaderState(
        IScriptPlayerStatusRuntime& runtime);
    // IDA 0x5d5180, giSetPlayerLevel.
    [[nodiscard]] static std::uint32_t SetLevel(
        std::int32_t player_id, std::int32_t level,
        IScriptPlayerStatusRuntime& runtime);
};

}  // namespace pal4
