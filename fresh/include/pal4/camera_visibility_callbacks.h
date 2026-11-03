#pragma once

#include "pal4/camera_render_object.h"

#include <cstdint>

namespace pal4 {

class ICameraVisibilityRuntime {
public:
    virtual ~ICameraVisibilityRuntime() = default;
    virtual void TraverseSectors(
        std::uint32_t camera, std::uint32_t& candidate,
        std::uint32_t (*callback)(std::uint32_t, std::uint32_t&)) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void ProcessGlobal() = 0;
};

class CameraVisibilityRuntimeStub final : public ICameraVisibilityRuntime {
public:
    void TraverseSectors(
        std::uint32_t, std::uint32_t&,
        std::uint32_t (*)(std::uint32_t, std::uint32_t&)) override {}
    [[nodiscard]] float ReadFloat(std::uint32_t) override { return 0.0f; }
    [[nodiscard]] std::uint32_t ReadWord(
        std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    void ProcessGlobal() override {}
};

class CameraVisibilityCallback final {
public:
    // IDA 0x5fced0.
    [[nodiscard]] static std::uint32_t Process(
        std::uint32_t sector, CameraRenderState& state,
        ICameraVisibilityRuntime& runtime);
};

class ICameraAtomicUserDataRuntime {
public:
    virtual ~ICameraAtomicUserDataRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::int32_t UsedCount(
        std::uint32_t geometry) = 0;
    [[nodiscard]] virtual std::uint32_t GetItem(
        std::uint32_t geometry, std::int32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t GetText(
        std::uint32_t item, std::int32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeString(
        std::uint32_t text) = 0;
    virtual void LowercaseString(std::uint32_t string) = 0;
    [[nodiscard]] virtual std::uint32_t GetClumpManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindClump(
        std::uint32_t manager, std::uint32_t string) = 0;
    virtual void CleanupString(std::uint32_t string) = 0;
};

class CameraAtomicUserDataRuntimeStub final
    : public ICameraAtomicUserDataRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(
        std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::int32_t UsedCount(
        std::uint32_t) override { return 0; }
    [[nodiscard]] std::uint32_t GetItem(
        std::uint32_t, std::int32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t GetText(
        std::uint32_t, std::int32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t InitializeString(
        std::uint32_t) override { return 0u; }
    void LowercaseString(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t GetClumpManager() override { return 0u; }
    [[nodiscard]] std::uint32_t FindClump(
        std::uint32_t, std::uint32_t) override { return 0u; }
    void CleanupString(std::uint32_t) override {}
};

class CameraAtomicUserData final {
public:
    // IDA 0x5fcf80.
    [[nodiscard]] static std::uint32_t ClearUnmatchedFlags(
        std::uint32_t atomic, ICameraAtomicUserDataRuntime& runtime);
    // IDA 0x5fd040.
    [[nodiscard]] static std::uint32_t SetUnmatchedFlags(
        std::uint32_t atomic, ICameraAtomicUserDataRuntime& runtime);
};

}  // namespace pal4
