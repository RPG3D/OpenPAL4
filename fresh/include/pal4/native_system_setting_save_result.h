#pragma once
#include "pal4/binary_xml_config.h"
#include <windows.h>
#include <filesystem>
#include <functional>
#include <cstdint>

namespace pal4 {
class NativeCeguiUiSession;
class NativeGameDatabase;
class DialogManagerState;
class IDialogManagerSettingsWriteRuntime;

// Borrows live page/settings. Game services are mandatory: an unfinished save
// writer or state owner must report its failure rather than imply success.
// Loading receives the original temporary config; consume/copy it synchronously.
class NativeSystemSettingSaveResult final {
public:
    using SaveSlot = std::function<void(std::int32_t)>;
    using UiEvent = std::function<void(std::uint32_t, std::uint32_t, std::uint32_t)>;
    using EnterLoading = std::function<void(const BinaryXmlNode&)>;
    NativeSystemSettingSaveResult(HMODULE module, NativeCeguiUiSession& ui,
        const std::filesystem::path& game_root, const NativeGameDatabase& database,
        DialogManagerState& globals, IDialogManagerSettingsWriteRuntime& settings,
        SaveSlot save_slot, UiEvent ui_event, EnterLoading enter_loading);
    [[nodiscard]] std::uint8_t Save(std::uint32_t selection);
    [[nodiscard]] std::uint8_t Load(std::uint32_t selection);
private:
    class Runtime;
    HMODULE module_;
    NativeCeguiUiSession& ui_;
    std::filesystem::path game_root_;
    const NativeGameDatabase& database_;
    DialogManagerState& globals_;
    IDialogManagerSettingsWriteRuntime& settings_;
    SaveSlot save_slot_;
    UiEvent ui_event_;
    EnterLoading enter_loading_;
};
} // namespace pal4
