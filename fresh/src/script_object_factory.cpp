#include "pal4/script_object_factory.h"

#include <array>
#include <cstddef>

namespace pal4 {
namespace {

enum class ConstructorKind {
    Base12,
    Simple12,
    CountCallback,
    PendingPointer,
    PendingGate,
    ReactiveGate,
    VariantEight
};

struct Specification {
    std::uint32_t size;
    std::uint32_t vtable;
    ConstructorKind constructor;
    bool pointer_first;
};

constexpr std::array<Specification, 5> kGroupZero{{
    {12u, 0x847e10u, ConstructorKind::Base12, false},
    {12u, 0x847cb0u, ConstructorKind::Base12, false},
    {12u, 0x847b50u, ConstructorKind::Simple12, true},
    {12u, 0x8479f0u, ConstructorKind::Simple12, true},
    {12u, 0x847890u, ConstructorKind::Simple12, true},
}};
constexpr std::array<Specification, 6> kGroupOne{{
    {12u, 0x847730u, ConstructorKind::Simple12, true},
    {12u, 0x8475d0u, ConstructorKind::Simple12, true},
    {12u, 0x847470u, ConstructorKind::Simple12, true},
    {12u, 0x847310u, ConstructorKind::Simple12, true},
    {12u, 0x8471b0u, ConstructorKind::Simple12, true},
    {12u, 0x847050u, ConstructorKind::Simple12, true},
}};
constexpr std::array<Specification, 6> kGroupTwo{{
    {16u, 0x846290u, ConstructorKind::CountCallback, true},
    {16u, 0x8463f0u, ConstructorKind::PendingPointer, true},
    {16u, 0x846550u, ConstructorKind::PendingGate, true},
    {12u, 0x846ef0u, ConstructorKind::Simple12, true},
    {16u, 0x8466b0u, ConstructorKind::ReactiveGate, true},
    {12u, 0x846d90u, ConstructorKind::Simple12, true},
}};
constexpr std::array<Specification, 5> kGroupEight{{
    {16u, 0x847f70u, ConstructorKind::VariantEight, true},
    {16u, 0x846c30u, ConstructorKind::VariantEight, false},
    {16u, 0x846ad0u, ConstructorKind::VariantEight, true},
    {16u, 0x846970u, ConstructorKind::VariantEight, false},
    {16u, 0x846810u, ConstructorKind::VariantEight, false},
}};

template <std::size_t N>
const Specification* FindIn(const std::array<Specification, N>& specs,
                            std::int32_t variant) {
    if (variant < 1 || variant > static_cast<std::int32_t>(N))
        return nullptr;
    return &specs[static_cast<std::size_t>(variant - 1)];
}

const Specification* Find(std::int32_t group, std::int32_t variant) {
    switch (group) {
        case 0: return FindIn(kGroupZero, variant);
        case 1: return FindIn(kGroupOne, variant);
        case 2: return FindIn(kGroupTwo, variant);
        case 8: return FindIn(kGroupEight, variant);
        default: return nullptr;
    }
}

void Construct(std::uint32_t object, const Specification& spec,
               IScriptObjectFactoryRuntime& runtime) {
    switch (spec.constructor) {
        case ConstructorKind::Base12:
            (void)ScriptObjectFactory::ConstructBase12(object, runtime);
            runtime.WriteWord(object, spec.vtable);
            return;
        case ConstructorKind::Simple12:
            runtime.WriteByte(object + 4u, 0u);
            runtime.WriteWord(object + 8u, 0u);
            runtime.WriteWord(object, spec.vtable);
            return;
        case ConstructorKind::CountCallback:
            runtime.WriteByte(object + 4u, 0u);
            runtime.WriteWord(object + 8u, 0u);
            runtime.WriteWord(object + 12u, 0u);
            runtime.WriteWord(object, spec.vtable);
            return;
        case ConstructorKind::PendingPointer:
            runtime.WriteByte(object + 4u, 0u);
            runtime.WriteWord(object + 8u, 0u);
            runtime.WriteWord(object, spec.vtable);
            runtime.WriteWord(object + 12u, 0u);
            return;
        case ConstructorKind::PendingGate:
            runtime.WriteByte(object + 4u, 0u);
            runtime.WriteWord(object + 8u, 0u);
            runtime.WriteWord(object, spec.vtable);
            return;
        case ConstructorKind::ReactiveGate:
            runtime.WriteByte(object + 4u, 0u);
            runtime.WriteWord(object + 8u, 0u);
            runtime.WriteByte(object + 12u, 0u);
            runtime.WriteWord(object, spec.vtable);
            return;
        case ConstructorKind::VariantEight:
            runtime.WriteByte(object + 4u, 0u);
            runtime.WriteWord(object + 8u, 0u);
            runtime.WriteByte(object + 12u, 0u);
            runtime.WriteWord(object, 0x847f70u);
            if (spec.vtable != 0x847f70u)
                runtime.WriteWord(object, spec.vtable);
            return;
    }
}

}  // namespace

std::uint32_t ScriptObjectFactory::Create(
    std::uint32_t output, std::int32_t group,
    std::int32_t variant, IScriptObjectFactoryRuntime& runtime) {
    const auto* spec = Find(group, variant);
    const auto object = spec == nullptr ? 0u : runtime.Allocate(spec->size);
    if (object != 0u) Construct(object, *spec, runtime);

    const auto valid = static_cast<std::uint8_t>(object != 0u);
    if (spec != nullptr && spec->pointer_first) {
        runtime.WriteWord(output + 4u, object);
        runtime.WriteByte(output, valid);
    } else {
        runtime.WriteByte(output, valid);
        runtime.WriteWord(output + 4u, object);
    }
    return output;
}

std::uint32_t ScriptObjectFactory::ConstructBase12(
    std::uint32_t object, IScriptObjectFactoryRuntime& runtime) {
    runtime.WriteByte(object + 4u, 0u);
    runtime.WriteWord(object + 8u, 0u);
    runtime.WriteWord(object, 0x845fd0u);
    return object;
}

}  // namespace pal4
