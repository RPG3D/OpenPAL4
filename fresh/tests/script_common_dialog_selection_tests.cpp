#include "pal4/script_common_dialog_selection.h"

#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct Runtime final : pal4::IScriptCommonDialogSelectionRuntime {
    std::vector<std::string> calls;
    std::int32_t selection = 0;

    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x4000u;
    }
    std::uint32_t CreateCommonDialog(std::uint32_t manager) override {
        if (manager != 0x4000u)
            throw std::runtime_error("common dialog manager");
        calls.emplace_back("dialog");
        return 0x5000u;
    }
    std::int32_t LastSelection(std::uint32_t dialog) override {
        if (dialog != 0x5000u)
            throw std::runtime_error("last selection dialog");
        calls.emplace_back("selection");
        return selection;
    }
};

}  // namespace

int main() {
    try {
        for (const auto selection : {
                 std::numeric_limits<std::int32_t>::min(), -1, 0, 1, 2,
                 std::numeric_limits<std::int32_t>::max()}) {
            Runtime runtime;
            runtime.selection = selection;
            if (pal4::ScriptCommonDialogSelection::IsOne(runtime) !=
                    (selection == 1 ? 1u : 0u) ||
                runtime.calls != std::vector<std::string>{
                    "manager", "dialog", "selection"})
                throw std::runtime_error("selection comparison or call order");
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
