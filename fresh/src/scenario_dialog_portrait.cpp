#include "pal4/scenario_dialog.h"

#include <cstdint>
#include <string>

namespace pal4 {

namespace {

constexpr const char* kPortraitDirectory =
    "gamedata\\ui\\portrait\\";

std::string NumberedImageset(const unsigned index) {
    return "portrait" + std::to_string(index) + "0";
}

std::string NumberedFile(const unsigned index) {
    return std::string(kPortraitDirectory) + "portrait" +
           std::to_string(index) + "_0.imageset";
}

void SetDefault(const std::uint32_t window,
                IScenarioPortraitRuntime& runtime) {
    runtime.SetImage(window,
                     runtime.GetImage("zhujiemian0", "default"));
}

}  // namespace

std::uint32_t ScenarioDialog::SetPortrait(
    const std::uint32_t object, const std::string& name,
    const std::uint32_t position_option,
    IScenarioPortraitRuntime& runtime) {
    runtime.BeginResources();
    const auto window = runtime.ReadWord(object + 44u);
    runtime.SetImage(window, 0u);
    const auto tree = object + 64u;
    const auto cached = runtime.FindCache(tree, name);
    if (cached) {
        if (cached->imageset.empty()) {
            runtime.SetImage(window, 0u);
            SetDefault(window, runtime);
        } else {
            if (!runtime.ImagesetPresent(cached->imageset)) {
                auto file = std::string(kPortraitDirectory) +
                            cached->imageset;
                file.pop_back();
                file += "_0.imageset";
                runtime.CreateImageset(file);
            }
            runtime.SetImage(window,
                             runtime.GetImage(cached->imageset,
                                              cached->image));
        }
    } else {
        // The original counter is a byte and wraps after 255. Resource packs
        // terminate this scan at the first missing numbered file.
        for (unsigned index = 0u; index < 256u; ++index) {
            const auto file = NumberedFile(index);
            if (!runtime.PortraitFileExists(file)) break;
            const auto imageset = NumberedImageset(index);
            if (!runtime.ImagesetPresent(imageset))
                runtime.CreateImageset(file);
            if (!runtime.ImageDefined(imageset, name)) continue;
            runtime.InsertCache(tree, name, {imageset, name});
            runtime.SetImage(window,
                             runtime.GetImage(imageset, name));
            break;
        }
    }
    if (runtime.ReadWord(window + 0x6b0u) == 0u) {
        runtime.InsertCache(tree, name, {"", name});
        SetDefault(window, runtime);
    }
    runtime.SetTextAndImagePositions(object, position_option);
    return runtime.EndResources();
}

}  // namespace pal4
