#pragma once

#include "pal4/script_frame_state_document.h"

#include <cstdint>

namespace pal4 {

using ScriptInnDialogAttributeLookup = ScriptFrameAttributeLookup;

class IScriptInnDialogRuntime : public IScriptFrameStateRuntime {
public:
    virtual ~IScriptInnDialogRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void ProcessEvent(std::uint32_t dispatcher, std::uint32_t key_cell,
                              std::uint32_t kind, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t FindFrame(
        std::uint32_t manager, std::uint32_t key_cell) = 0;
    [[nodiscard]] std::uint32_t ResolveFrame() override;
    [[nodiscard]] virtual std::int32_t ReadSignedWord(
        std::uint32_t address) = 0;
};

class ScriptInnDialog final {
public:
    static constexpr std::uint32_t kEventKeyCell = 0x8a80c8u;
    static constexpr std::uint32_t kFrameKeyCell = 0x8b0660u;

    // IDA 0x5d7b10, giShowInnDialog.
    [[nodiscard]] static std::uint32_t Show(std::uint32_t enabled,
                                            IScriptInnDialogRuntime& runtime);
    // IDA 0x5d7c60, giGetInnDialogResult.
    [[nodiscard]] static std::int32_t Result(IScriptInnDialogRuntime& runtime);
};

}  // namespace pal4
