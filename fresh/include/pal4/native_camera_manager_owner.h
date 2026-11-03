#pragma once

#include "pal4/camera_controller.h"
#include "pal4/camera_controller_access.h"
#include "pal4/camera_mode_configuration.h"
#include "pal4/camera_parameter_setters.h"
#include "pal4/camera_property_cleanup.h"
#include "pal4/camera_system_initializer.h"
#include "pal4/base_object_scene_parameter.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/script_unique_collection.h"

#include <array>
#include <cstdint>

namespace pal4 {

class ICameraTaskGeometry;
class NativeGameSaveArchiveReader;
class NativeGameSaveArchiveBuffer;
class ICallbackObjectRuntime;

class INativeCameraObjectResources {
public:
    virtual ~INativeCameraObjectResources() = default;
    [[nodiscard]] virtual CameraRenderRect RenderRect() = 0;
    virtual void LogInitialization(bool complete) = 0;
    [[nodiscard]] virtual bool Initialize(
        CameraSystemObjectState& object, CameraDataState& data,
        const CameraSetupParameters& parameters) = 0;
    virtual void Destroy(CameraSystemObjectState& object,
                         CameraDataState& data) noexcept = 0;
    virtual void SetFarClip(std::uint32_t camera, float distance) = 0;
    [[nodiscard]] virtual float MinimumDistance() = 0;
    [[nodiscard]] virtual float MaximumDistance() = 0;
    [[nodiscard]] virtual std::uint32_t UpdateMatrix(
        CameraDataState& data, std::uint32_t solve_eye) = 0;
    [[nodiscard]] virtual std::uint32_t AdvanceFrame(
        CameraDataState& data, const CameraControllerState& manager, float elapsed);
    [[nodiscard]] virtual ICameraTaskGeometry* TaskGeometry() { return nullptr; }
};

// Owns the startup controller, four camera objects and their CameraData images.
// SDK resources are supplied by an engine-bound implementation of the boundary.
class NativeCameraManagerOwner final : private ICameraSystemInitializerRuntime,
                                       private ICameraControllerAccessRuntime,
                                       private ICameraModeConfigurationRuntime,
                                       private ICameraParameterRuntime,
                                       private ICameraPropertyCleanupRuntime {
public:
    NativeCameraManagerOwner();
    NativeCameraManagerOwner(const NativeCameraManagerOwner&) = delete;
    NativeCameraManagerOwner& operator=(const NativeCameraManagerOwner&) = delete;
    ~NativeCameraManagerOwner();

    [[nodiscard]] std::uint32_t Construct();
    [[nodiscard]] bool InitializeCameras(INativeCameraObjectResources& resources);
    void Destroy();
    [[nodiscard]] std::uint32_t Address() noexcept;
    [[nodiscard]] bool Constructed() const noexcept { return constructed_; }
    [[nodiscard]] bool CamerasReady() const noexcept { return cameras_ready_; }
    [[nodiscard]] CameraControllerState& State() noexcept { return state_; }
    [[nodiscard]] const CameraSystemObjectState& Object(
        std::size_t index) const { return objects_.at(index); }
    [[nodiscard]] const CameraDataState& Data(
        std::size_t index) const { return data_.at(index); }
    [[nodiscard]] CameraDataState& Data(std::size_t index) {
        return data_.at(index);
    }
    [[nodiscard]] bool ActivateByName(const char* name);
    [[nodiscard]] std::uint32_t FindByName(const char* name);
    [[nodiscard]] std::uint32_t ActiveCameraPointer();
    [[nodiscard]] std::uint32_t ActiveCameraInternalId();
    void InitializeCameraAnimation(std::uint32_t camera, std::uint32_t time,
        std::uint32_t amplitude, std::uint32_t frequency);
    void SetCameraAnimationActive(std::uint32_t camera, std::uint32_t enabled);
    [[nodiscard]] CameraDataState& CameraDataForObject(
        std::uint32_t object) override;
    [[nodiscard]] CameraDataState& CameraDataAt(
        std::uint32_t address) override;
    [[nodiscard]] bool BindCameraToActor(std::uint32_t object,
        BaseObjectState& actor, IBaseObjectSceneParameterRuntime& runtime);
    [[nodiscard]] std::uint32_t SetCameraYaw(std::uint32_t camera,
        float yaw, std::uint32_t keep_look_at);
    [[nodiscard]] std::uint32_t SetCameraPitch(std::uint32_t camera,
        float pitch, std::uint32_t keep_look_at);
    [[nodiscard]] std::uint32_t SetCameraAngles(std::uint32_t camera,
        const std::array<float, 3>& angles, std::uint32_t keep_look_at);
    [[nodiscard]] bool SetCameraDistance(std::uint32_t camera,
        float distance, std::uint32_t keep_look_at);
    [[nodiscard]] std::uint32_t AdjustCameraAngles(std::uint32_t camera,
        float yaw_delta, float pitch_delta, std::uint32_t keep_look_at);
    [[nodiscard]] std::uint32_t AdjustCameraObjectYpr(std::uint32_t object,
        const std::array<float, 3>& delta, std::uint32_t keep_look_at);
    [[nodiscard]] bool BindMainCameraToActor(
        BaseObjectState& actor, IBaseObjectSceneParameterRuntime& runtime);
    void ClearMainCameraSceneParameter();
    [[nodiscard]] std::uint32_t SetMainCameraYaw(float yaw,
                                                  std::uint32_t keep_look_at);
    [[nodiscard]] float CameraFollowValue() const;
    [[nodiscard]] std::uint32_t SetMainCameraPitch(
        float pitch, std::uint32_t keep_look_at);
    [[nodiscard]] std::uint32_t AdjustMainCameraAngles(
        float yaw_delta, float pitch_delta, std::uint32_t keep_look_at);
    [[nodiscard]] bool SetModeByIndex(std::uint32_t index);
    // IDA5EB630. Scene task owners must retire their tasks before this reset.
    void ConfigureSceneModes();
    // Borrow the creating owner's runtime before populating the named tree.
    // Retire/unbind after cleanup, before destroying the runtime's dependencies.
    void BindNamedCameraCleanup(ICallbackObjectRuntime* runtime);
    // IDA5EB600/4012A0. Nonempty collections require their creating runtime.
    void ResetSceneProperties();
    // IDA5EBFB0: consumes CAM_MGR XML at the caller's current archive cursor.
    // Requires initialized cameras and applies mode/angles through shared setters.
    [[nodiscard]] bool LoadConfig(NativeGameSaveArchiveReader& archive);
    // IDA5EBD00: append CAM_MGR using the MainCamera, even if another is active.
    [[nodiscard]] bool SaveConfig(NativeGameSaveArchiveBuffer& archive);
    [[nodiscard]] bool SetCameraTarget(std::uint32_t camera,
                                       float x, float y, float z,
                                       std::uint32_t keep_look_at);
    [[nodiscard]] bool AddSceneClump(std::uint32_t clump);
    [[nodiscard]] bool RemoveSceneClump(std::uint32_t clump);
    [[nodiscard]] std::size_t SceneClumpCount() const noexcept;
    [[nodiscard]] bool SetActiveFarClip(float distance);
    [[nodiscard]] std::uint32_t AdvanceActiveCamera(float elapsed);

private:
    class ConfigInputRuntime;
    class ConfigOutputRuntime;
    class Runtime final : public ICameraControllerRuntime,
                          public IScriptUniqueCollectionRuntime {
    public:
        explicit Runtime(NativeCameraManagerOwner& owner) : owner_(owner) {}
        void SetName(std::uint32_t& name_pointer,
                     const char* name) override;
        void DestroyNameObject(CameraControllerState& state,
                               std::uint32_t offset) override;
        [[nodiscard]] std::uint32_t FreeData(std::uint32_t data) override;
        [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;
        void WriteWord(std::uint32_t address,
                       std::uint32_t value) override;
        [[nodiscard]] std::uint32_t Allocate(std::uint32_t bytes) override;
        void Free(std::uint32_t address) override;
    private:
        NativeCameraManagerOwner& owner_;
    };

    void LogInitialization(bool complete) override;
    [[nodiscard]] CameraRenderRect RenderRect() override;
    [[nodiscard]] std::uint32_t AllocateObject(std::uint32_t bytes) override;
    void ConstructObject(std::uint32_t object) override;
    [[nodiscard]] bool SetupObject(
        std::uint32_t object,
        const CameraSetupParameters& parameters) override;
    [[nodiscard]] std::size_t ObjectIndex(std::uint32_t object) const;
    [[nodiscard]] CameraSystemObjectState& ObjectAt(
        std::uint32_t address) override;
    void SetFarClip(std::uint32_t camera, float distance) override;
    void ClearObjectCollection(std::uint32_t object,
                               std::uint32_t offset) override;
    [[nodiscard]] std::optional<std::uint32_t> FindNamedObject(
        const CameraControllerState& state,
        std::uint32_t collection_offset, const char* name) override;
    void ReportMissingCameraName(const char* name) override;
    [[nodiscard]] float MinimumDistance() override;
    [[nodiscard]] float MaximumDistance() override;
    [[nodiscard]] std::uint32_t UpdateMatrix(
        CameraDataState& data, std::uint32_t solve_eye) override;
    void DestroyCameras() noexcept;
    [[nodiscard]] std::uint32_t IsEmpty(
        CameraControllerState& state, std::uint32_t offset) override;
    void ClearCollection(CameraControllerState& state,
                         std::uint32_t offset) override;

    CameraControllerState state_{};
    std::array<CameraSystemObjectState, 4> objects_{};
    std::array<CameraDataState, 4> data_{};
    NativeBaseObjectMemory collection_memory_;
    Runtime runtime_;
    INativeCameraObjectResources* resources_{};
    ICallbackObjectRuntime* named_cleanup_{};
    std::size_t object_count_{};
    bool setup_succeeded_{};
    bool constructed_{};
    bool cameras_ready_{};
};

}  // namespace pal4
