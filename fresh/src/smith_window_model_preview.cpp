#include "pal4/smith_window_model_preview.h"

#include <array>
#include <bit>
#include <cctype>
#include <cstdint>
#include <string_view>

namespace pal4 {
namespace {

constexpr std::array<float, 3> kFlowerScale{3.5f, 3.5f, 3.5f};
constexpr std::array<float, 3> kFlowerPosition{
    std::bit_cast<float>(0xc26eae14u),
    std::bit_cast<float>(0x439aee14u),
    std::bit_cast<float>(0x43952148u),
};
constexpr std::array<float, 3> kModelPosition{
    std::bit_cast<float>(0xc26eae14u),
    std::bit_cast<float>(0x43b8ee14u),
    std::bit_cast<float>(0x43952148u),
};
constexpr std::array<float, 3> kModelVector{30.0f, 10.0f, 0.0f};
constexpr std::array<float, 3> kModelRotation{0.0f, 50.0f, 0.0f};

bool EqualsIgnoreCase(const std::string_view left,
                      const std::string_view right) {
    if (left.size() != right.size()) return false;
    for (std::size_t index = 0; index < left.size(); ++index) {
        if (std::tolower(static_cast<unsigned char>(left[index])) !=
            std::tolower(static_cast<unsigned char>(right[index]))) {
            return false;
        }
    }
    return true;
}

}  // namespace

std::uint32_t SmithWindowModelPreview::Update(
    const std::uint32_t object,
    const std::optional<std::string_view> model_name,
    const std::optional<std::string_view> texture,
    const std::optional<std::string_view> class_path,
    ISmithWindowModelPreviewRuntime& runtime) {
    const auto initial_collection = runtime.FrameCollection();
    (void)runtime.RemoveGameObject(initial_collection,
                                   runtime.ReadWord(object + 0x9cu));
    auto result = runtime.RemoveGameObject(initial_collection,
                                           runtime.ReadWord(object + 0xa0u));
    runtime.WriteWord(object + 0x9cu, 0u);
    runtime.WriteWord(object + 0xa0u, 0u);
    if (!model_name) return result;

    runtime.LoadFlowerScript();
    const auto flower = runtime.CreateGameObject(
        runtime.FrameCollection(), "ui\\model\\", "_ui_flower", true);
    if (flower != 0u) {
        runtime.WriteWord(object + 0xa0u,
                          runtime.ReadWord(flower + 0x170u));
        runtime.SetScale(flower, kFlowerScale);
        runtime.SetPosition(flower, kFlowerPosition);
    }
    result = runtime.CleanupScripts();

    const auto collection = runtime.FrameCollection();
    const auto existing = runtime.FindGameObject(
        collection, runtime.ReadWord(object + 0x9cu));
    if (existing != 0u) {
        result = EqualsIgnoreCase(runtime.ObjectName(existing), *model_name)
                     ? 0u
                     : 1u;
        if (result == 0u) return result;
    }
    (void)runtime.RemoveGameObject(collection,
                                   runtime.ReadWord(object + 0x9cu));

    const auto path = class_path.value_or("palObject\\");
    if (EqualsIgnoreCase(path, "palObject\\")) {
        runtime.LoadObjectScript();
    } else if (EqualsIgnoreCase(path, "palWeapon\\")) {
        runtime.LoadWeaponScript();
    } else {
        return runtime.CleanupScripts();
    }

    const auto model = runtime.CreateGameObject(
        collection, path, *model_name, true);
    if (model != 0u) {
        runtime.SetTexture(model, texture);
        runtime.WriteWord(object + 0x9cu,
                          runtime.ReadWord(model + 0x170u));
        runtime.SetObjectVector(model, kModelVector, 0u);
        runtime.SetPosition(model, kModelPosition);
        runtime.SetRotation(model, 1u, kModelRotation);
    }
    return runtime.CleanupScripts();
}

}  // namespace pal4
