#include "pal4/mission_window_quest_image.h"

namespace pal4 {

std::uint32_t MissionWindowQuestImage::Load(
    std::uint32_t quest, IMissionWindowQuestImageRuntime& runtime) {
    if (quest == 0u || runtime.ImageFlag(quest, 0x2cu) == 0u) return 0u;
    runtime.PreparePkFileManager();
    bool cleaned = false;
    try {
        const auto key = runtime.ImageKey(quest, 0x28u);
        const auto path = runtime.ConvertToUtf8(
            "gamedata\\ui\\quest\\" + key + ".png");
        const auto name = runtime.ConvertToUtf8("Pal4QuestImage" + key);
        if (!runtime.ImagesetPresent(name)) {
            runtime.CreateImagesetFromImageFile(name, path, "");
        }
        const auto imageset = runtime.GetImageset(name);
        runtime.SetAutoScalingEnabled(imageset, false);
        const auto image_name = runtime.ConvertToUtf8("full_image");
        cleaned = true;
        runtime.CleanupPkFileManager();
        return runtime.GetImage(imageset, image_name);
    } catch (...) {
        if (!cleaned) runtime.CleanupPkFileManager();
        throw;
    }
}

}  // namespace pal4
