#include "pal4/actor_manager_property_forwarders.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

using Spec = pal4::ActorPropertyForwardSpec;
constexpr std::array<Spec, 39> expected{{
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

struct Runtime final : pal4::IActorManagerPropertyForwardRuntime {
    std::uint32_t expected_slot = 0u;
    std::span<const std::uint32_t> expected_arguments;
    std::vector<std::string> trace;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x1320u, "actor property pointer offset");
        trace.emplace_back("property");
        return 0x2000u;
    }
    std::uint32_t InvokeProperty(
        std::uint32_t property, std::uint32_t actor,
        std::uint32_t slot,
        std::span<const std::uint32_t> arguments) override {
        Check(property == 0x2000u && actor == 0x1000u &&
              slot == expected_slot &&
              arguments.size() == expected_arguments.size(),
              "property virtual dispatch identity");
        for (std::size_t i = 0; i < arguments.size(); ++i)
            Check(arguments[i] == expected_arguments[i],
                  "property argument order");
        trace.emplace_back("invoke");
        return slot + 42u;
    }
    std::array<std::uint32_t, 3> ChildPosition(
        std::uint32_t actor) override {
        Check(actor == 0x1000u, "embedded member owner offset");
        trace.emplace_back("position");
        return {0x3f800000u, 0x40000000u, 0x40400000u};
    }
};

void AllForwardEntries() {
    const auto specs = pal4::ActorManagerPropertyForwarders::Specs();
    Check(specs.size() == expected.size(), "all 39 IDA forwards listed");
    for (std::size_t i = 0; i < expected.size(); ++i) {
        const auto& value = expected[i];
        Check(specs[i].entry == value.entry &&
              specs[i].slot == value.slot &&
              specs[i].argument_count == value.argument_count &&
              (i == 0u || specs[i - 1u].entry < specs[i].entry),
              "IDA address-to-slot and arity mapping");
        Runtime runtime;
        runtime.expected_slot = value.slot;
        const std::array arguments{0x55u, 0x66u};
        const std::span call_arguments{arguments.data(),
                                       value.argument_count};
        runtime.expected_arguments = call_arguments;
        Check(pal4::ActorManagerPropertyForwarders::Forward(
                  value.entry, 0x1000u, call_arguments, runtime) ==
                  value.slot + 42u &&
              runtime.trace ==
                  std::vector<std::string>{"property", "invoke"},
              "forwarded return and exact argument order");
    }
}

void PositionAndRejection() {
    Runtime runtime;
    Check(pal4::ActorManagerPropertyForwarders::CopyChildPosition(
              0x12d4u, runtime) ==
              std::array<std::uint32_t, 3>{
                  0x3f800000u, 0x40000000u, 0x40400000u} &&
          runtime.trace == std::vector<std::string>{"position"},
          "embedded position helper copies all three component words");

    const std::array argument{0x55u};
    try {
        (void)pal4::ActorManagerPropertyForwarders::Forward(
            0x52f280u, 0x1000u, argument, runtime);
        Check(false, "position entry is not a virtual forward");
    } catch (const std::invalid_argument&) {
    }
    try {
        (void)pal4::ActorManagerPropertyForwarders::Forward(
            0x52f230u, 0x1000u, {}, runtime);
        Check(false, "wrong arity must be rejected");
    } catch (const std::invalid_argument&) {
    }
    Check(runtime.trace == std::vector<std::string>{"position"},
          "invalid forwards do not touch runtime");
}

}  // namespace

int main() {
    try {
        AllForwardEntries();
        PositionAndRejection();
        std::cout << "actor_manager_property_forwarders_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
