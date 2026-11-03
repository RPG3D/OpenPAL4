#pragma once
#include "pal4/script_inventory_change.h"
#include <functional>
#include <string>
namespace pal4 {
class NativeInventorySession;
class NativeGameDatabase;
// Bridges original inventory semantics to independently owned UI/audio sessions.
class NativeScriptInventoryChange final : public IScriptInventoryChangeRuntime {
public:
    using CreateHint = std::function<std::uint32_t()>;
    using Show = std::function<void(std::uint32_t,std::string_view,float,float,float,std::uint32_t,std::uint8_t)>;
    using Sound = std::function<void(std::string_view,std::uint32_t)>;
    NativeScriptInventoryChange(NativeInventorySession& inventory, const NativeGameDatabase& database,
        CreateHint create, Show show, Sound sound);
    std::uint32_t UiFrameManager() override;
    std::uint32_t CreateHintWindow(std::uint32_t frames) override;
    std::uint32_t GameDatabase() override;
    std::uint8_t HasProperty(std::uint32_t database,std::uint32_t id) override;
    std::uint32_t FindProperty(std::uint32_t database,std::uint32_t id) override;
    std::uint32_t Inventory() override;
    void AddItem(std::uint32_t inventory,std::uint32_t id,std::int32_t count) override;
    void RemoveItem(std::uint32_t inventory,std::uint32_t id,std::int32_t count) override;
    void InitializeTextBuffer() override;
    std::string_view PropertyName(std::uint32_t property) override;
    std::string_view GameString(std::uint32_t table,std::uint32_t id) override;
    void FormatText(std::string_view format,std::string_view name,std::int32_t count) override;
    std::string_view TextBuffer() override;
    void ShowHint(std::uint32_t window,std::string_view text,float x,float y,float duration,std::uint32_t parent,std::uint32_t flag) override;
    std::uint32_t AudioManager() override;
    void PlaySound(std::uint32_t audio,std::uint8_t flag,std::string_view path,std::uint32_t mode) override;
    std::uint8_t CleanupTextBuffer() noexcept override;
private:
    NativeInventorySession& inventory_;
    const NativeGameDatabase& database_;
    CreateHint create_;
    Show show_;
    Sound sound_;
    std::string text_;
};
}
