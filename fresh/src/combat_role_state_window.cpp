#include "pal4/combat_role_state_window.h"

#include <array>
#include <bit>

namespace pal4 {
namespace {

using ColourBits = std::array<std::uint32_t, 4>;
constexpr ColourBits kWhite{{0x3f800000u, 0x3f800000u, 0x3f800000u,
                             0x3f800000u}};
constexpr ColourBits kBlack{{0u, 0u, 0u, 0x3f800000u}};
constexpr std::array<ColourBits, 5> kElementColours{{
    {{0x3ed6d6d7u, 0x3f119192u, 0x3f6bebecu, 0x3f800000u}},
    {{0x3f66e6e7u, 0x3f028283u, 0x3ec8c8c9u, 0x3f800000u}},
    {{0x3f4dcdceu, 0x3f169697u, 0x3f66e6e7u, 0x3f800000u}},
    {{0x3f21a1a2u, 0x3f52d2d3u, 0x3f109091u, 0x3f800000u}},
    {{0x3f48c8c9u, 0x3f109091u, 0x3e808081u, 0x3f800000u}},
}};
constexpr ColourBits kBlinkDark{{0x3f139394u, 0x3f119192u,
                                  0x3f4acacbu, 0x3f800000u}};
constexpr ColourBits kBlinkBright{{0x3f51d1d2u, 0x3f4ececfu,
                                    0x3f69e9eau, 0x3f800000u}};
constexpr std::uint32_t kBlinkByte = 0x8f95b4u;

bool HasEligibleCandidate(ICombatRoleStateDisplayRuntime& runtime) {
    const auto iterator = runtime.OpenCandidateIterator();
    struct IteratorGuard {
        ICombatRoleStateDisplayRuntime& runtime;
        std::uint32_t handle;
        ~IteratorGuard() { if (handle) runtime.ReleaseIterator(handle); }
    } guard{runtime, iterator};
    while (!runtime.IteratorAtEnd(iterator)) {
        const auto actor = runtime.CurrentActor(iterator);
        if (!runtime.ActorCheckState3(actor) &&
            runtime.ActorIsStateZero(actor) &&
            runtime.ActorActionCount(actor) >= 3)
            return true;
        runtime.AdvanceIterator(iterator);
    }
    return false;
}

void EnableSubstitutionButtons(const std::uint32_t window,
                               const bool enabled,
                               ICombatRoleStateDisplayRuntime& runtime) {
    for (std::uint32_t role = 0u; role < 3u; ++role)
        runtime.SetControlEnabled(runtime.ReadWord(window + 256u + 4u * role),
                                  enabled);
}

std::uint32_t VisitActiveRoles(const std::uint32_t window,
                               const bool mark_dirty,
                               ICombatRoleStateDisplayRuntime& runtime) {
    struct ActiveRoleIteratorGuard {
        ICombatRoleStateDisplayRuntime& runtime;
        std::uint32_t handle;
        ~ActiveRoleIteratorGuard() {
            if (handle) (void)runtime.ReleaseActiveRoleIterator(handle);
        }
        std::uint32_t Release() {
            if (!handle) return 0u;
            const auto held = handle;
            handle = 0u;
            return runtime.ReleaseActiveRoleIterator(held);
        }
    } iterator{runtime, runtime.OpenActiveRoleIterator()};
    while (!runtime.IteratorAtEnd(iterator.handle)) {
        const auto actor = runtime.CurrentActor(iterator.handle);
        if (mark_dirty) runtime.WriteByte(actor + 892u, 1u);
        (void)CombatRoleStateWindow::UpdateCharacterImages(window, actor,
                                                            runtime);
        runtime.AdvanceIterator(iterator.handle);
    }
    return iterator.Release();
}

}  // namespace

std::uint32_t CombatRoleStateWindow::Construct(
    const std::uint32_t window, const std::uint32_t layout_name,
    ICombatRoleStateWindowRuntime& runtime) {
    runtime.LoadLayout(window, layout_name);
    for (std::uint32_t index = 0u;
         index < CombatRoleStateSubscriptions::kCount; ++index) {
        runtime.WriteWord(window + 40u + 8u * index, 0u);
        runtime.WriteWord(window + 44u + 8u * index, 0u);
    }
    runtime.WriteWord(window, kVtable);
    (void)InitializeControls(window, runtime);
    (void)CombatRoleStateSubscriptions::Subscribe(window, runtime);
    return window;
}

void CombatRoleStateWindow::AddTimer(
    const std::uint32_t queue, const std::uint32_t name,
    const std::uint32_t period_bits, const std::uint32_t callback,
    ICombatRoleStateWindowRuntime& runtime) {
    TimerQueueTask::InsertOwnedCallback(queue, name, period_bits,
                                        callback, runtime);
}

void CombatRoleStateWindow::UpdateDisplay(
    const std::uint32_t window, const std::int32_t index,
    ICombatRoleStateDisplayRuntime& runtime) {
    if (index >= 3) return;
    const auto slot = static_cast<std::uint32_t>(index) * 4u;
    runtime.SetImage(runtime.ReadWord(window + 184u + slot), 0u);
    runtime.SetWidth(runtime.ReadWord(window + 196u + slot), 0.0f);
    runtime.SetWidth(runtime.ReadWord(window + 220u + slot), 0.0f);
    runtime.SetWidth(runtime.ReadWord(window + 208u + slot), 0.0f);
    runtime.SetText(runtime.ReadWord(window + 148u + slot), "");
    runtime.SetText(runtime.ReadWord(window + 160u + slot), "");
    runtime.SetText(runtime.ReadWord(window + 172u + slot), "");
    runtime.SetControlVisible(runtime.ReadWord(window + 232u + slot), false);
    const auto button = runtime.ReadWord(window + 256u + slot);
    runtime.WriteWord(button + 280u, 0u);

    if (!runtime.IsCommandOrTargetSelection()) {
        runtime.SetControlEnabled(button, false);
        return;
    }

    EnableSubstitutionButtons(window, HasEligibleCandidate(runtime), runtime);
}

std::int32_t CombatRoleStateWindow::UpdateCharacterDisplay(
    const std::uint32_t window, const std::uint32_t actor,
    ICombatRoleStateDisplayRuntime& runtime) {
    const auto index = runtime.ActorActionCount(actor);
    if (index >= 3) return index;
    const auto slot = static_cast<std::uint32_t>(index) * 4u;
    const auto portrait = runtime.ReadWord(actor + 1192u);
    runtime.SetControlVisible(runtime.ReadWord(window + 232u + slot), true);
    runtime.SetImage(runtime.ReadWord(window + 184u + slot),
                     runtime.GetIconImage(portrait, 1u, 0u));

    constexpr std::array<std::uint32_t, 3> kStats{{7u, 9u, 8u}};
    constexpr std::array<std::uint32_t, 3> kTextOffsets{{148u, 172u, 160u}};
    for (std::uint32_t i = 0u; i < kStats.size(); ++i) {
        const auto text = runtime.ConvertToUtf8(
            std::to_string(runtime.ActorStat(actor, kStats[i])));
        runtime.SetText(runtime.ReadWord(window + kTextOffsets[i] + slot),
                        text);
    }

    const auto hp = runtime.ActorStat(actor, 7u);
    const auto hp_width = std::bit_cast<float>(
        runtime.ReadWord(window + 112u + slot));
    const auto hp_product = static_cast<float>(
        static_cast<double>(hp) * hp_width);
    const auto hp_max = runtime.ActorStatMaximum(actor, 0u);
    runtime.SetWidth(runtime.ReadWord(window + 196u + slot),
                     static_cast<float>(
                         static_cast<double>(hp_product) / hp_max));
    const auto mp = runtime.ActorStat(actor, 9u);
    const auto mp_width = std::bit_cast<float>(
        runtime.ReadWord(window + 124u + slot));
    const auto mp_product = static_cast<float>(
        static_cast<double>(mp) * mp_width);
    const auto mp_max = runtime.ActorStatMaximum(actor, 2u);
    runtime.SetWidth(runtime.ReadWord(window + 208u + slot),
                     static_cast<float>(
                         static_cast<double>(mp_product) / mp_max));
    const auto rage = runtime.ActorStat(actor, 8u);
    const auto rage_width = std::bit_cast<float>(
        runtime.ReadWord(window + 136u + slot));
    runtime.SetWidth(runtime.ReadWord(window + 220u + slot),
                     static_cast<float>(
                         static_cast<double>(rage) * rage_width *
                         0.0099999998f));

    runtime.SetBackgroundImage(runtime.ReadWord(window + 244u + slot),
                               runtime.ReadWord(window + 272u + 4u * portrait));
    runtime.WriteWord(runtime.ReadWord(window + 256u + slot) + 280u, actor);
    return static_cast<std::int32_t>(runtime.CleanupTemporaryString());
}

std::uint8_t CombatRoleStateWindow::UpdateCharacterImages(
    const std::uint32_t window, const std::uint32_t actor,
    ICombatRoleStateDisplayRuntime& runtime) {
    if (runtime.ReadByte(actor + 892u) == 0u) return 0u;
    runtime.WriteByte(actor + 892u, 0u);
    const auto index = runtime.ActorActionCount(actor);
    if (index >= 3) return static_cast<std::uint8_t>(index);
    const auto slot = static_cast<std::uint32_t>(index);
    for (std::uint32_t status = 0u; status < 10u; ++status) {
        const auto control = runtime.ReadWord(
            window + 404u + 40u * slot + 4u * status);
        runtime.SetControlVisible(control, false);
        runtime.SetImage(control, 0u);
    }

    struct StatusIteratorGuard {
        ICombatRoleStateDisplayRuntime& runtime;
        std::uint32_t handle;
        ~StatusIteratorGuard() {
            if (handle) (void)runtime.ReleaseStatusIterator(handle);
        }
        std::uint8_t Release() {
            if (!handle) return 0u;
            const auto held = handle;
            handle = 0u;
            return runtime.ReleaseStatusIterator(held);
        }
    };
    std::uint32_t used = 0u;
    const auto collect = [&](const std::uint32_t iterator) {
        while (!runtime.StatusIteratorAtEnd(iterator) && used < 10u) {
            const auto value = runtime.StatusValue(iterator);
            if (value > 0 && value < 29) {
                const auto control = runtime.ReadWord(
                    window + 404u + 40u * slot + 4u * used++);
                runtime.SetImage(control, runtime.ReadWord(
                    window + 288u + 4u * static_cast<std::uint32_t>(value)));
                runtime.SetControlVisible(control, true);
            }
            runtime.AdvanceStatusIterator(iterator);
        }
    };
    StatusIteratorGuard primary{runtime,
        runtime.OpenStatusIterator(actor, false)};
    collect(primary.handle);
    StatusIteratorGuard secondary{runtime,
        runtime.OpenStatusIterator(actor, true)};
    collect(secondary.handle);
    auto result = secondary.Release();
    if (primary.handle) result = primary.Release();
    return result;
}

std::uint32_t CombatRoleStateWindow::UpdateAllCharacterImages(
    const std::uint32_t window, ICombatRoleStateDisplayRuntime& runtime) {
    return VisitActiveRoles(window, false, runtime);
}

std::uint8_t CombatRoleStateWindow::OnCharacterImageEvent(
    const std::uint32_t window, const std::uint32_t parameter,
    ICombatRoleStateDisplayRuntime& runtime) {
    const auto actor = runtime.GetDialogDataFromParam(parameter);
    if (actor && runtime.ReadWord(actor + 804u) == 1u) {
        runtime.WriteByte(actor + 892u, 1u);
        return UpdateCharacterImages(window, actor, runtime);
    }
    return static_cast<std::uint8_t>(actor);
}

std::int32_t CombatRoleStateWindow::OnCharacterDisplayEvent(
    const std::uint32_t window, const std::uint32_t parameter,
    ICombatRoleStateDisplayRuntime& runtime) {
    const auto actor = runtime.GetDialogDataFromParam(parameter);
    if (!actor) return 0;
    return UpdateCharacterDisplay(window, actor, runtime);
}

void CombatRoleStateWindow::OnRoleDisplayEvent(
    const std::uint32_t window, const std::uint32_t parameter,
    ICombatRoleStateDisplayRuntime& runtime) {
    const auto actor = runtime.GetDialogDataFromParam(parameter);
    UpdateDisplay(window, runtime.ActorActionCount(actor), runtime);
}

void CombatRoleStateWindow::RefreshSubstitutionButtons(
    const std::uint32_t window,
    ICombatRoleStateDisplayRuntime& runtime) {
    EnableSubstitutionButtons(window, HasEligibleCandidate(runtime), runtime);
}

void CombatRoleStateWindow::DisableSubstitutionButtons(
    const std::uint32_t window,
    ICombatRoleStateDisplayRuntime& runtime) {
    EnableSubstitutionButtons(window, false, runtime);
}

std::uint32_t CombatRoleStateWindow::RefreshCombatantVisibility(
    const std::uint32_t window,
    ICombatRoleStateDisplayRuntime& runtime) {
    const auto visible = runtime.PlayerRosterCount() > 3;
    for (std::uint32_t role = 0u; role < 3u; ++role)
        runtime.SetControlVisible(runtime.ReadWord(window + 256u +
                                                  4u * role), visible);
    return VisitActiveRoles(window, true, runtime);
}

std::int32_t CombatRoleStateWindow::OnCombatantStatEvent(
    const std::uint32_t window, const std::uint32_t parameter,
    ICombatRoleStateDisplayRuntime& runtime) {
    const auto actor = runtime.GetDialogDataFromParam(parameter);
    if (!actor) return 0;
    const auto index = runtime.ActorActionCount(actor);
    if (index >= 3) return index;
    const auto stat = runtime.ReadWord(parameter + 4u);
    if (stat == 7u || stat == 9u || stat == 8u)
        return UpdateCharacterDisplay(window, actor, runtime);
    return static_cast<std::int32_t>(stat);
}

std::uint8_t CombatRoleStateWindow::RefreshCombatantColours(
    const std::uint32_t window, ICombatRoleStateDisplayRuntime& runtime) {
    for (std::uint32_t slot = 0u; slot < 3u; ++slot) {
        const auto actor = runtime.ActorAtRoleSlot(slot);
        if (!actor) continue;
        const auto portrait = runtime.ReadWord(window + 184u + 4u * slot);
        runtime.SetImageColours(portrait, kWhite);
        if (runtime.ActorCheckState3(actor)) {
            runtime.SetImageColours(portrait, kBlack);
            continue;
        }
        if (runtime.ActorStatusKind(actor) == 2) {
            runtime.SetImageColours(portrait,
                runtime.ReadByte(kBlinkByte) ? kBlinkBright : kBlinkDark);
            continue;
        }
        const auto element = runtime.ActorElement(actor);
        if (element >= 1 && element <= 5)
            runtime.SetImageColours(portrait,
                kElementColours[static_cast<std::size_t>(element - 1)]);
        for (std::uint32_t status = 0u; status < 10u; ++status) {
            const auto control = runtime.ReadWord(
                window + 404u + 40u * slot + 4u * status);
            const auto image = runtime.ReadWord(control + 1712u);
            for (std::uint32_t colour = 0u;
                 colour < kElementColours.size(); ++colour) {
                if (image == runtime.ReadWord(window + 316u + 4u * colour)) {
                    runtime.SetImageColours(portrait, kElementColours[colour]);
                    break;
                }
            }
        }
    }
    const auto old = runtime.ReadByte(kBlinkByte);
    runtime.WriteByte(kBlinkByte, old == 0u ? 1u : 0u);
    return old;
}

std::int32_t ICombatRoleStateWindowRuntime::PlayerRosterCount() {
    const auto manager=ActorManager(),begin=ReadWord(manager+36u),end=ReadWord(manager+40u);
    return begin ? (std::bit_cast<std::int32_t>(end-begin)>>2) : 0;
}
std::uint8_t ICombatRoleStateWindowRuntime::StartColourTimer(std::uint32_t window) {
    const auto name=CreateName("FlushRoleHead");const auto queue=ReadWord(CombatSystem()+412u);
    const auto callback=Allocate(12u);
    if(callback){WriteWord(callback,CombatRoleStateWindow::kTimerCallbackVtable);WriteWord(callback+4u,window);WriteWord(callback+8u,CombatRoleStateWindow::kRefreshCallback);}
    CombatRoleStateWindow::AddTimer(queue,name,CombatRoleStateWindow::kTimerPeriodBits,callback,*this);
    return CleanupName(name);
}
std::uint8_t ICombatRoleStateWindowRuntime::StopColourTimer(std::uint32_t) {
    const auto name=CreateName("FlushRoleHead");const auto queue=ReadWord(CombatSystem()+412u);
    RemoveTimerByName(queue,name);return CleanupName(name);
}
std::uint8_t CombatRoleStateWindow::SetVisibility(std::uint32_t window,bool visible,std::uint32_t parent,ICombatRoleStateVisibilityRuntime& runtime) {
    runtime.SetParent(window,visible,parent);
    if(visible)for(std::uint32_t index=0u;index<3u;++index)UpdateDisplay(window,static_cast<std::int32_t>(index),runtime);
    return visible?runtime.StartColourTimer(window):runtime.StopColourTimer(window);
}
std::uint8_t CombatRoleStateWindow::SetVisible(std::uint32_t window,bool visible,std::uint32_t parent,ICombatRoleStateWindowRuntime& runtime) {
    return SetVisibility(window,visible,parent,runtime);
}
} // namespace pal4
