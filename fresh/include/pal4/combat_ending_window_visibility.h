#pragma once

#include "pal4/event_handler_array.h"

#include <cstdint>

namespace pal4 {

class ICombatEndingWindowActivityRuntime {
public:
    virtual ~ICombatEndingWindowActivityRuntime() = default;
    virtual std::uint32_t InsertEndingHandler(std::uint32_t handler) = 0;
    virtual void RemoveEndingHandler(std::uint32_t handler) = 0;
};

class ICombatEndingWindowVisibilityRuntime {
public:
    virtual ~ICombatEndingWindowVisibilityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void SetParent(std::uint32_t window, std::uint8_t visible,
                           std::uint32_t parent) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    virtual void CleanupHint(std::uint32_t hint) = 0;
    virtual void PlayStoredSound(std::uint32_t handle_address) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t ResetListFlags(
        std::uint32_t list) = 0;
};

class CombatEndingWindowVisibility final {
public:
    // IDA 0x543e90.
    [[nodiscard]] static std::uint32_t Update(
        std::uint32_t window, std::uint8_t visible,
        std::uint32_t parent, ICombatEndingWindowVisibilityRuntime& runtime,
        IEventHandlerArrayRuntime& array_runtime);
    [[nodiscard]] static std::uint32_t Update(
        std::uint32_t window, std::uint8_t visible, std::uint32_t parent,
        ICombatEndingWindowVisibilityRuntime& runtime,
        ICombatEndingWindowActivityRuntime& activity);
};

}  // namespace pal4
