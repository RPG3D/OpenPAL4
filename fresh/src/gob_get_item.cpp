#include "pal4/gob_get_item.h"
#include "pal4/gob_scene_package.h"

namespace pal4 {

std::uint32_t GobGetItem::TypeKey() { return 7u; }

std::uint32_t GobGetItem::Create(IGobGetItemRuntime& runtime) {
    const auto object = runtime.Allocate(kSize);
    if (object == 0u) return 0u;
    try {
        return Construct(object, runtime);
    } catch (...) {
        runtime.Free(object);
        throw;
    }
}

std::uint32_t GobGetItem::Construct(
    const std::uint32_t object, IGobGetItemConstructRuntime& runtime) {
    runtime.ConstructBase(object);
    runtime.WriteWord(object, kVtable);
    return object;
}

void GobGetItem::Destruct(
    const std::uint32_t object, IGobGetItemConstructRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    runtime.DestructBase(object);
}

std::uint32_t GobGetItem::InitializeSceneAndConfig(
    const std::uint32_t object, const std::uint32_t first,
    const std::string_view path, const std::uint32_t fourth,
    const std::uint32_t fifth, IGobGetItemRuntime& runtime) {
    runtime.CleanupBeforeSceneLoad(object);
    SelectGobScenePackage(path, runtime);
    const auto loaded = runtime.LoadBaseModel(
        object, first, path, fourth, fifth);
    if (loaded != 0u) {
        runtime.HideHookUserDataAtomics(
            runtime.ReadWord(object + 0x14u));
        const auto document = runtime.ReadWord(object + 0x1c0u);
        const auto parameters = runtime.FindXmlChild(
            document, "PAL4-GameObject-Parameters");
        if (parameters != 0u) {
            const auto old = runtime.FindXmlChild(
                parameters, "PAL4_GameObject-getItem");
            if (old != 0u) runtime.RemoveXmlChild(parameters, old);
            (void)runtime.CreateXmlChild(
                parameters, "PAL4_GameObject-getItem");
        }
    }
    runtime.CleanupScriptPackages();
    return loaded;
}

std::uint32_t GobGetItem::GetConfigNode(
    const std::uint32_t object, IGobGetItemRuntime& runtime) {
    const auto document = runtime.ReadWord(object + 0x1c0u);
    if (document == 0u) return 0u;
    const auto parameters = runtime.FindXmlChild(
        document, "PAL4-GameObject-Parameters");
    return parameters == 0u ? 0u :
        runtime.FindXmlChild(parameters, "PAL4_GameObject-getItem");
}

void GobGetItem::ShowGrantHint(
    const std::uint32_t window, const std::string_view name,
    const std::int32_t quantity, IGobGetItemRuntime& runtime) {
    const auto format = runtime.GameString(0xfbu);
    const auto text = runtime.FormatHint(format, name, quantity);
    runtime.ShowHint(window, text, 0.5f, 0.15f, 1.0f, 0u, 1u);
    runtime.PlayHintSound("gamedata\\PALSound\\WA06.wav", 1u);
}

std::uint32_t GobGetItem::TriggerGrantAndShowHint(
    const std::uint32_t object, const std::uint32_t context,
    IGobGetItemRuntime& runtime) {
    (void)context;
    if (runtime.ReadWord(object + 0x198u) == 0u) return 0u;
    const auto window = runtime.CreateHintWindow();
    if (window == 0u) return 0u;
    const auto node = GetConfigNode(object, runtime);
    if (node == 0u) return 0u;
    const auto property_id = runtime.ReadXmlInt(
        node, "PAL4_GameObject-getItem-PropID", 0);
    const auto equipment_id = runtime.ReadXmlInt(
        node, "PAL4_GameObject-getItem-EquipID", 0);
    const auto quantity = runtime.ReadXmlInt(
        node, "PAL4_GameObject-getItem-num", 1);
    if (property_id > 0) {
        if (runtime.PropertyExists(property_id)) {
            runtime.GrantProperty(property_id, quantity);
            ShowGrantHint(window, runtime.PropertyName(property_id),
                          quantity, runtime);
        } else {
            runtime.ShowIdError("PropID not exist", "IDError");
        }
    } else if (equipment_id > 0) {
        if (runtime.EquipmentExists(equipment_id)) {
            runtime.GrantEquipment(equipment_id);
            ShowGrantHint(window, runtime.EquipmentName(equipment_id),
                          quantity, runtime);
        } else {
            runtime.ShowIdError("EquipID not exist", "IDError");
        }
    }
    return runtime.ConsumeResearchUse(object);
}

std::uint32_t GobGetItem::CreateStateSnapshot(
    const std::uint32_t object, IGobGetItemSnapshotRuntime& runtime) {
    if (runtime.ReadWord(object + 0x1a4u) == 0u) return 0u;
    const auto snapshot = runtime.Allocate(0x1cu);
    if (snapshot == 0u) return 0u;
    try {
        runtime.ConstructSnapshot(snapshot);
        runtime.AssignSnapshotName(
            snapshot, runtime.ReadCString(runtime.ReadWord(object + 4u)));
        const auto remaining = runtime.ReadWord(object + 0x198u);
        runtime.WriteWord(snapshot + 0x18u, remaining);
        auto visible = runtime.ReadWord(object + 0x184u);
        if (remaining == 0u && runtime.ReadWord(object + 0x190u) != 0u)
            visible = 0u;
        runtime.WriteWord(snapshot + 0x14u, visible);
    } catch (...) {
        runtime.Free(snapshot);
        throw;
    }
    return snapshot;
}

std::uint32_t GobGetItem::ApplyStateSnapshot(
    const std::uint32_t object, const std::uint32_t snapshot,
    IGobGetItemStateApplyRuntime& runtime) {
    const auto enabled = runtime.ReadWord(object + 0x1a4u);
    if (enabled == 0u) return enabled;
    const auto type = runtime.SnapshotType(snapshot);
    if (type != TypeKey()) return type;
    runtime.WriteWord(object + 0x198u,
                      runtime.ReadWord(snapshot + 0x18u));
    return runtime.SetBaseVisible(
        object, runtime.ReadWord(snapshot + 0x14u));
}

}  // namespace pal4
