#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct EffectFactoryDescriptor {
    std::string_view name;
    std::uint32_t creator;
    std::uint32_t bytes;
    bool via_event_helper;
};

enum class EffectFactoryKind : std::uint8_t {
    Clump,
    RtkPrtStdAtomic,
    Sound3D,
    Cylinder,
    Strip,
    Effect,
    BillBord,
    Camera,
    SequnceAnim,
    DynamicStrip,
    Thunder,
};

class IEffectSystemFactoryCreationRuntime {
public:
    virtual ~IEffectSystemFactoryCreationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Construct(
        EffectFactoryKind kind, std::uint32_t object) = 0;
};

class IEffectSystemFactoryRuntime {
public:
    virtual ~IEffectSystemFactoryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t RegisterEffectSystem() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void ClearRegistry(std::uint32_t tree) = 0;
    [[nodiscard]] virtual bool HasFactory(std::uint32_t tree,
                                          std::string_view name) = 0;
    virtual void InsertFactory(std::uint32_t tree, std::string_view name,
                               std::uint32_t creator) = 0;
    [[nodiscard]] virtual std::uint8_t ReleaseName(
        std::string_view name) = 0;
};

class EffectSystemFactoryRuntimeStub : public IEffectSystemFactoryRuntime {
public:
    [[nodiscard]] std::uint32_t RegisterEffectSystem() override { return 0u; }
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void ClearRegistry(std::uint32_t) override {}
    [[nodiscard]] bool HasFactory(std::uint32_t,
                                  std::string_view) override { return false; }
    void InsertFactory(std::uint32_t, std::string_view,
                       std::uint32_t) override {}
    [[nodiscard]] std::uint8_t ReleaseName(
        std::string_view) override { return 0u; }
};

class EffectSystemFactoryRegistry final {
public:
    static constexpr std::array<EffectFactoryDescriptor, 11u> kFactories{{
        {"ClumpObject", 0x63e1f0u, 0x144u, false},
        {"RtkPrtStdAtomicObject", 0x63e490u, 0x88u, false},
        {"Sound3DObject", 0x63e250u, 0x5cu, false},
        {"CylinderObject", 0x63e2b0u, 0x9cu, false},
        {"StripObject", 0x63e310u, 0x12cu, false},
        {"EffectObject", 0x63e370u, 0xa8u, false},
        {"BillBordObject", 0x63e3d0u, 0xa8u, true},
        {"CameraObject", 0x63e430u, 0xc0u, true},
        {"SequnceAnimObject", 0x63e4f0u, 0xa8u, true},
        {"DynamicStripObject", 0x63e550u, 0xe4u, true},
        {"ThunderObject", 0x63e5b0u, 0x124u, true},
    }};

    // IDA 0x63dc40: register eleven object factories in source order.
    [[nodiscard]] static std::uint8_t InitializeEvents(
        IEffectSystemFactoryRuntime& runtime);
    // IDA 0x63e7d0: insert a named factory only when absent.
    [[nodiscard]] static std::uint8_t ProcessEvent(
        std::uint32_t tree, std::string_view name,
        std::uint32_t creator,
        IEffectSystemFactoryRuntime& runtime);
    // IDA 0x63e1f0..0x63e5b0: eleven fixed-size factory entrypoints.
    [[nodiscard]] static std::uint32_t Create(
        EffectFactoryKind kind,
        IEffectSystemFactoryCreationRuntime& runtime);
};

}  // namespace pal4
