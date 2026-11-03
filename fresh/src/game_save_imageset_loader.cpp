#include "pal4/game_save_imageset_loader.h"

namespace pal4 {

std::uint32_t GameSaveImagesetLoader::Load(
    const std::int32_t slot, IGameSaveImagesetRuntime& runtime) {
    const auto first_packed_file_manager = runtime.PackedFileManager();
    runtime.ProcessPackedFile3(first_packed_file_manager);

    const auto path = runtime.ConvertToUtf8(
        "save\\" + std::to_string(slot) + ".png");
    const auto name = runtime.ConvertToUtf8(
        "GameSaveImage" + std::to_string(slot));

    const auto first_imageset_manager = runtime.ImagesetManager();
    if (runtime.IsImagesetPresent(first_imageset_manager, name)) {
        const auto destroy_manager = runtime.ImagesetManager();
        runtime.DestroyImageset(destroy_manager, name);
    }
    const auto create_manager = runtime.ImagesetManager();
    runtime.CreateImagesetFromImageFile(create_manager, name, path, "");
    const auto get_manager = runtime.ImagesetManager();
    const auto imageset = runtime.GetImageset(get_manager, name);
    runtime.SetAutoScalingEnabled(imageset, false);

    const auto image_name = runtime.ConvertToUtf8("full_image");
    const auto second_packed_file_manager = runtime.PackedFileManager();
    runtime.ProcessPackedFile4(second_packed_file_manager);
    return runtime.GetImage(imageset, image_name);
}

}  // namespace pal4
