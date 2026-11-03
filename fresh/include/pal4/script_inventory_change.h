#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IScriptInventoryChangeRuntime {
public:
    virtual ~IScriptInventoryChangeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t UiFrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t CreateHintWindow(
        std::uint32_t frames) = 0;
    [[nodiscard]] virtual std::uint32_t GameDatabase() = 0;
    [[nodiscard]] virtual std::uint8_t HasProperty(
        std::uint32_t database, std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t FindProperty(
        std::uint32_t database, std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t Inventory() = 0;
    virtual void AddItem(std::uint32_t inventory, std::uint32_t id,
                         std::int32_t count) = 0;
    virtual void RemoveItem(std::uint32_t inventory, std::uint32_t id,
                            std::int32_t count) = 0;
    virtual void InitializeTextBuffer() = 0;
    [[nodiscard]] virtual std::string_view PropertyName(
        std::uint32_t property) = 0;
    [[nodiscard]] virtual std::string_view GameString(
        std::uint32_t table, std::uint32_t id) = 0;
    virtual void FormatText(std::string_view format, std::string_view name,
                            std::int32_t count) = 0;
    [[nodiscard]] virtual std::string_view TextBuffer() = 0;
    virtual void ShowHint(std::uint32_t window, std::string_view text,
                          float x, float y, float scale,
                          std::uint32_t mode, std::uint32_t enabled) = 0;
    [[nodiscard]] virtual std::uint32_t AudioManager() = 0;
    virtual void PlaySound(std::uint32_t audio, std::uint8_t flag,
                           std::string_view path, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint8_t CleanupTextBuffer() noexcept = 0;
};

class ScriptInventoryChange final {
public:
    // IDA 0x5d46f0, giAddProperty.
    [[nodiscard]] static std::uint8_t Add(
        std::uint32_t id, std::int32_t count, std::uint8_t show_hint,
        IScriptInventoryChangeRuntime& runtime);
    // IDA 0x5d4820, giDelProperty.
    [[nodiscard]] static std::uint8_t Remove(
        std::uint32_t id, std::int32_t count, std::uint8_t show_hint,
        IScriptInventoryChangeRuntime& runtime);
};

}  // namespace pal4
