#include "pal4/equipment_model_preview.h"

#include <cstdint>
#include <string>

namespace pal4 {
namespace {

constexpr EquipmentPreviewVector kItemPosition{-59.669998f, 349.85999f,
                                                298.26001f};
constexpr EquipmentPreviewVector kItemTransform{30.0f, 10.0f, 0.0f};
constexpr EquipmentPreviewVector kItemRotation{0.0f, 50.0f, 0.0f};
constexpr EquipmentPreviewVector kEffectPosition{-59.669998f, 200.86f,
                                                  298.26001f};
constexpr EquipmentPreviewVector kFlowerPosition{-59.669998f, 309.85999f,
                                                  298.26001f};

std::string ReadOptionalText(IEquipmentModelPreviewRuntime& runtime,
                             std::uint32_t address) {
    return address == 0u ? std::string{} : runtime.ReadText(address);
}

EquipmentPreviewEffectHandle EffectHandle(const EquipmentWindowState& state) {
    return {state.Read(0x104u), state.Read(0x108u)};
}

}  // namespace

void EquipmentModelPreview::UpdateItem(
    EquipmentWindowState& state, std::uint32_t item,
    IEquipmentModelPreviewRuntime& runtime) {
    if (item == 0u) return;

    const auto existing = runtime.FindObject(state.Read(0xf8u));
    bool same_name = false;
    if (existing != 0u) {
        const auto item_name = runtime.ItemName(item);
        const auto object_name = runtime.ObjectName(existing);
        same_name = item_name == object_name;
    }
    if (same_name) {
        const auto category = runtime.ReadWord(item + 0x94u);
        if (category > 6u) return;
        runtime.LoadItemScript(category <= 2u);
        runtime.SetTexture(existing, ReadOptionalText(
            runtime, runtime.ReadWord(item + 0xecu)));
        runtime.CleanupScripts();
        return;
    }

    runtime.RemoveObject(state.Read(0xf8u));
    const auto category = runtime.ReadWord(item + 0x94u);
    if (category > 6u) return;

    const bool weapon = category <= 2u;
    runtime.LoadItemScript(weapon);
    const auto object = runtime.CreateObject(
        weapon ? "PALWeapon\\" : "palObject\\",
        ReadOptionalText(runtime, runtime.ReadWord(item + 0xdcu)), 1u);
    if (object != 0u) {
        runtime.SetTexture(object, ReadOptionalText(
            runtime, runtime.ReadWord(item + 0xecu)));
        state.Write(0xf8u, runtime.ObjectId(object));
        if (weapon) runtime.SetScaleFromMember(object, 0xc4u);
        runtime.SetTransform(object, kItemTransform, 0u);
        runtime.SetPosition(object, kItemPosition);
        runtime.SetRotation(object, 1u, kItemRotation);
    }
    runtime.CleanupScripts();
}

void EquipmentModelPreview::SetVisible(
    EquipmentWindowState& state, bool visible,
    IEquipmentModelPreviewRuntime& runtime) {
    if (!visible) {
        runtime.RemoveObject(state.Read(0xfcu));
        runtime.RemoveObject(state.Read(0x100u));
        runtime.RemoveObject(state.Read(0xf8u));
        state.Write(0xfcu, 0u);
        state.Write(0x100u, 0u);
        state.Write(0xf8u, 0u);
        runtime.CloseEffect(EffectHandle(state), 0u);
        return;
    }

    if (runtime.FindObject(state.Read(0xfcu)) != 0u) return;
    const auto effect = runtime.CreateEffect("LH", kEffectPosition, -1, 0u);
    state.Write(0x104u, effect.first);
    state.Write(0x108u, effect.second);
    runtime.ResetTemporaryEffect();
    runtime.SetEffectScale(effect, {2.2f, 2.2f, 2.2f}, false);
    runtime.MarkEffectDirty(effect);
    runtime.LoadFlowerScript();
    const auto flower = runtime.CreateObject("ui\\model\\", "_ui_flower", 1u);
    if (flower != 0u) {
        state.Write(0xfcu, runtime.ObjectId(flower));
        runtime.SetScale(flower, {3.5f, 3.5f, 3.5f});
        runtime.SetPosition(flower, kFlowerPosition);
    }
    runtime.CleanupScripts();
}

}  // namespace pal4
