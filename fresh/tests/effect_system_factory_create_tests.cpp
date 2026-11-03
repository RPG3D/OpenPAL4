#include "pal4/effect_system_factory_registry.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Expected {
    pal4::EffectFactoryKind kind;
    std::string_view name;
    std::uint32_t entry;
    std::uint32_t bytes;
};

constexpr std::array<Expected, 11u> kExpected{{
    {pal4::EffectFactoryKind::Clump, "ClumpObject", 0x63e1f0u, 0x144u},
    {pal4::EffectFactoryKind::RtkPrtStdAtomic,
     "RtkPrtStdAtomicObject", 0x63e490u, 0x88u},
    {pal4::EffectFactoryKind::Sound3D, "Sound3DObject", 0x63e250u, 0x5cu},
    {pal4::EffectFactoryKind::Cylinder, "CylinderObject", 0x63e2b0u,
     0x9cu},
    {pal4::EffectFactoryKind::Strip, "StripObject", 0x63e310u, 0x12cu},
    {pal4::EffectFactoryKind::Effect, "EffectObject", 0x63e370u, 0xa8u},
    {pal4::EffectFactoryKind::BillBord, "BillBordObject", 0x63e3d0u,
     0xa8u},
    {pal4::EffectFactoryKind::Camera, "CameraObject", 0x63e430u, 0xc0u},
    {pal4::EffectFactoryKind::SequnceAnim, "SequnceAnimObject", 0x63e4f0u,
     0xa8u},
    {pal4::EffectFactoryKind::DynamicStrip, "DynamicStripObject", 0x63e550u,
     0xe4u},
    {pal4::EffectFactoryKind::Thunder, "ThunderObject", 0x63e5b0u,
     0x124u},
}};

struct Runtime final : pal4::IEffectSystemFactoryCreationRuntime {
    std::uint32_t allocation = 0x3000u;
    std::uint32_t requested = 0u;
    std::vector<pal4::EffectFactoryKind> constructed;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        requested = bytes;
        return allocation;
    }
    std::uint32_t Construct(pal4::EffectFactoryKind kind,
                            std::uint32_t object) override {
        Check(object == allocation, "constructor receives allocated object");
        constructed.push_back(kind);
        return object + 4u;
    }
};

void TestFactories() {
    Runtime runtime;
    for (std::size_t i = 0u; i < kExpected.size(); ++i) {
        const auto& expected = kExpected[i];
        const auto& descriptor =
            pal4::EffectSystemFactoryRegistry::kFactories[i];
        Check(descriptor.name == expected.name &&
                  descriptor.creator == expected.entry &&
                  descriptor.bytes == expected.bytes &&
                  pal4::EffectSystemFactoryRegistry::Create(
                      expected.kind, runtime) == runtime.allocation + 4u &&
                  runtime.requested == expected.bytes &&
                  runtime.constructed.back() == expected.kind,
              "IDA factory entry, type, size and constructor result");
    }
    Check(runtime.constructed.size() == kExpected.size(),
          "every factory invoked its constructor once");
}

void TestAllocationFailure() {
    Runtime runtime;
    runtime.allocation = 0u;
    for (const auto& expected : kExpected) {
        Check(pal4::EffectSystemFactoryRegistry::Create(
                  expected.kind, runtime) == 0u &&
                  runtime.requested == expected.bytes &&
                  runtime.constructed.empty(),
              "allocation failure returns null without construction");
    }
}

}  // namespace

int main() {
    TestFactories();
    TestAllocationFailure();
}
