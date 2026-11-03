#pragma once

#include <cstdint>

namespace pal4 {

class IScriptCommonDialogSelectionRuntime {
public:
    virtual ~IScriptCommonDialogSelectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t CreateCommonDialog(
        std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::int32_t LastSelection(
        std::uint32_t dialog) = 0;
};

class ScriptCommonDialogSelection final {
public:
    // IDA 0x5d8980, giCommonDialogGetLastSelect.
    [[nodiscard]] static std::uint32_t IsOne(
        IScriptCommonDialogSelectionRuntime& runtime);
};

}  // namespace pal4
