#include "pal4/trade_window_model_display.h"

#include <cctype>

namespace pal4 {
namespace {

bool EqualsIgnoreCase(const std::string_view left,
                      const std::string_view right) {
    if (left.size() != right.size()) return false;
    for (std::size_t i = 0; i < left.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(left[i])) !=
            std::tolower(static_cast<unsigned char>(right[i]))) {
            return false;
        }
    }
    return true;
}

}  // namespace

std::uint32_t TradeWindowModelDisplay::SetGameObjectDisplay(
    const std::uint32_t frame, const std::optional<std::string_view> name,
    const std::uint32_t texture, const std::optional<std::string_view> path,
    ITradeWindowModelRuntime& runtime) {
    const auto collection = runtime.ReadWord(runtime.FrameManager() + 0x19cu);
    const auto old_id = runtime.ReadWord(frame + 0x11cu);
    if (!name) {
        const auto removed = runtime.RemoveGameObject(collection, old_id);
        runtime.WriteWord(frame + 0x11cu, 0u);
        return removed;
    }

    const auto previous = runtime.FindGameObject(collection, old_id);
    if (previous != 0u &&
        EqualsIgnoreCase(runtime.ObjectName(previous), *name)) {
        return runtime.SetTexture(previous, texture);
    }
    static_cast<void>(runtime.RemoveGameObject(collection, old_id));

    const auto target_path = path.value_or("palObject\\");
    const auto object_path = EqualsIgnoreCase(target_path, "palObject\\");
    const auto weapon_path = EqualsIgnoreCase(target_path, "palWeapon\\");
    if (!object_path && !weapon_path) return runtime.CleanupScripts();
    if (object_path) {
        runtime.ProcessObjectScript();
    } else {
        runtime.ProcessWeaponScript();
    }
    const auto created = runtime.CreateGameObject(
        collection, target_path, *name, true);
    if (created == 0u) return runtime.CleanupScripts();
    if (weapon_path) {
        runtime.SetScaleFromMember(created, created + 0xc4u);
    }

    static_cast<void>(runtime.SetTexture(created, texture));
    runtime.WriteWord(frame + 0x11cu,
                      runtime.ReadWord(created + 0x170u));
    runtime.SetObjectVector(created, {30.0f, 10.0f, 0.0f}, 0u);
    runtime.SetPosition(created, {-59.669998f, 369.85999f, 298.26001f});
    runtime.SetRotation(created, 1u, {0.0f, 50.0f, 0.0f});
    return runtime.CleanupScripts();
}

}  // namespace pal4
