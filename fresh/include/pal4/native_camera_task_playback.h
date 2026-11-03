#pragma once

#include "pal4/camera_parameter_setters.h"
#include "pal4/camera_task_object.h"
#include "pal4/native_base_object_memory.h"

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace pal4 {

class NativeCameraFileCatalog;
class NativeCameraManagerOwner;
class INativeCameraObjectResources;

// Executes a prepared .cam record against an owned native camera image.
class NativeCameraTaskPlayback final : public ICameraTaskPlaybackRuntime,
                                       public ICameraTaskFactoryRuntime,
                                       private ICameraParameterRuntime {
public:
    NativeCameraTaskPlayback(NativeCameraManagerOwner& cameras,
                             INativeCameraObjectResources& resources,
                             const NativeCameraFileCatalog& records);
    ~NativeCameraTaskPlayback() override;

    [[nodiscard]] std::uint32_t Create();
    void Configure(std::uint32_t camera_data, std::uint32_t record);
    void Configure(std::uint32_t task, std::uint32_t camera_data,
                   std::uint32_t record);
    [[nodiscard]] bool Enter();
    [[nodiscard]] bool Enter(std::uint32_t task);
    [[nodiscard]] bool Advance(float elapsed);
    [[nodiscard]] bool Advance(std::uint32_t task, float elapsed);
    [[nodiscard]] bool Completed() const;
    [[nodiscard]] bool Completed(std::uint32_t task) const;
    void Destroy();
    void Destroy(std::uint32_t task);
    [[nodiscard]] std::uint32_t Address() const noexcept { return task_; }
    [[nodiscard]] bool Owns(std::uint32_t task) const noexcept;
    [[nodiscard]] std::size_t OutstandingAllocations() const noexcept {
        return memory_.OutstandingAllocations();
    }

private:
    void BaseConstruct(std::uint32_t object) override;
    [[nodiscard]] std::uint32_t BaseDestruct(std::uint32_t object) override;
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t address) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    [[nodiscard]] std::uint32_t AllocateMemory(std::uint32_t bytes) override;
    void FreeMemory(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t FindXmlChild(
        std::uint32_t document, std::string_view name) override;
    [[nodiscard]] std::optional<float> ConfigFloat(
        std::uint32_t child, std::string_view name) override;
    void SetLookAt(std::uint32_t camera, CameraTaskVector position,
                   std::uint32_t flag) override;
    void SetSecondAngle(std::uint32_t camera, float angle,
                        std::uint32_t flag) override;
    void SetAngle(std::uint32_t camera, float angle,
                  std::uint32_t flag) override;
    void SetThirdAngle(std::uint32_t camera, float angle,
                       std::uint32_t flag) override;
    void SetScalar(std::uint32_t camera, float scalar,
                   std::uint32_t flag) override;
    [[nodiscard]] float InterpolateFloat(float start, float end,
                                          float fraction,
                                          std::uint32_t mode) override;
    void SetPosition(std::uint32_t camera, CameraTaskVector position,
                     std::uint32_t flag) override;
    void SetEye(std::uint32_t camera, CameraTaskVector position,
                std::uint32_t flag) override;
    [[nodiscard]] float MinimumDistance() override;
    [[nodiscard]] float MaximumDistance() override;
    [[nodiscard]] std::uint32_t UpdateMatrix(
        CameraDataState& state, std::uint32_t mode) override;

    [[nodiscard]] CameraDataState& Camera(std::uint32_t address);
    [[nodiscard]] bool TaskAddress(std::uint32_t address,
                                   std::uint32_t bytes) const noexcept;

    NativeCameraManagerOwner& cameras_;
    INativeCameraObjectResources& resources_;
    const NativeCameraFileCatalog& records_;
    NativeBaseObjectMemory memory_;
    std::vector<std::uint32_t> tasks_;
    std::uint32_t task_{};
    std::uint32_t pending_task_{};
};

}  // namespace pal4
