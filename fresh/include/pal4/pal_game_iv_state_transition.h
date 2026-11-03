#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

struct PalGameIVStateLabels {
    std::uint8_t guard = 0;
    std::array<std::uint8_t, 13> markers{};
    std::array<std::uint8_t, 13> error_flags{};
    std::array<std::string, 13> names{};
};

class IPalGameIVStateRuntime {
public:
    virtual ~IPalGameIVStateRuntime() = default;
    virtual void RegisterAtExit(std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t FindEntry(
        std::uint32_t game, std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void Leave(std::uint32_t state, std::uint32_t next_id) = 0;
    [[nodiscard]] virtual std::uint32_t StateId(std::uint32_t state) = 0;
    [[nodiscard]] virtual std::uint32_t UiFrameManager() = 0;
    virtual void SetCursor(std::uint32_t cursor_slot,
                           std::string_view resource) = 0;
    virtual void LogExchange(std::string_view previous,
                             std::string_view next,
                             std::uint32_t source_line) = 0;
    [[nodiscard]] virtual bool Enter(std::uint32_t state,
                                     std::uint32_t previous_id,
                                     std::uint32_t argument) = 0;
    [[nodiscard]] virtual std::uint32_t TimeGetTime() = 0;
    virtual void WriteGameStateWord(std::uint32_t offset,
                                    std::uint32_t value) = 0;
};

class PalGameIVStateTransition final {
public:
    [[nodiscard]] static bool Initialize(
        std::uint32_t game, std::uint32_t next_id,
        std::uint32_t argument, PalGameIVStateLabels& labels,
        IPalGameIVStateRuntime& runtime);
    static void Cleanup(PalGameIVStateLabels& labels);
};

}  // namespace pal4
