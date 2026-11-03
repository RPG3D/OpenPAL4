#include "pal4/minimap_file_loader.h"

#include <new>
#include <utility>

namespace pal4 {

MinimapLayerCollection& MinimapFileLoader::Load(
    MinimapLayerCollection& output, const std::string& folder,
    const std::string& scene, const std::uint8_t incoming_header_byte,
    IMinimapFileRuntime& runtime) {
    runtime.EnterPackageScope();
    runtime.InitializeXmlDocument();
    std::vector<std::uint32_t> objects;
    const auto scene_path = runtime.BasePath() + folder + "\\" + scene;
    const auto file = runtime.OpenRead(scene_path + "\\MiniMap.df");
    if (file != 0u) {
        runtime.InitializeFileStream(file);
        runtime.SetImageBasePath(scene_path + "\\");
        std::int32_t count = 0;
        runtime.ReadBytes(file, &count, sizeof(count));
        bool all_images_found = true;
        for (std::int32_t index = 0; index < count; ++index) {
            std::uint32_t scale = 0u;
            std::array<std::uint8_t, 24> metadata{};
            runtime.ReadBytes(file, &scale, sizeof(scale));
            runtime.ReadBytes(file, metadata.data(),
                              static_cast<std::uint32_t>(metadata.size()));
            const auto image = runtime.FindImagePath(
                "MiniMap_" + std::to_string(index) + ".png");
            if (image == 0u) {
                all_images_found = false;
                break;
            }
            const auto object = runtime.AllocateGridObject(32u);
            if (object == 0u) throw std::bad_alloc();
            runtime.ConstructGridObject(object);
            runtime.InitializeGridObject(object, image, scale, metadata);
            objects.push_back(object);
        }
        if (all_images_found) runtime.CloseFile(file);
    }
    runtime.LeavePackageScope();
    output.header_byte = incoming_header_byte;
    output.grid_objects = std::move(objects);
    runtime.DestroyXmlDocument();
    return output;
}

}  // namespace pal4
