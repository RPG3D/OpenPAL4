#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include "pal4/collection_lifecycle.h"

namespace pal4 {

class ICollectionCameraRuntime {
public:
    virtual ~ICollectionCameraRuntime() = default;
    [[nodiscard]] virtual std::uint32_t InitializeCameraSubsystem() = 0;
    [[nodiscard]] virtual std::uint32_t FindCamera(
        std::uint32_t manager, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t InternalObject(
        std::uint32_t camera) = 0;
    virtual void SetAngles(std::uint32_t internal,
                           std::uint32_t first_angle_bits,
                           std::uint32_t second_angle_bits,
                           std::uint32_t zero, std::uint32_t mode) = 0;
    virtual void SetLookAt(std::uint32_t internal,
                           const std::array<std::uint32_t, 3>& words,
                           std::uint32_t mode) = 0;
    virtual void SetScalar(std::uint32_t internal,
                           std::uint32_t scalar_bits,
                           std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t CameraContainer(
        std::uint32_t camera) = 0;
    virtual void SetWorldCamera(CollectionState& state,
                                std::uint32_t container) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeLogger() = 0;
    [[nodiscard]] virtual std::uint32_t AppendLog(
        std::uint32_t logger, std::string_view format,
        std::string_view source, std::uint32_t line) = 0;
};

class CollectionCameraRuntimeStub : public ICollectionCameraRuntime {
public:
    [[nodiscard]] std::uint32_t InitializeCameraSubsystem() override {
        return 0u;
    }
    [[nodiscard]] std::uint32_t FindCamera(
        std::uint32_t, std::string_view) override { return 0u; }
    [[nodiscard]] std::uint32_t InternalObject(
        std::uint32_t) override { return 0u; }
    void SetAngles(std::uint32_t, std::uint32_t, std::uint32_t,
                   std::uint32_t, std::uint32_t) override {}
    void SetLookAt(std::uint32_t,
                   const std::array<std::uint32_t, 3>&,
                   std::uint32_t) override {}
    void SetScalar(std::uint32_t, std::uint32_t,
                   std::uint32_t) override {}
    [[nodiscard]] std::uint32_t CameraContainer(
        std::uint32_t) override { return 0u; }
    void SetWorldCamera(CollectionState&, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t InitializeLogger() override { return 0u; }
    [[nodiscard]] std::uint32_t AppendLog(
        std::uint32_t, std::string_view, std::string_view,
        std::uint32_t) override { return 0u; }
};

class CollectionCameraBootstrap final {
public:
    static constexpr std::string_view kCameraName = "UiCamera";
    static constexpr std::uint32_t kFirstAngleBits = 0x43350000u;
    static constexpr std::uint32_t kSecondAngleBits = 0x43b0c000u;
    static constexpr std::array<std::uint32_t, 3> kLookAtBits{
        0x412220c5u, 0x43d20000u, 0xc400f0c5u};
    static constexpr std::uint32_t kDistanceBits = 0x448cc000u;

    [[nodiscard]] static std::uint32_t InitializeAndSetup(
        CollectionState& state, ICollectionCameraRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetActiveCamera(
        ICollectionCameraRuntime& runtime);
};

}  // namespace pal4
