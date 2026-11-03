#pragma once

#include "pal4/script_equipment_inventory_query.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class IScriptEquipmentRemovalRuntime : public IScriptEquipmentInventoryQueryRuntime {
public:
    virtual ~IScriptEquipmentRemovalRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t CreateHintWindow(
        std::uint32_t manager) = 0;
    virtual void RemoveElement(std::uint32_t collection,
                               std::uint32_t element) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeText() = 0;
    [[nodiscard]] virtual std::uint32_t EmptyText() = 0;
    [[nodiscard]] virtual std::uint32_t FindGameString(
        std::uint32_t table, std::uint32_t id) = 0;
    virtual void FormatText(std::uint32_t destination,
                            std::uint32_t format,
                            std::uint32_t name) = 0;
    virtual void AppendText(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void ShowHint(std::uint32_t hint, std::uint32_t text,
                          float x, float y, float scale,
                          std::uint32_t mode, std::uint32_t visible) = 0;
    [[nodiscard]] virtual std::uint32_t AudioManager() = 0;
    virtual void PlayMusic(std::uint32_t audio, std::uint8_t flag,
                           std::string_view path, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint8_t CleanupText(
        std::uint32_t text) = 0;
};

class ScriptEquipmentRemoval final {
public:
    // IDA 0x5d8240, giRemoveEquipment.
    [[nodiscard]] static std::uint8_t Remove(
        std::int32_t id, std::uint8_t show_hint,
        IScriptEquipmentRemovalRuntime& runtime);
};

}  // namespace pal4
