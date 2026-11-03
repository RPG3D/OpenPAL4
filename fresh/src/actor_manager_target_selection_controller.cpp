#include "pal4/actor_manager_target_selection_controller.h"

#include <array>
#include <bit>
#include <cmath>
#include <initializer_list>
#include <limits>

namespace pal4 {
std::uint32_t IActorManagerTargetSelectionRuntime::StopInputSelection(std::uint32_t c,const std::array<std::uint32_t,4>& p){return ActorManagerTargetSelectionController::StopSelecting(c,p,*this);}
std::uint32_t IActorManagerTargetSelectionRuntime::FindInputClosest(std::uint32_t c){return ActorManagerTargetSelectionController::FindClosestActor(c,*this);}
std::uint32_t IActorManagerTargetSelectionRuntime::PreferredTipsIterator(){return PreferredIterator(ActorManagerSingleton());}
std::uint32_t IActorManagerTargetSelectionRuntime::FallbackTipsIterator(){return FallbackIterator(ActorManagerSingleton()+32u);}


namespace {

struct EventBinding {
    std::string_view name;
    std::uint32_t mode;
    std::uint32_t callback;
};

constexpr std::array<EventBinding, 5> kBindings{{
    {"start_select_target", 2u, 0x53bb80u},
    {"stop_select_target", 2u, 0x53bbd0u},
    {"close_system", 3u, 0x53bbd0u},
    {"combat_over", 3u, 0x53bbd0u},
    {"combat_prepare", 3u, 0x53bbd0u},
}};

}  // namespace

std::uint32_t IActorManagerTargetSelectionRuntime::CreateTipsIterator(bool player) {
    const auto manager=ActorManagerSingleton();
    return player?FallbackIterator(manager+32u):PreferredIterator(manager);
}
void IActorManagerTargetSelectionRuntime::ReportInvalidTargetType(std::uint32_t type) {
    if(type==0u || type==3u || type==4u) {
        const auto description=ReadWord(0x8ef458u+16u*type+4u);
        ReportTargetTypeError(true,description?description:0x841964u);
    } else ReportTargetTypeError(false,type);
}
void IActorManagerTargetSelectionRuntime::UnsubscribeEvent(
    std::uint32_t router,std::uint32_t key,std::uint32_t handle) {
        const auto group = FindEventGroup(router, key);
        if (group != 0u) {
            const auto head = ReadWord(group + 4u);
            for (auto node = ReadWord(head); node != head;
                 node = ReadWord(node)) {
                if (ReadWord(node + 12u) != handle) continue;
                const auto previous = ReadWord(node + 4u);
                const auto next = ReadWord(node);
                WriteWord(previous, next);
                WriteWord(next + 4u, previous);
                ReleaseOwnedSlot(node + 8u);
                FreeNode(node);
                WriteWord(group + 8u,
                                  ReadWord(group + 8u) - 1u);
                break;
            }
        }
}
std::uint32_t IActorManagerTargetSelectionRuntime::DetermineSelectionTargetMode(std::uint32_t controller) {
    return ActorManagerTargetSelectionController::DetermineTargetMode(controller,*this);
}
std::uint32_t IActorManagerTargetSelectionRuntime::RefreshTargetSelection(std::uint32_t controller) {
    return ActorManagerTargetSelectionController::RefreshSelection(controller,*this);
}

std::uint32_t ActorManagerTargetSelectionController::Construct(
    const std::uint32_t controller,
    IActorManagerTargetSelectionBoundaryRuntime& runtime) {
    for (std::uint32_t index = 0u; index < kBindings.size(); ++index) {
        runtime.WriteWord(controller + 8u * index, 0u);
        runtime.WriteWord(controller + 8u * index + 4u, 0u);
    }
    runtime.InitializeSelectionData(controller + 40u, 0u);
    for (const auto offset : {56u, 60u, 64u, 68u})
        runtime.WriteWord(controller + offset, 0u);
    runtime.WriteWord(controller + 72u, 2u);
    runtime.WriteWord(controller + 76u, 0u);
    for (const auto offset : {80u, 81u, 82u})
        runtime.WriteByte(controller + offset, 0u);
    static_cast<void>(SubscribeEvents(controller, runtime));
    return controller;
}

std::uint32_t ActorManagerTargetSelectionController::SubscribeEvents(
    const std::uint32_t controller,
    IActorManagerTargetSelectionBoundaryRuntime& runtime) {
    UnsubscribeEvents(controller, runtime);
    std::uint32_t result = 0u;
    for (std::uint32_t index = 0u; index < kBindings.size(); ++index) {
        const auto& binding = kBindings[index];
        const auto key = runtime.EventKey(binding.name);
        result = runtime.Subscribe(runtime.EventRouter(), binding.mode, key,
                                   controller, binding.callback);
        runtime.WriteWord(controller + 8u * index, result);
        runtime.WriteWord(controller + 8u * index + 4u, key);
    }
    return result;
}

void ActorManagerTargetSelectionController::UnsubscribeEvents(
    const std::uint32_t controller,
    IActorManagerTargetSelectionBoundaryRuntime& runtime) {
    for (std::uint32_t index = 0u; index < kBindings.size(); ++index) {
        const auto slot = controller + 8u * index;
        const auto handle = runtime.ReadWord(slot);
        if (handle == 0u) continue;

        const auto router = runtime.EventRouter();
        const auto key = runtime.ReadWord(slot + 4u);
        runtime.UnsubscribeEvent(router,key,handle);
        runtime.WriteWord(slot, 0u);
        runtime.WriteWord(slot + 4u, 0u);
    }
}

std::uint32_t ActorManagerTargetSelectionController::ResetTargetPair(
    const std::uint32_t controller,
    IActorManagerTargetSelectionRuntime& runtime) {
    runtime.WriteWord(controller + 56u, 0u);
    runtime.WriteWord(controller + 60u, 0u);
    return 0u;
}

std::uint32_t ActorManagerTargetSelectionController::ResetCommandStateFirst(
    const std::uint32_t controller,
    IActorManagerTargetSelectionRuntime& runtime) {
    runtime.WriteWord(controller + 44u, 0u);
    runtime.WriteWord(controller + 64u, 0u);
    runtime.WriteWord(controller + 68u, 0u);
    return 0u;
}

std::uint32_t ActorManagerTargetSelectionController::ResetCommandStateSecond(
    const std::uint32_t controller,
    IActorManagerTargetSelectionMemoryRuntime& runtime) {
    runtime.WriteWord(controller + 64u, 0u);
    runtime.WriteWord(controller + 44u, 0u);
    runtime.WriteWord(controller + 68u, 0u);
    return 0u;
}

std::uint32_t ActorManagerTargetSelectionController::ProcessInput(
    const std::uint32_t controller, const float delta,
    IActorManagerTargetInputRuntime& runtime) {
    const auto inactive = runtime.CombatInactive(runtime.CombatSystem());
    if (inactive != 0u) return inactive;
    const auto active = runtime.CombatActive(runtime.CombatSystem());
    if (static_cast<std::uint8_t>(active) == 0u) return active;

    const auto start_input = runtime.InputManager();
    const bool start_pressed =
        runtime.ReadWord(start_input + 0x21e4u) == 1u &&
        (runtime.ReadWord(start_input + 0x21d8u) & 0xfu) == 1u;
    if (start_pressed) {
        if (runtime.ReadWord(controller + 64u) == 0u) return 0u;
        static_cast<void>(runtime.DispatchAddress(
            runtime.EventRouter(), "re_start_ui_control",
            controller + 40u, 1u));
        std::array<std::uint32_t, 4> payload{};
        for (std::uint32_t index = 0u; index < payload.size(); ++index)
            payload[index] = runtime.ReadWord(controller + 40u + 4u * index);
        return runtime.StopInputSelection(controller, payload);
    }

    const auto select_input = runtime.InputManager();
    const bool select_pressed =
        runtime.ReadWord(select_input + 0x21d4u) == 1u &&
        (runtime.ReadWord(select_input + 0x21c8u) & 0xfu) == 1u;
    if (select_pressed) {
        const auto target = runtime.ReadWord(controller + 64u);
        if (target != 0u) {
            runtime.WriteWord(controller + 44u, target);
            std::array<std::uint32_t, 4> payload{};
            for (std::uint32_t index = 0u; index < payload.size(); ++index)
                payload[index] = runtime.ReadWord(controller + 40u + 4u * index);
            static_cast<void>(runtime.DispatchWords(
                runtime.EventRouter(), "re_select_command_target",
                payload, 1u));
        }
    }

    const auto target = runtime.ReadWord(controller + 64u);
    if (target != 0u) runtime.CheckTargetState(target);
    const auto old_time = std::bit_cast<float>(
        runtime.ReadWord(controller + 76u));
    const double elapsed = static_cast<double>(old_time) + delta;
    runtime.WriteWord(controller + 76u,
                      std::bit_cast<std::uint32_t>(
                          static_cast<float>(elapsed)));
    constexpr float kThreshold = 0.3f;
    if (std::isnan(elapsed)) return 0x4100u;
    if (elapsed < static_cast<double>(kThreshold)) return 0x100u;
    if (elapsed == static_cast<double>(kThreshold)) return 0x4000u;
    return runtime.FindInputClosest(controller);
}

std::uint32_t ActorManagerTargetSelectionController::StartSelecting(
    const std::uint32_t controller,
    const std::array<std::uint32_t, 4>& payload,
    IActorManagerTargetSelectionBoundaryRuntime& runtime) {
    runtime.WriteWord(controller + 40u, payload[0]);
    runtime.WriteWord(controller + 44u, payload[1]);
    runtime.WriteWord(controller + 48u, payload[2]);
    runtime.WriteByte(controller + 82u, 1u);
    runtime.WriteWord(controller + 52u, payload[3]);
    runtime.WriteWord(controller + 68u, 0u);
    runtime.WriteWord(controller + 72u,
                      runtime.DetermineSelectionTargetMode(controller));
    static_cast<void>(runtime.RefreshTargetSelection(controller));
    return runtime.InitializeInputState(runtime.InputManager());
}

std::uint32_t ActorManagerTargetSelectionController::StopSelecting(
    const std::uint32_t controller,
    const std::array<std::uint32_t, 4>& payload,
    IActorManagerTargetSelectionBoundaryRuntime& runtime) {
    static_cast<void>(payload);
    const auto combat = runtime.CombatSystem();
    if (runtime.ReadWord(combat + 508u) != 7u) {
        const auto target = runtime.ReadWord(controller + 64u);
        if (target != 0u)
            static_cast<void>(runtime.SetTargetSelected(target, 0u));
    }
    const auto result = runtime.DispatchSimple(
        runtime.EventRouter(), "hide_monster_tips", 0u, 1u);
    runtime.WriteByte(controller + 82u, 0u);
    runtime.WriteWord(controller + 44u, 0u);
    runtime.WriteWord(controller + 64u, 0u);
    runtime.WriteWord(controller + 68u, 0u);
    return result;
}

bool ActorManagerTargetSelectionController::CandidateAllowed(
    const std::uint32_t controller, const std::uint32_t actor,
    IActorManagerTargetCandidatesRuntime& runtime) {
    return runtime.ReadByte(controller + 80u) == 0u ||
           runtime.TargetStateNotThree(actor);
}

std::uint32_t ActorManagerTargetSelectionController::SwitchActiveActor(
    const std::uint32_t controller, const std::uint32_t actor,
    IActorManagerTargetCandidatesRuntime& runtime) {
    if (actor == 0u) return 0u;
    const auto previous = runtime.ReadWord(controller + 64u);
    if (previous == actor) return 0u;
    if (previous != 0u)
        static_cast<void>(runtime.SetTargetSelected(previous, 0u));
    runtime.WriteWord(controller + 64u, actor);
    return runtime.SetTargetSelected(actor, 1u);
}

namespace {

bool ScanCandidates(const std::uint32_t controller,
                    const std::uint32_t iterator,
                    const std::uint32_t config_first,
                    const std::uint32_t config_second,
                    IActorManagerTargetPointerRuntime& runtime) {
    if (iterator == 0u) return false;
    bool accepted = false;
    try {
        while (!runtime.IteratorAtEnd(iterator)) {
            const auto actor = runtime.IteratorValue(iterator);
            if (ActorManagerTargetSelectionController::CandidateAllowed(
                    controller, actor, runtime)) {
                const auto radius = runtime.ActorRadius(actor);
                const auto context = runtime.ReadWord(controller + 60u);
                if (runtime.ActorManagerHelper(context, config_first,
                                               config_second, radius)) {
                    accepted = true;
                    break;
                }
            }
            runtime.IteratorNext(iterator);
        }
    } catch (...) {
        static_cast<void>(runtime.DeleteIterator(iterator, 1u));
        throw;
    }
    static_cast<void>(runtime.DeleteIterator(iterator, 1u));
    return accepted;
}

}  // namespace

std::uint32_t ActorManagerTargetSelectionController::UpdateTarget(
    const std::uint32_t controller, const std::uint32_t argument,
    IActorManagerTargetPointerRuntime& runtime) {
    if (runtime.ReadByte(controller + 82u) == 0u) return 1u;

    const auto current_tips_actor = runtime.ReadWord(controller + 68u);
    const auto config_second = runtime.GameConfigWord(13u);
    const auto config_first = runtime.GameConfigWord(12u);
    if (current_tips_actor != 0u) {
        const auto radius = runtime.ActorRadius(current_tips_actor);
        const auto context = runtime.ReadWord(controller + 60u);
        if (!runtime.ActorManagerHelper(context, config_first,
                                        config_second, radius)) {
            static_cast<void>(runtime.DispatchSimple(
                runtime.EventRouter(), "hide_monster_tips", 0u, 1u));
            runtime.WriteWord(controller + 68u, 0u);
        }
    } else {
        const auto preferred = runtime.CreateTipsIterator(false);
        const auto preferred_found = ScanCandidates(
            controller, preferred, config_first, config_second, runtime);
        if (!preferred_found) {
            const auto fallback = runtime.CreateTipsIterator(true);
            const auto fallback_found = ScanCandidates(
                controller, fallback, config_first, config_second, runtime);
            if (!fallback_found) runtime.WriteWord(controller + 76u, 0u);
        }
    }

    if (runtime.ReadByte(controller + 82u) != 0u) {
        const auto actor = FindNearestCandidate(controller, argument, runtime);
        if (actor != 0u)
            static_cast<void>(SwitchActiveActor(controller, actor, runtime));
    }
    return 1u;
}

std::uint32_t ActorManagerTargetSelectionController::RefreshSelection(
    const std::uint32_t controller,
    IActorManagerTargetCandidatesRuntime& runtime) {
    const auto dialog = runtime.SelectionDialog(controller + 40u);
    const auto mode = runtime.ReadWord(controller + 72u);
    if (mode == 2u) return 2u;

    const auto temporary = runtime.CreateSelectionIterator(
        dialog, mode != 0u);
    std::uint32_t iterator = 0u;
    try {
        iterator = runtime.CloneSelectionIterator(temporary);
    } catch (...) {
        runtime.DeleteTemporaryIterator(temporary, 1u);
        throw;
    }
    runtime.DeleteTemporaryIterator(temporary, 1u);
    if (iterator == 0u) return 0u;

    try {
        if (runtime.IteratorHasData(iterator)) {
            while (!runtime.IteratorAtEnd(iterator)) {
                const auto actor = runtime.IteratorValue(iterator);
                if (CandidateAllowed(controller, actor, runtime)) {
                    static_cast<void>(SwitchActiveActor(
                        controller, actor, runtime));
                    break;
                }
                runtime.IteratorNext(iterator);
            }
        }
    } catch (...) {
        static_cast<void>(runtime.DeleteIterator(iterator, 1u));
        throw;
    }
    return runtime.DeleteIterator(iterator, 1u);
}

std::uint32_t ActorManagerTargetSelectionController::DetermineTargetMode(
    const std::uint32_t controller,
    IActorManagerTargetModeRuntime& runtime) {
    runtime.WriteByte(controller + 80u, 1u);
    const auto initial_dialog = runtime.SelectionDialog(controller + 40u);
    const auto command = runtime.ReadWord(controller + 52u);
    if (command == 4u) return 1u;

    SelectionRecordKind kind{};
    std::string_view error_kind;
    switch (command) {
    case 0u:
        kind = SelectionRecordKind::Magic;
        error_kind = "magic";
        break;
    case 1u:
        kind = SelectionRecordKind::Stunt;
        error_kind = "stunt";
        break;
    case 2u:
        kind = SelectionRecordKind::Property;
        error_kind = "property";
        break;
    default: {
        const auto name = runtime.ReadWord(initial_dialog + 816u);
        runtime.ReportSelectionError("command",
                                     name == 0u ? 0x841964u : name,
                                     command);
        return 2u;
    }
    }

    const auto dialog = runtime.SelectionDialog(controller + 40u);
    const auto database = runtime.GameDatabase();
    const auto key = runtime.ReadWord(controller + 48u);
    const auto record = runtime.FindSelectionRecord(
        kind == SelectionRecordKind::Property ? database : database + 12u,
        key, kind);
    if (record == 0u) {
        const auto name = runtime.ReadWord(dialog + 816u);
        runtime.ReportSelectionError(error_kind,
                                     name == 0u ? 0x841964u : name,
                                     key);
        return 2u;
    }

    const auto status_offset = kind == SelectionRecordKind::Stunt ? 4u : 8u;
    runtime.WriteByte(controller + 80u,
                      runtime.ReadWord(record + status_offset) == 5u ? 0u : 1u);
    return MapTargetType(runtime.RecordTargetType(record, kind), runtime);
}

std::uint32_t ActorManagerTargetSelectionController::MapTargetType(
    const std::uint32_t type,
    IActorManagerTargetModeRuntime& runtime) {
    if (type == 1u) return 0u;
    if (type == 2u) return 1u;
    runtime.ReportInvalidTargetType(type);
    return 2u;
}

std::uint32_t ActorManagerTargetSelectionController::FindNearestCandidate(
    const std::uint32_t controller, const std::uint32_t coordinate_pair,
    IActorManagerTargetPointerRuntime& runtime) {
    if (runtime.ReadWord(controller + 56u) == 0u ||
        runtime.ReadWord(controller + 60u) == 0u)
        return 0u;

    const auto dialog = runtime.SelectionDialog(controller + 40u);
    const auto mode = runtime.ReadWord(controller + 72u);
    const auto temporary = runtime.CreateSelectionIterator(
        dialog, mode != 0u);
    std::uint32_t iterator = 0u;
    try {
        iterator = runtime.CloneSelectionIterator(temporary);
    } catch (...) {
        runtime.DeleteTemporaryIterator(temporary, 1u);
        throw;
    }
    runtime.DeleteTemporaryIterator(temporary, 1u);
    if (iterator == 0u) return 0u;

    std::uint32_t nearest = 0u;
    float closest_distance = std::numeric_limits<float>::max();
    try {
        while (!runtime.IteratorAtEnd(iterator)) {
            const auto actor = runtime.IteratorValue(iterator);
            if (CandidateAllowed(controller, actor, runtime)) {
                const auto radius = runtime.ActorRadius(actor);
                const auto first = runtime.ReadWord(coordinate_pair);
                const auto second = runtime.ReadWord(coordinate_pair + 4u);
                const auto context = runtime.ReadWord(controller + 60u);
                const auto projected = runtime.ActorManagerHelper(
                    context, first, second, radius);
                if (projected != 0u) {
                    const auto reference = runtime.ReadWord(context);
                    const auto distance = runtime.DistanceSquared(
                        projected, reference);
                    if (distance < static_cast<double>(closest_distance)) {
                        closest_distance = static_cast<float>(distance);
                        nearest = actor;
                    }
                }
            }
            runtime.IteratorNext(iterator);
        }
    } catch (...) {
        static_cast<void>(runtime.DeleteIterator(iterator, 1u));
        throw;
    }
    static_cast<void>(runtime.DeleteIterator(iterator, 1u));
    return nearest;
}

std::uint32_t ActorManagerTargetSelectionController::CurrentActor(
    const std::uint32_t controller,
    IActorManagerTargetSelectionMemoryRuntime& runtime) {
    return runtime.ReadWord(controller + 64u);
}

std::uint32_t ActorManagerTargetSelectionController::SwitchActiveActorThunk(
    const std::uint32_t controller, const std::uint32_t actor,
    IActorManagerTargetSelectionRuntime& runtime) {
    return SwitchActiveActor(controller, actor, runtime);
}

std::uint32_t ActorManagerTargetSelectionController::DispatchSelectedTarget(
    const std::uint32_t controller,
    IActorManagerTargetControlRuntime& runtime) {
    const auto target = runtime.ReadWord(controller + 64u);
    if (target == 0u) return 0u;
    runtime.WriteWord(controller + 44u, target);
    std::array<std::uint32_t, 4> payload{};
    for (std::uint32_t index = 0u; index < payload.size(); ++index)
        payload[index] = runtime.ReadWord(controller + 40u + 4u * index);
    return runtime.DispatchWords(runtime.EventRouter(),
                                 "re_select_command_target", payload, 1u);
}

std::uint32_t ActorManagerTargetSelectionController::RestartUiControl(
    const std::uint32_t controller,
    IActorManagerTargetControlRuntime& runtime) {
    if (runtime.ReadWord(controller + 64u) == 0u) return 0u;
    static_cast<void>(runtime.DispatchAddress(
        runtime.EventRouter(), "re_start_ui_control", controller + 40u, 1u));
    return StopSelecting(controller, {}, runtime);
}

namespace {

std::uint32_t ScanNearestActors(
    const std::uint32_t controller, const std::uint32_t iterator,
    const std::uint32_t first, const std::uint32_t second,
    float& best_distance, IActorManagerTargetHoverRuntime& runtime) {
    if (iterator == 0u) return 0u;
    std::uint32_t nearest = 0u;
    while (!runtime.IteratorAtEnd(iterator)) {
        const auto actor = runtime.IteratorValue(iterator);
        if (ActorManagerTargetSelectionController::CandidateAllowed(
                controller, actor, runtime)) {
            const auto radius = runtime.ActorRadius(actor);
            const auto context = runtime.ReadWord(controller + 60u);
            const auto projected = runtime.ActorManagerHelper(
                context, first, second, radius);
            if (projected != 0u) {
                const auto reference = runtime.ReadWord(context);
                const auto distance = runtime.DistanceSquared(
                    projected, reference);
                if (distance < static_cast<double>(best_distance)) {
                    best_distance = static_cast<float>(distance);
                    nearest = actor;
                }
            }
        }
        runtime.IteratorNext(iterator);
    }
    return nearest;
}

}  // namespace

std::uint32_t ActorManagerTargetSelectionController::FindClosestActor(
    const std::uint32_t controller,
    IActorManagerTargetHoverRuntime& runtime) {
    float best_distance = std::numeric_limits<float>::max();
    runtime.WriteWord(controller + 76u, 0u);
    const auto first = runtime.GameConfigWord(12u);
    const auto second = runtime.GameConfigWord(13u);
    const auto preferred = runtime.PreferredTipsIterator();
    std::uint32_t fallback = 0u;
    std::uint32_t nearest = 0u;
    try {
        nearest = ScanNearestActors(controller, preferred, first, second,
                                    best_distance, runtime);
        if (nearest == 0u) {
            fallback = runtime.FallbackTipsIterator();
            nearest = ScanNearestActors(controller, fallback, first, second,
                                        best_distance, runtime);
        }
        if (nearest != 0u) {
            runtime.WriteWord(controller + 68u, nearest);
            static_cast<void>(runtime.DispatchActor(
                runtime.EventRouter(), "show_monster_tips", nearest, 1u));
        }
    } catch (...) {
        if (fallback != 0u)
            static_cast<void>(runtime.DeleteIterator(fallback, 1u));
        if (preferred != 0u)
            static_cast<void>(runtime.DeleteIterator(preferred, 1u));
        throw;
    }
    if (fallback != 0u)
        static_cast<void>(runtime.DeleteIterator(fallback, 1u));
    if (preferred != 0u)
        return runtime.DeleteIterator(preferred, 1u);
    return 0u;
}

}  // namespace pal4
