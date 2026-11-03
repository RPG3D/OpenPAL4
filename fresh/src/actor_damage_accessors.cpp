#include "pal4/actor_damage_accessors.h"

#include <bit>

namespace pal4 {
namespace {

std::uint32_t GetWord(std::uint32_t actor, std::uint32_t index,
                      std::uint32_t base, std::uint32_t maximum,
                      IActorDamageAccessorRuntime& runtime) {
    if (index > maximum) return index;
    return runtime.ReadWord(actor + base + 4u * index);
}

double GetFloat(std::uint32_t actor, std::int32_t index,
                std::uint32_t base, std::uint32_t maximum,
                IActorDamageAccessorRuntime& runtime) {
    if (static_cast<std::uint32_t>(index) > maximum)
        return static_cast<double>(index);
    return static_cast<double>(std::bit_cast<float>(
        runtime.ReadWord(actor + base + 4u *
                         static_cast<std::uint32_t>(index))));
}

double AddDirectFloat(std::uint32_t actor, std::int32_t index,
                      float delta, std::uint32_t base,
                      std::uint32_t maximum,
                      IActorDamageAccessorRuntime& runtime) {
    if (static_cast<std::uint32_t>(index) > maximum)
        return static_cast<double>(index);
    const auto address = actor + base + 4u *
        static_cast<std::uint32_t>(index);
    const auto old_value = std::bit_cast<float>(runtime.ReadWord(address));
    const auto sum = static_cast<double>(old_value) +
                     static_cast<double>(delta);
    runtime.WriteWord(address, std::bit_cast<std::uint32_t>(
                                   static_cast<float>(sum)));
    return sum;
}

std::int32_t AddWord(std::uint32_t actor, std::uint32_t index,
                     std::int32_t delta, std::uint32_t get_offset,
                     std::uint32_t set_offset,
                     IActorDamageAccessorRuntime& runtime) {
    const auto vtable = runtime.ReadWord(actor);
    const auto current = runtime.CallIntGet(
        actor, vtable, get_offset, index);
    const auto sum = static_cast<std::int32_t>(
        static_cast<std::uint32_t>(current) +
        static_cast<std::uint32_t>(delta));
    return runtime.CallIntSet(actor, vtable, set_offset, index, sum);
}

std::int32_t AddFloat(std::uint32_t actor, std::uint32_t index,
                      float delta, std::uint32_t get_offset,
                      std::uint32_t set_offset,
                      IActorDamageAccessorRuntime& runtime) {
    const auto vtable = runtime.ReadWord(actor);
    const auto current = runtime.CallFloatGet(
        actor, vtable, get_offset, index);
    const auto sum = static_cast<float>(current +
                                        static_cast<double>(delta));
    return runtime.CallFloatSet(actor, vtable, set_offset, index,
                                std::bit_cast<std::uint32_t>(sum));
}

}  // namespace

std::uint32_t ActorDamageAccessors::ConstructCountCallback(
    std::uint32_t object, IActorDamageConstructionRuntime& runtime) {
    runtime.WriteByte(object + 4u, 0u);
    runtime.WriteWord(object + 8u, 0u);
    runtime.WriteWord(object, 0x846130u);
    return object;
}

std::uint32_t ActorDamageAccessors::GetPropertyCount() {
    return 9u;
}

std::uint32_t ActorDamageAccessors::GetActorCount672(
    std::uint32_t actor, IActorDamageAccessorRuntime& runtime) {
    return runtime.ReadWord(actor + 672u);
}

std::uint32_t ActorDamageAccessors::GetActorCount668(
    std::uint32_t actor, IActorDamageSlotRuntime& runtime) {
    return runtime.ReadWord(actor + 668u);
}

std::uint32_t ActorDamageAccessors::SetActorCount672(
    std::uint32_t actor, std::uint32_t value,
    IActorDamageConstructionRuntime& runtime) {
    runtime.WriteWord(actor + 672u, value);
    return value;
}

std::uint32_t ActorDamageAccessors::GetCountState632(
    std::uint32_t actor, IActorDamageAccessorRuntime& runtime) {
    return runtime.ReadWord(actor + 632u);
}
std::uint32_t ActorDamageAccessors::GetCountState636(
    std::uint32_t actor, IActorDamageAccessorRuntime& runtime) {
    return runtime.ReadWord(actor + 636u);
}
std::uint32_t ActorDamageAccessors::GetCountState640(
    std::uint32_t actor, IActorDamageAccessorRuntime& runtime) {
    return runtime.ReadWord(actor + 640u);
}
std::uint32_t ActorDamageAccessors::GetCountState644(
    std::uint32_t actor, IActorDamageAccessorRuntime& runtime) {
    return runtime.ReadWord(actor + 644u);
}
std::uint32_t ActorDamageAccessors::GetCountState648(
    std::uint32_t actor, IActorDamageAccessorRuntime& runtime) {
    return runtime.ReadWord(actor + 648u);
}
std::uint32_t ActorDamageAccessors::GetCountState652(
    std::uint32_t actor, IActorDamageAccessorRuntime& runtime) {
    return runtime.ReadWord(actor + 652u);
}
std::uint32_t ActorDamageAccessors::GetCountState656(
    std::uint32_t actor, IActorDamageAccessorRuntime& runtime) {
    return runtime.ReadWord(actor + 656u);
}
std::uint32_t ActorDamageAccessors::GetCountState660(
    std::uint32_t actor, IActorDamageAccessorRuntime& runtime) {
    return runtime.ReadWord(actor + 660u);
}
std::uint32_t ActorDamageAccessors::GetCountState664(
    std::uint32_t actor, IActorDamageAccessorRuntime& runtime) {
    return runtime.ReadWord(actor + 664u);
}

double ActorDamageAccessors::AddDirectFloat484(
    std::uint32_t actor, std::int32_t index, float delta,
    IActorDamageAccessorRuntime& runtime) {
    return AddDirectFloat(actor, index, delta, 484u, 5u, runtime);
}
double ActorDamageAccessors::AddDirectFloat508(
    std::uint32_t actor, std::int32_t index, float delta,
    IActorDamageAccessorRuntime& runtime) {
    return AddDirectFloat(actor, index, delta, 508u, 6u, runtime);
}
double ActorDamageAccessors::AddDirectFloat536(
    std::uint32_t actor, std::int32_t index, float delta,
    IActorDamageAccessorRuntime& runtime) {
    return AddDirectFloat(actor, index, delta, 536u, 1u, runtime);
}

std::uint32_t ActorDamageAccessors::Construct(
    std::uint32_t object, IActorDamageConstructionRuntime& runtime) {
    runtime.WriteByte(object + 4u, 0u);
    runtime.WriteWord(object + 8u, 0u);
    runtime.WriteWord(object, 0x845e70u);
    return object;
}

std::uint32_t ActorDamageAccessors::GetWord344(
    std::uint32_t actor, std::uint32_t index,
    IActorDamageAccessorRuntime& runtime) {
    return GetWord(actor, index, 344u, 7u, runtime);
}
std::int32_t ActorDamageAccessors::AddWord48_64(
    std::uint32_t actor, std::uint32_t index, std::int32_t delta,
    IActorDamageAccessorRuntime& runtime) {
    return AddWord(actor, index, delta, 48u, 64u, runtime);
}
double ActorDamageAccessors::GetFloat376(
    std::uint32_t actor, std::int32_t index,
    IActorDamageAccessorRuntime& runtime) {
    return GetFloat(actor, index, 376u, 3u, runtime);
}
std::int32_t ActorDamageAccessors::AddFloat44_60(
    std::uint32_t actor, std::uint32_t index, float delta,
    IActorDamageAccessorRuntime& runtime) {
    return AddFloat(actor, index, delta, 44u, 60u, runtime);
}
std::uint32_t ActorDamageAccessors::GetWord464(
    std::uint32_t actor, std::uint32_t index,
    IActorDamageAccessorRuntime& runtime) {
    return GetWord(actor, index, 464u, 4u, runtime);
}
std::int32_t ActorDamageAccessors::AddWord28_32(
    std::uint32_t actor, std::uint32_t index, std::int32_t delta,
    IActorDamageAccessorRuntime& runtime) {
    return AddWord(actor, index, delta, 28u, 32u, runtime);
}
std::uint32_t ActorDamageAccessors::GetWord392(
    std::uint32_t actor, std::uint32_t index,
    IActorDamageAccessorRuntime& runtime) {
    return GetWord(actor, index, 392u, 5u, runtime);
}
std::int32_t ActorDamageAccessors::AddWord88_92(
    std::uint32_t actor, std::uint32_t index, std::int32_t delta,
    IActorDamageAccessorRuntime& runtime) {
    return AddWord(actor, index, delta, 88u, 92u, runtime);
}
double ActorDamageAccessors::GetFloat416(
    std::uint32_t actor, std::int32_t index,
    IActorDamageAccessorRuntime& runtime) {
    return GetFloat(actor, index, 416u, 5u, runtime);
}
std::int32_t ActorDamageAccessors::AddFloat100_104(
    std::uint32_t actor, std::uint32_t index, float delta,
    IActorDamageAccessorRuntime& runtime) {
    return AddFloat(actor, index, delta, 100u, 104u, runtime);
}
double ActorDamageAccessors::GetFloat440(
    std::uint32_t actor, std::int32_t index,
    IActorDamageAccessorRuntime& runtime) {
    return GetFloat(actor, index, 440u, 5u, runtime);
}
std::int32_t ActorDamageAccessors::AddFloat112_116(
    std::uint32_t actor, std::uint32_t index, float delta,
    IActorDamageAccessorRuntime& runtime) {
    return AddFloat(actor, index, delta, 112u, 116u, runtime);
}
std::uint32_t ActorDamageAccessors::GetWord544(
    std::uint32_t actor, std::uint32_t index,
    IActorDamageAccessorRuntime& runtime) {
    return GetWord(actor, index, 544u, 9u, runtime);
}
std::int32_t ActorDamageAccessors::AddWord52_68(
    std::uint32_t actor, std::uint32_t index, std::int32_t delta,
    IActorDamageAccessorRuntime& runtime) {
    return AddWord(actor, index, delta, 52u, 68u, runtime);
}
double ActorDamageAccessors::GetFloat584(
    std::uint32_t actor, std::int32_t index,
    IActorDamageAccessorRuntime& runtime) {
    return GetFloat(actor, index, 584u, 4u, runtime);
}
std::int32_t ActorDamageAccessors::AddFloat40_56(
    std::uint32_t actor, std::uint32_t index, float delta,
    IActorDamageAccessorRuntime& runtime) {
    return AddFloat(actor, index, delta, 40u, 56u, runtime);
}
double ActorDamageAccessors::GetFloat604(
    std::uint32_t actor, std::int32_t index,
    IActorDamageAccessorRuntime& runtime) {
    return GetFloat(actor, index, 604u, 6u, runtime);
}
std::int32_t ActorDamageAccessors::AddFloat124_128(
    std::uint32_t actor, std::uint32_t index, float delta,
    IActorDamageAccessorRuntime& runtime) {
    return AddFloat(actor, index, delta, 124u, 128u, runtime);
}

std::uint32_t ActorDamageAccessors::SetConditional672(
    std::uint32_t actor, std::uint32_t value,
    IActorDamageAccessorRuntime& runtime) {
    if (static_cast<std::int32_t>(runtime.ReadWord(actor + 1144u)) <= 0)
        runtime.WriteWord(actor + 672u, value);
    return actor;
}

std::uint32_t ActorDamageAccessors::SetMirrored332_668(
    std::uint32_t actor, std::uint32_t value,
    IActorDamageSlotRuntime& runtime) {
    runtime.WriteWord(actor + 332u, value);
    runtime.WriteWord(actor + 668u, value);
    return value;
}

}  // namespace pal4
