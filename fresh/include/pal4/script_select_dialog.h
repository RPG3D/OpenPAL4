#pragma once

#include <cstdint>

namespace pal4 {

class IScriptSelectDialogRuntime {
public:
    virtual ~IScriptSelectDialogRuntime() = default;
    [[nodiscard]] virtual std::uint32_t UiFrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t SelectDialog(
        std::uint32_t frames) = 0;
    [[nodiscard]] virtual std::int32_t AddButton(
        std::uint32_t dialog, std::uint32_t string_object) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class ScriptSelectDialog final {
public:
    // IDA 0x5d6e90, giSelectDialogAddItem.
    [[nodiscard]] static std::int32_t AddItem(
        std::uint32_t string_object, IScriptSelectDialogRuntime& runtime);
    // IDA 0x5d6eb0, giSelectDialogGetLastSelect.
    [[nodiscard]] static std::int32_t LastSelection(
        IScriptSelectDialogRuntime& runtime);
    // IDA 0x5d7fd0, giSelectDialogSetDefaultSelect.
    [[nodiscard]] static std::uint32_t SetDefaultSelection(
        std::int32_t selection, IScriptSelectDialogRuntime& runtime);
};

}  // namespace pal4
