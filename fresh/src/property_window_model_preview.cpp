#include "pal4/property_window_model_preview.h"

#include <bit>

namespace pal4 {
namespace {

constexpr std::array<float, 3> kBaseTransform{{30.0f, 10.0f, 0.0f}};
const std::array<float, 3> kPosition{{
    std::bit_cast<float>(0xc26eae14u),
    std::bit_cast<float>(0x43aeee14u),
    std::bit_cast<float>(0x43952148u),
}};
constexpr std::array<float, 3> kRotation{{0.0f, 50.0f, 0.0f}};

}  // namespace

std::uint32_t PropertyWindowModelPreview::Update(
    PropertyWindowState& state, std::uint32_t record,
    IPropertyWindowModelPreviewRuntime& runtime) {
    if (record == 0u) {
        const auto collection = runtime.GameObjectCollection();
        const auto current_id = state.Read(0x78u);
        if (runtime.FindObject(collection, current_id) == 0u) return 0u;
        const auto result = runtime.RemoveObject(collection, current_id);
        state.Write(0x78u, 0u);
        return result;
    }

    const auto definition = runtime.ReadWord(record);
    const auto model_pointer = runtime.ReadWord(definition + 0x54u);
    const auto model_name = model_pointer != 0u
        ? model_pointer : kEmptyTextAddress;
    const auto collection = runtime.GameObjectCollection();
    const auto current_id = state.Read(0x78u);
    const auto existing = runtime.FindObject(collection, current_id);
    const bool reuse = existing != 0u && runtime.CompareNamesIgnoreCase(
        runtime.ReadWord(existing + 4u), model_name) == 0;

    runtime.PrepareScripts();
    try {
        if (reuse) {
            const auto texture_pointer = runtime.ReadWord(definition + 0x64u);
            runtime.SetTexture(existing, texture_pointer != 0u
                ? texture_pointer : kEmptyTextAddress);
        } else {
            (void)runtime.RemoveObject(collection, current_id);
            const auto object = runtime.CreateObject(
                collection, "palObject\\", model_name, 1u);
            if (object != 0u && runtime.ReadWord(object + 0x14u) != 0u) {
                const auto texture_pointer = runtime.ReadWord(definition + 0x64u);
                runtime.SetTexture(object, texture_pointer != 0u
                    ? texture_pointer : kEmptyTextAddress);
                state.Write(0x78u, runtime.ReadWord(object + 0x170u));
                runtime.SetBaseTransform(object, kBaseTransform, 0u);
                runtime.SetPosition(object, kPosition);
                runtime.SetRotation(object, 1u, kRotation);
            }
        }
    } catch (...) {
        (void)runtime.CleanupScripts();
        throw;
    }
    return runtime.CleanupScripts();
}

}  // namespace pal4
