#pragma once

#include "pal4/script_frame_state_document.h"

#include <cstdint>

namespace pal4 {

class IScriptQuestDialogRuntime : public IScriptFrameStateRuntime {
public:
    virtual ~IScriptQuestDialogRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t CreateQuestDialog(
        std::uint32_t manager) = 0;
    virtual void ShowText(std::uint32_t dialog, const char* text) = 0;
    [[nodiscard]] virtual std::int32_t ReadSignedWord(
        std::uint32_t address) = 0;
    [[nodiscard]] std::uint32_t ResolveFrame() override;
};

class ScriptQuestDialog final {
public:
    // IDA 0x5d7ff0, giShowQuestDialog.
    [[nodiscard]] static std::uint32_t Show(
        const char* text, IScriptQuestDialogRuntime& runtime);
    // IDA 0x5d8130, giGetQuestDialogResult.
    [[nodiscard]] static std::int32_t Result(
        IScriptQuestDialogRuntime& runtime);
};

}  // namespace pal4
