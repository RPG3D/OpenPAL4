#include "pal4/scenario_dialog.h"

#include <array>
#include <cstdint>
#include <string>

namespace pal4 {

std::uint32_t ScenarioDialog::Construct(
    const std::uint32_t object, IScenarioDialogRuntime& runtime) {
    unsigned stage = 0u;
    try {
        runtime.LoadLayout(object, "ScenarioDialog");
        stage = 1u;
        runtime.WriteWord(object + 40u, kBaseTreeVtable);
        const auto tree_byte = runtime.IndeterminateTreeByte();
        runtime.WriteByte(object + 64u, tree_byte);
        runtime.WriteByte(object + 65u, tree_byte);
        runtime.WriteByte(object + 72u, 0u);
        runtime.InitializeTree(object + 64u);
        stage = 2u;
        runtime.WriteWord(object, kVtable);
        runtime.WriteWord(object + 40u, kTreeVtable);
        runtime.WriteByte(object + 60u, 0u);

        const auto prefix = runtime.ReadLayoutPrefix(object);
        constexpr std::array<const char*, 4> suffixes{
            "EditDialog", "ImgPortrait", "ImgClick", "ScenarioDialog"};
        constexpr std::array<std::uint32_t, 4> offsets{
            56u, 44u, 48u, 52u};
        for (std::size_t index = 0u; index < suffixes.size(); ++index) {
            const auto window = runtime.FindWindow(
                object, prefix + suffixes[index]);
            runtime.WriteWord(object + offsets[index], window);
            if (index == 0u || index == 3u)
                runtime.SetEnabled(window, false);
        }
        runtime.SetEnabled(runtime.ReadWord(object + 44u), false);
        runtime.SetAlwaysOnTop(runtime.ReadWord(object + 36u), true);
        runtime.RegisterEditboxEvent(object);
        runtime.SetTextAndImagePositions(object, 0u);
        return object;
    } catch (...) {
        if (stage >= 2u) runtime.DestructTree(object + 64u);
        if (stage >= 1u) runtime.DestructBase(object);
        throw;
    }
}

}  // namespace pal4
