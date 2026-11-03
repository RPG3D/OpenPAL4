#include "pal4/actor_damage_event_setters.h"

#include <bit>

namespace pal4 {
namespace {

void EmitChange(std::uint32_t actor, std::uint32_t index,
                std::uint32_t previous_bits, std::uint32_t key_address,
                IActorDamageEventRuntime& runtime) {
    const auto record = runtime.TemporaryRecord();
    (void)ActorFacetEventRecord::ConstructPair(
        record, actor, index, previous_bits, runtime);
    const auto key = runtime.ReadWord(key_address);
    runtime.DispatchEvent(runtime.EventRouter(), key, record, 1u);
}

std::int32_t SetWord(std::uint32_t actor, std::uint32_t index,
                     std::int32_t value, std::uint32_t maximum,
                     std::uint32_t base, std::uint32_t getter_offset,
                     std::uint32_t key_address,
                     IActorDamageEventRuntime& runtime) {
    if (index > maximum) return static_cast<std::int32_t>(index);
    const auto previous = runtime.ActorIntVirtual(
        actor, getter_offset, index);
    if (previous == value) return value;
    runtime.WriteWord(actor + base + 4u * index,
                      static_cast<std::uint32_t>(value));
    EmitChange(actor, index, static_cast<std::uint32_t>(previous),
               key_address, runtime);
    return runtime.ActorIntVirtual(actor, getter_offset, index);
}

double SetFloat(std::uint32_t actor, std::int32_t index,
                float value, std::uint32_t maximum,
                std::uint32_t base, std::uint32_t getter_offset,
                std::uint32_t result_offset, std::uint32_t key_address,
                bool integer_previous,
                IActorDamageEventRuntime& runtime) {
    if (static_cast<std::uint32_t>(index) > maximum)
        return static_cast<double>(index);
    const auto slot = static_cast<std::uint32_t>(index);
    const auto previous = integer_previous
        ? static_cast<float>(runtime.ActorIntVirtual(
              actor, getter_offset, slot))
        : static_cast<float>(runtime.ActorFloatVirtual(
              actor, getter_offset, slot));
    if (previous == value) return static_cast<double>(value);
    runtime.WriteWord(actor + base + 4u * slot,
                      std::bit_cast<std::uint32_t>(value));
    EmitChange(actor, slot, std::bit_cast<std::uint32_t>(previous),
               key_address, runtime);
    return runtime.ActorFloatVirtual(actor, result_offset, slot);
}

std::int32_t SetState(std::uint32_t actor, std::uint32_t value,
                      std::uint32_t state_index, std::uint32_t maximum,
                      IActorDamageEventRuntime& runtime,
                      std::uint32_t key_address = 0x8b5fc8u) {
    const auto getter_offset = 136u + 8u * state_index;
    const auto previous = runtime.ActorStateVirtual(actor, getter_offset);
    if (static_cast<std::uint32_t>(previous) != value) {
        if (value <= maximum)
            runtime.WriteWord(actor + 632u + 4u * state_index, value);
        EmitChange(actor, state_index,
                   static_cast<std::uint32_t>(previous), key_address,
                   runtime);
        if (state_index == 4u && previous == 3)
            runtime.ActorVoidVirtual(actor, 548u);
    }
    return runtime.ActorStateVirtual(actor, getter_offset);
}

double GetFloat(std::uint32_t actor, std::int32_t index,
                std::uint32_t maximum, std::uint32_t base,
                IActorDamageEventRuntime& runtime) {
    if (static_cast<std::uint32_t>(index) > maximum)
        return static_cast<double>(index);
    return static_cast<double>(std::bit_cast<float>(runtime.ReadWord(
        actor + base + 4u * static_cast<std::uint32_t>(index))));
}

std::int32_t SetExtendedWordWithKey(
    std::uint32_t actor, std::int32_t index,
    std::int32_t value, std::uint32_t key_address,
    IActorDamageEventRuntime& runtime) {
    if (static_cast<std::uint32_t>(index) >= 10u) return index;
    const auto slot = static_cast<std::uint32_t>(index);
    const auto previous = runtime.ActorIntVirtual(actor, 52u, slot);
    if (previous != value) {
        if (slot == 4u)
            runtime.WriteWord(actor + 224u,
                              static_cast<std::uint32_t>(value));
        runtime.SetValidatedProperty(actor + 340u, slot, value);
        if (slot == 7u) runtime.UpdateHealthStatus(actor);
        const auto record = runtime.TemporaryRecord();
        (void)ActorFacetEventRecord::ConstructFourWords(
            record, actor, slot, static_cast<std::uint32_t>(previous),
            static_cast<std::uint32_t>(value), runtime);
        const auto key = runtime.ReadWord(key_address);
        runtime.DispatchEvent(runtime.EventRouter(), key, record, 1u);
    }
    if (slot == 7u) runtime.UpdateHealthStatus(actor);
    return runtime.ActorIntVirtual(actor, 52u, slot);
}

}  // namespace

std::int32_t ActorDamageEventSetters::SetWord344(
    std::uint32_t actor, std::uint32_t index, std::int32_t value,
    IActorDamageEventRuntime& runtime) {
    return SetWord(actor, index, value, 7u, 344u, 48u,
                   0x8b5fa4u, runtime);
}
double ActorDamageEventSetters::SetFloat376(
    std::uint32_t actor, std::int32_t index, float value,
    IActorDamageEventRuntime& runtime) {
    return SetFloat(actor, index, value, 3u, 376u, 44u, 44u,
                    0x8b5fa8u, false, runtime);
}
std::int32_t ActorDamageEventSetters::SetWord464(
    std::uint32_t actor, std::uint32_t index, std::int32_t value,
    IActorDamageEventRuntime& runtime) {
    return SetWord(actor, index, value, 4u, 464u, 28u,
                   0x8b5fb8u, runtime);
}
std::int32_t ActorDamageEventSetters::SetWord392(
    std::uint32_t actor, std::uint32_t index, std::int32_t value,
    IActorDamageEventRuntime& runtime) {
    return SetWord(actor, index, value, 5u, 392u, 88u,
                   0x8b5facu, runtime);
}
double ActorDamageEventSetters::SetFloat416FromInt(
    std::uint32_t actor, std::int32_t index, float value,
    IActorDamageEventRuntime& runtime) {
    return SetFloat(actor, index, value, 5u, 416u, 88u, 100u,
                    0x8b5fb0u, true, runtime);
}
double ActorDamageEventSetters::SetFloat440(
    std::uint32_t actor, std::int32_t index, float value,
    IActorDamageEventRuntime& runtime) {
    return SetFloat(actor, index, value, 5u, 440u, 112u, 112u,
                    0x8b5fb4u, false, runtime);
}
std::int32_t ActorDamageEventSetters::SetState632(
    std::uint32_t actor, std::uint32_t value,
    IActorDamageEventRuntime& runtime) {
    return SetState(actor, value, 0u, 5u, runtime);
}

std::int32_t ActorDamageEventSetters::SetState636(
    std::uint32_t actor, std::uint32_t value,
    IActorDamageEventRuntime& runtime) {
    return SetState(actor, value, 1u, 6u, runtime);
}
std::int32_t ActorDamageEventSetters::SetState640(
    std::uint32_t actor, std::uint32_t value,
    IActorDamageEventRuntime& runtime) {
    return SetState(actor, value, 2u, 6u, runtime);
}
std::int32_t ActorDamageEventSetters::SetState644(
    std::uint32_t actor, std::uint32_t value,
    IActorDamageEventRuntime& runtime) {
    return SetState(actor, value, 3u, 1u, runtime);
}
std::int32_t ActorDamageEventSetters::SetState648(
    std::uint32_t actor, std::uint32_t value,
    IActorDamageEventRuntime& runtime) {
    return SetState(actor, value, 4u, 3u, runtime);
}
std::int32_t ActorDamageEventSetters::SetState652(
    std::uint32_t actor, std::uint32_t value,
    IActorDamageEventRuntime& runtime) {
    return SetState(actor, value, 5u, 1u, runtime);
}
std::int32_t ActorDamageEventSetters::SetState656(
    std::uint32_t actor, std::uint32_t value,
    IActorDamageEventRuntime& runtime) {
    return SetState(actor, value, 6u, 4u, runtime);
}
std::int32_t ActorDamageEventSetters::SetState660(
    std::uint32_t actor, std::uint32_t value,
    IActorDamageEventRuntime& runtime) {
    return SetState(actor, value, 7u, 2u, runtime);
}
std::int32_t ActorDamageEventSetters::SetState664(
    std::uint32_t actor, std::uint32_t value,
    IActorDamageEventRuntime& runtime) {
    return SetState(actor, value, 8u, 5u, runtime);
}

std::int32_t ActorDamageEventSetters::SetExtendedWord(
    std::uint32_t actor, std::int32_t index, std::int32_t value,
    IActorDamageEventRuntime& runtime) {
    return SetExtendedWordWithKey(actor, index, value, 0x8b5fbcu,
                                  runtime);
}

double ActorDamageEventSetters::SetFloat584(
    std::uint32_t actor, std::int32_t index, float value,
    IActorDamageEventRuntime& runtime) {
    return SetFloat(actor, index, value, 4u, 584u, 40u, 40u,
                    0x8b5fc0u, false, runtime);
}
double ActorDamageEventSetters::SetFloat604(
    std::uint32_t actor, std::int32_t index, float value,
    IActorDamageEventRuntime& runtime) {
    return SetFloat(actor, index, value, 6u, 604u, 124u, 124u,
                    0x8b5fc4u, false, runtime);
}
double ActorDamageEventSetters::GetFloat484(
    std::uint32_t actor, std::int32_t index,
    IActorDamageEventRuntime& runtime) {
    return GetFloat(actor, index, 5u, 484u, runtime);
}
double ActorDamageEventSetters::SetFloat484(
    std::uint32_t actor, std::int32_t index, float value,
    IActorDamageEventRuntime& runtime) {
    return SetFloat(actor, index, value, 5u, 484u, 216u, 216u,
                    0x8b5fccu, false, runtime);
}
double ActorDamageEventSetters::GetFloat508(
    std::uint32_t actor, std::int32_t index,
    IActorDamageEventRuntime& runtime) {
    return GetFloat(actor, index, 6u, 508u, runtime);
}
double ActorDamageEventSetters::SetFloat508(
    std::uint32_t actor, std::int32_t index, float value,
    IActorDamageEventRuntime& runtime) {
    return SetFloat(actor, index, value, 6u, 508u, 212u, 212u,
                    0x8b5fd0u, false, runtime);
}
double ActorDamageEventSetters::GetFloat536(
    std::uint32_t actor, std::int32_t index,
    IActorDamageEventRuntime& runtime) {
    return GetFloat(actor, index, 1u, 536u, runtime);
}
double ActorDamageEventSetters::SetFloat536(
    std::uint32_t actor, std::int32_t index, float value,
    IActorDamageEventRuntime& runtime) {
    return SetFloat(actor, index, value, 1u, 536u, 208u, 208u,
                    0x8b5fd4u, false, runtime);
}

std::int32_t ActorDamageEventSetters::SetCountWord344(
    std::uint32_t actor, std::uint32_t index, std::int32_t value,
    IActorDamageEventRuntime& runtime) {
    return SetWord(actor, index, value, 7u, 344u, 48u,
                   0x8b62f8u, runtime);
}
double ActorDamageEventSetters::SetCountFloat376(
    std::uint32_t actor, std::int32_t index, float value,
    IActorDamageEventRuntime& runtime) {
    return SetFloat(actor, index, value, 3u, 376u, 44u, 44u,
                    0x8b62fcu, false, runtime);
}
std::int32_t ActorDamageEventSetters::SetCountWord464(
    std::uint32_t actor, std::uint32_t index, std::int32_t value,
    IActorDamageEventRuntime& runtime) {
    return SetWord(actor, index, value, 4u, 464u, 28u,
                   0x8b630cu, runtime);
}
std::int32_t ActorDamageEventSetters::SetCountWord392(
    std::uint32_t actor, std::uint32_t index, std::int32_t value,
    IActorDamageEventRuntime& runtime) {
    return SetWord(actor, index, value, 5u, 392u, 88u,
                   0x8b6300u, runtime);
}
double ActorDamageEventSetters::SetCountFloat416FromInt(
    std::uint32_t actor, std::int32_t index, float value,
    IActorDamageEventRuntime& runtime) {
    return SetFloat(actor, index, value, 5u, 416u, 88u, 100u,
                    0x8b6304u, true, runtime);
}
double ActorDamageEventSetters::SetCountFloat440(
    std::uint32_t actor, std::int32_t index, float value,
    IActorDamageEventRuntime& runtime) {
    return SetFloat(actor, index, value, 5u, 440u, 112u, 112u,
                    0x8b6308u, false, runtime);
}

std::int32_t ActorDamageEventSetters::SetCountState632(
    std::uint32_t actor, std::uint32_t value,
    IActorDamageEventRuntime& runtime) {
    return SetState(actor, value, 0u, 5u, runtime, 0x8b631cu);
}
std::int32_t ActorDamageEventSetters::SetCountState636(
    std::uint32_t actor, std::uint32_t value,
    IActorDamageEventRuntime& runtime) {
    return SetState(actor, value, 1u, 6u, runtime, 0x8b631cu);
}
std::int32_t ActorDamageEventSetters::SetCountState640(
    std::uint32_t actor, std::uint32_t value,
    IActorDamageEventRuntime& runtime) {
    return SetState(actor, value, 2u, 6u, runtime, 0x8b631cu);
}
std::int32_t ActorDamageEventSetters::SetCountState644(
    std::uint32_t actor, std::uint32_t value,
    IActorDamageEventRuntime& runtime) {
    return SetState(actor, value, 3u, 1u, runtime, 0x8b631cu);
}
std::int32_t ActorDamageEventSetters::SetCountState648(
    std::uint32_t actor, std::uint32_t value,
    IActorDamageEventRuntime& runtime) {
    return SetState(actor, value, 4u, 3u, runtime, 0x8b631cu);
}
std::int32_t ActorDamageEventSetters::SetCountState652(
    std::uint32_t actor, std::uint32_t value,
    IActorDamageEventRuntime& runtime) {
    return SetState(actor, value, 5u, 1u, runtime, 0x8b631cu);
}
std::int32_t ActorDamageEventSetters::SetCountState656(
    std::uint32_t actor, std::uint32_t value,
    IActorDamageEventRuntime& runtime) {
    return SetState(actor, value, 6u, 4u, runtime, 0x8b631cu);
}
std::int32_t ActorDamageEventSetters::SetCountState660(
    std::uint32_t actor, std::uint32_t value,
    IActorDamageEventRuntime& runtime) {
    return SetState(actor, value, 7u, 2u, runtime, 0x8b631cu);
}
std::int32_t ActorDamageEventSetters::SetCountState664(
    std::uint32_t actor, std::uint32_t value,
    IActorDamageEventRuntime& runtime) {
    return SetState(actor, value, 8u, 5u, runtime, 0x8b631cu);
}
std::int32_t ActorDamageEventSetters::SetCountExtendedWord(
    std::uint32_t actor, std::int32_t index, std::int32_t value,
    IActorDamageEventRuntime& runtime) {
    return SetExtendedWordWithKey(actor, index, value, 0x8b6310u,
                                  runtime);
}

double ActorDamageEventSetters::SetCountFloat584(
    std::uint32_t actor, std::int32_t index, float value,
    IActorDamageEventRuntime& runtime) {
    return SetFloat(actor, index, value, 4u, 584u, 40u, 40u,
                    0x8b6314u, false, runtime);
}
double ActorDamageEventSetters::SetCountFloat604(
    std::uint32_t actor, std::int32_t index, float value,
    IActorDamageEventRuntime& runtime) {
    return SetFloat(actor, index, value, 6u, 604u, 124u, 124u,
                    0x8b6318u, false, runtime);
}
double ActorDamageEventSetters::SetCountFloat484(
    std::uint32_t actor, std::int32_t index, float value,
    IActorDamageEventRuntime& runtime) {
    return SetFloat(actor, index, value, 5u, 484u, 216u, 216u,
                    0x8b6320u, false, runtime);
}
double ActorDamageEventSetters::SetCountFloat508(
    std::uint32_t actor, std::int32_t index, float value,
    IActorDamageEventRuntime& runtime) {
    return SetFloat(actor, index, value, 6u, 508u, 212u, 212u,
                    0x8b6324u, false, runtime);
}
double ActorDamageEventSetters::SetCountFloat536(
    std::uint32_t actor, std::int32_t index, float value,
    IActorDamageEventRuntime& runtime) {
    return SetFloat(actor, index, value, 1u, 536u, 208u, 208u,
                    0x8b6328u, false, runtime);
}

}  // namespace pal4
