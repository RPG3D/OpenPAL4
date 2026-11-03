#pragma once

#include "pal4/native_base_object_memory.h"
#include "pal4/native_scene_gob_catalog.h"
#include "pal4/rw_dict_schema.h"

#include <cstdint>
#include <array>
#include <functional>
#include <optional>
#include <memory>
#include <string_view>
#include <utility>

namespace pal4 {

class IGameResourceManagerRuntime;
class NativeSceneMovementAsset;
class NativeGobConfigDocument;
class NativeEventDocumentRuntime;
struct GameResourceManager;

class INativeGobPlatformService {
public:
    virtual ~INativeGobPlatformService() = default;
    virtual std::uint32_t Touch(std::uint32_t argument) = 0;
    virtual void AttachMovement(NativeSceneMovementAsset&) = 0;
    virtual void DetachMovement(NativeSceneMovementAsset&) = 0;
};

// Derived Obstacle runtime borrows the image and its primary collision clump.
// Its helper callback is registered only while this collision attachment lives.
class INativeGobObstacleService {
public:
    virtual ~INativeGobObstacleService() = default;
    virtual std::uint32_t Touch(std::uint32_t event) = 0;
    virtual void AttachMovement(NativeSceneMovementAsset&) = 0;
    virtual void DetachMovement(NativeSceneMovementAsset&) = 0;
};

class INativeGobAnimationService {
public:
    virtual ~INativeGobAnimationService() = default;
    [[nodiscard]] virtual bool Select(std::string_view action) = 0;
    [[nodiscard]] virtual float Length() const = 0;
    [[nodiscard]] virtual float Time() const = 0;
    [[nodiscard]] virtual bool SetTime(float time) = 0;
    [[nodiscard]] virtual bool ProcessRender(std::uint32_t camera)=0;
};

class INativeGobMachineService {
public:
    virtual ~INativeGobMachineService() = default;
    virtual std::uint32_t Trigger() = 0;
    virtual std::uint32_t ClearContainer(std::uint32_t) = 0;
    virtual void SceneActivity(bool active) = 0;
};

class INativeGobSoundService {
public:
    virtual ~INativeGobSoundService() = default;
    virtual void Reload(const NativeSceneGobRecord&) = 0;
    virtual void SetActive(bool active) = 0;
    virtual std::uint32_t StopAndRemove() = 0;
};

// Scene-owned effect service. The original two-word handle is kept by the GOB.
class INativeGobEffectService {
public:
    virtual ~INativeGobEffectService() = default;
    [[nodiscard]] virtual std::pair<std::uint32_t, std::uint32_t> Create(
        std::string_view name, std::uint32_t times, bool distance_clip) = 0;
    [[nodiscard]] virtual bool IsActive(std::uint32_t id,
                                        std::uint32_t owner) const = 0;
    virtual void Stop(std::uint32_t id, std::uint32_t owner) = 0;
    // Hide drains remaining particles; replacement/resource cleanup releases now.
    virtual void Release(std::uint32_t id, std::uint32_t owner) = 0;
    virtual void Attach(std::uint32_t id, std::uint32_t owner,
                        std::uint32_t parent_frame) = 0;
    [[nodiscard]] virtual std::uint8_t Update(std::uint32_t id,
                                              std::uint32_t owner) = 0;
};

// Owns supported scene GOB images and their inherited state; the RenderWare
// Clump and execution services remain borrowed from their creating scene.
class NativeGobObjectImage final {
public:
    NativeGobObjectImage(NativeBaseObjectMemory& memory,
                         IRwDictSchemaRuntime& schema);
    ~NativeGobObjectImage();
    NativeGobObjectImage(const NativeGobObjectImage&) = delete;
    NativeGobObjectImage& operator=(const NativeGobObjectImage&) = delete;

    [[nodiscard]] std::uint32_t Construct(std::uint32_t type);
    void BindRecord(const NativeSceneGobRecord& record,
                    std::uint32_t borrowed_clump,
                    IGameResourceManagerRuntime& transforms,
                    const std::function<void(GameResourceManager&)>& prepare_resource = {});
    [[nodiscard]] std::uint8_t SetVisible(std::uint32_t visible);
    void SetPosition(const std::array<float, 3>& position);
    void SetRotation(const std::array<float, 3>& rotation, bool relative = false);
    std::uint32_t AdvanceResource(float elapsed);
    void AssignSoundName(std::string_view);
    void BindSoundService(INativeGobSoundService* service) noexcept {sound_service_=service;}
    void SetScale(const std::array<float, 3>& scale);
    void BindMachineService(INativeGobMachineService* service) noexcept {machine_service_=service;}
    INativeGobMachineService* MachineService() const noexcept {return machine_service_;}
    void BindPlatformService(INativeGobPlatformService* service) noexcept { platform_service_ = service; }
    INativeGobPlatformService* PlatformService() const noexcept { return platform_service_; }
    void BindObstacleService(INativeGobObstacleService* service) noexcept { obstacle_service_ = service; }
    INativeGobObstacleService* ObstacleService() const noexcept { return obstacle_service_; }
    [[nodiscard]] std::optional<BinaryXmlConfig> CaptureSaveDocument() const;
    void ApplySaveDocument(const BinaryXmlConfig& document);
    void ResetActionAnimation();
    // Virtual +0x74: re-read retained configuration, distinct from animation reset.
    void ReloadConfiguration(const NativeSceneGobRecord& record);
    void BindEffectService(INativeGobEffectService* service) noexcept {
        effect_service_ = service;
    }
    void BindAnimationService(INativeGobAnimationService* service) noexcept {
        animation_service_ = service;
    }
    [[nodiscard]] INativeGobAnimationService* AnimationService() const noexcept {
        return animation_service_;
    }
    void Render(std::uint32_t camera);
    void Activate();
    void Deactivate();
    void CleanupResources(std::uint32_t texture_context,
                          IGameResourceManagerRuntime& resources,
                          const std::function<void()>& release_animation,
                          const std::function<std::uint32_t(std::uint32_t)>& stop_audio = {});
    void Destroy();
    [[nodiscard]] std::uint32_t Address() const noexcept { return object_; }
    [[nodiscard]] std::uint32_t Type() const noexcept { return type_; }
    [[nodiscard]] bool Bound() const noexcept { return bound_; }
    [[nodiscard]] std::uint32_t ConfigurationAddress() const noexcept;
    NativeEventDocumentRuntime& ConfigurationDocuments();

private:
    class Runtime;
    void ApplySceneConfiguration();
    void ApplyActionConfiguration();
    NativeBaseObjectMemory& memory_;
    IRwDictSchemaRuntime& schema_;
    std::unique_ptr<NativeGobConfigDocument> config_document_;
    std::uint32_t object_ = 0u;
    std::uint32_t type_ = 0u;
    bool bound_ = false;
    bool rendering_{};
    IGameResourceManagerRuntime* transforms_ = nullptr;
    INativeGobEffectService* effect_service_ = nullptr;
    INativeGobAnimationService* animation_service_ = nullptr;
    INativeGobMachineService* machine_service_ = nullptr;
    INativeGobPlatformService* platform_service_ = nullptr;
    INativeGobObstacleService* obstacle_service_ = nullptr;
    INativeGobSoundService* sound_service_ = nullptr;
};

}  // namespace pal4
