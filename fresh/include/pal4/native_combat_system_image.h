#pragma once

#include "pal4/native_combat_translator.h"
#include "pal4/native_combat_world_resources.h"
#include "pal4/native_combat_target_selection.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <span>

namespace pal4 {
// Stable storage for the original520-byte CombatSystem. Existing native owners
// construct world/translator and publish borrowed manager lifecycle handles.
// Reserved ranges are not constructed managers, VC6 strings or containers;
// explicit native owners may publish borrowed handles into member slots.
// This image alone is not a running/initialized CombatSystem; in particular it
// must not set cold-init byte517 or be installed as a state dispatcher.
// Declare before all borrowers. Never copy/relocate after subobject binding.
struct NativeCombatSystemImage final {
    NativeCombatSystemImage() = default;
    NativeCombatSystemImage(const NativeCombatSystemImage&) = delete;
    NativeCombatSystemImage& operator=(const NativeCombatSystemImage&) = delete;
    NativeCombatSystemImage(NativeCombatSystemImage&&) = delete;
    NativeCombatSystemImage& operator=(NativeCombatSystemImage&&) = delete;

    NativeCombatTargetSelectionImage target_selection;
    std::array<std::uint32_t,8> world_event_storage{};
    NativeCombatWorldImage world;
    std::array<std::uint32_t,8> world_text_storage{};
    NativeCombatTranslatorImage translator;
    std::array<std::uint32_t,31> session_storage{};
    // Borrowed handle owned by NativeCombatGameEventHandler, not a VC6 object.
    std::uint32_t& GameEventHandlerSlot() noexcept { return session_storage[5]; }
    std::uint32_t& EventTableSlot() noexcept { return session_storage[1]; }
    std::uint32_t& SystemManagerSlot() noexcept { return session_storage[0]; }
    std::uint32_t& ActorHandlersSlot() noexcept { return session_storage[2]; }
    std::uint32_t& ExtendedHandlersSlot() noexcept { return session_storage[3]; }
    std::uint32_t& TimedEventsSlot() noexcept { return session_storage[4]; }
    std::uint32_t& EndSoundSlot() noexcept { return session_storage[6]; }
    std::span<std::uint32_t,18> TailStorage() noexcept { return std::span<std::uint32_t,18>(session_storage.data()+9u,18u); }
    std::uint32_t& ActorManagerSlot() noexcept { return session_storage[7]; }
    // IDA528700/575460: scheduler state and inactive-mode byte, not cold517.
    std::uint32_t& State() noexcept { return session_storage[28]; }
    std::uint8_t& Mode() noexcept { return reinterpret_cast<std::uint8_t*>(session_storage.data())[120]; }
};
static_assert(std::is_standard_layout_v<NativeCombatSystemImage>);
static_assert(offsetof(NativeCombatSystemImage,target_selection)==0u);
static_assert(offsetof(NativeCombatSystemImage,world_event_storage)==84u);
static_assert(offsetof(NativeCombatSystemImage,world)==116u);
static_assert(offsetof(NativeCombatSystemImage,world_text_storage)==348u);
static_assert(offsetof(NativeCombatSystemImage,translator)==380u);
static_assert(offsetof(NativeCombatSystemImage,session_storage)==396u);
static_assert(offsetof(NativeCombatSystemImage,session_storage)+sizeof(std::uint32_t)==400u);
static_assert(offsetof(NativeCombatSystemImage,session_storage)+5u*sizeof(std::uint32_t)==416u);
static_assert(offsetof(NativeCombatSystemImage,session_storage)+7u*sizeof(std::uint32_t)==424u);
static_assert(sizeof(NativeCombatSystemImage)==520u);
}
