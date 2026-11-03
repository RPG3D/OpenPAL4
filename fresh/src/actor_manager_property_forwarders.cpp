#include "pal4/actor_manager_property_forwarders.h"

#include <algorithm>
#include <stdexcept>

namespace pal4 {
namespace {

// Each IDA entry here is a direct virtual forward with the listed slot and arity.
constexpr std::array<ActorPropertyForwardSpec, 39> kSpecs{{
    {0x52f220u, 112u, 0u}, {0x52f230u, 116u, 1u},
    {0x52f250u, 120u, 0u}, {0x52f260u, 124u, 1u},
    {0x52f2b0u, 148u, 1u}, {0x52f2d0u, 180u, 2u},
    {0x52f2f0u, 164u, 2u}, {0x52f310u, 144u, 1u},
    {0x52f330u, 176u, 2u}, {0x52f350u, 160u, 2u},
    {0x52f370u, 128u, 1u}, {0x52f390u, 136u, 2u},
    {0x52f3b0u, 132u, 2u}, {0x52f3d0u, 188u, 1u},
    {0x52f3f0u, 196u, 2u}, {0x52f410u, 192u, 2u},
    {0x52f430u, 200u, 1u}, {0x52f450u, 208u, 2u},
    {0x52f470u, 204u, 2u}, {0x52f490u, 212u, 1u},
    {0x52f4b0u, 216u, 2u}, {0x52f4d0u, 152u, 1u},
    {0x52f4f0u, 184u, 2u}, {0x52f510u, 168u, 2u},
    {0x52f530u, 140u, 1u}, {0x52f550u, 172u, 2u},
    {0x52f570u, 156u, 2u}, {0x52f590u, 224u, 1u},
    {0x52f5b0u, 232u, 2u}, {0x52f5d0u, 228u, 2u},
    {0x52f5f0u, 312u, 1u}, {0x52f610u, 324u, 2u},
    {0x52f630u, 336u, 2u}, {0x52f650u, 316u, 1u},
    {0x52f670u, 328u, 2u}, {0x52f690u, 340u, 2u},
    {0x52f6b0u, 308u, 1u}, {0x52f6d0u, 320u, 2u},
    {0x52f6f0u, 332u, 2u},
}};

}  // namespace

std::span<const ActorPropertyForwardSpec> ActorManagerPropertyForwarders::Specs() {
    return kSpecs;
}

std::uint32_t ActorManagerPropertyForwarders::Forward(
    std::uint32_t entry, std::uint32_t actor,
    std::span<const std::uint32_t> arguments,
    IActorManagerPropertyForwardRuntime& runtime) {
    const auto it = std::lower_bound(
        kSpecs.begin(), kSpecs.end(), entry,
        [](const ActorPropertyForwardSpec& spec, std::uint32_t address) {
            return spec.entry < address;
        });
    if (it == kSpecs.end() || it->entry != entry ||
        it->argument_count != arguments.size())
        throw std::invalid_argument("unknown property entry or argument count");
    return runtime.InvokeProperty(runtime.ReadWord(actor + 800u), actor,
                                  it->slot, arguments);
}

std::array<std::uint32_t, 3> ActorManagerPropertyForwarders::CopyChildPosition(
    std::uint32_t embedded_member,
    IActorManagerPropertyForwardRuntime& runtime) {
    return runtime.ChildPosition(embedded_member - 724u);
}

}  // namespace pal4
