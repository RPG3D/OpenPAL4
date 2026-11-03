#pragma once
#include "pal4/native_effect_task_factory.h"
#include "pal4/native_effect_object_queues.h"
#include "pal4/native_effect_position_facets.h"
#include "pal4/native_effect_track_facets.h"
#include "pal4/native_effect_rotation_facets.h"
#include "pal4/native_effect_revolution_facets.h"
#include "pal4/native_effect_shadow_facets.h"
#include "pal4/native_effect_playback_facets.h"
#include "pal4/native_effect_sound.h"
#include "pal4/native_effect_camera_facets.h"
#include "pal4/native_effect_message_facets.h"
#include <memory>
#include <filesystem>
#include <functional>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeEventDocumentRuntime;
class NativeEffectDialogTargets;
class NativeEffectTextureDictionary;
class NativeParticleConfiguration;
class IEffectProximityRuntime;
class INativeEffectFactoryLookup;
class NativeNestedEffectDialogs;
class NativePkFileManagerRuntime;
class NativeEffectSupportObjects;
class NativeArenaEffectServices;

// Concrete particle, ThunderObject, CylinderObject, StripObject, BillBordObject and nested
// EffectObject and CameraObject owners/task facets,
// with a composed ClumpObject owner for Attr_Dumy and real models when
// BindModelResources supplies packages and visibility/geometry services.
// BindNestedDialogs supplies the shared embedded Dialog lifecycle;
// stop/destroy nested owners before unbinding or destroying those services.
// Borrows an initialized effect
// system (instance counter at +164) and the application's actual status service.
// Delete all referencing tasks/associations before destroying this factory.
// A supplied registry is borrowed and must outlive the factory. Standalone
// particle probes may omit it to use the static descriptor catalog.
class NativeParticleEffectFactory final : public INativeEffectTaskFacets, public INativeEffectRotationFacets,
                                         public INativeEffectPlaybackFacets,
                                         public INativeEffectSoundFacets,
                                         public INativeEffectCameraFacets,
                                         public INativeEffectMessageFacets,
                                         public INativeEffectRevolutionFacets,
                                         public INativeEffectShadowFacets,
                                         public INativeEffectPositionFacets,
                                         public INativeEffectTrackFacets,
                                         public INativeEffectObjectClassification {
public:
    NativeParticleEffectFactory(NativeBaseObjectMemory&,NativeEventDocumentRuntime&,
        NativeEffectTextureDictionary&,std::uint32_t system,INativeEffectTaskStatus&,
        INativeEffectFactoryLookup* registry=nullptr);
    ~NativeParticleEffectFactory();
    NativeParticleEffectFactory(const NativeParticleEffectFactory&)=delete;
    NativeParticleEffectFactory& operator=(const NativeParticleEffectFactory&)=delete;
    [[nodiscard]] std::uint32_t Create(std::uint32_t document,std::uint32_t owner,std::uint32_t context);
    void Destroy(std::uint32_t object);
    [[nodiscard]] std::size_t LiveObjects() const;
    [[nodiscard]] std::size_t LiveParticleObjects() const;
    void BindNestedDialogs(NativeNestedEffectDialogs&,INativeDialogParticleQueues&);
    void UnbindNestedDialogs();
    void BindModelResources(const std::filesystem::path&,NativePkFileManagerRuntime&,
        NativeEffectSupportObjects&,NativeArenaEffectServices&,
        std::function<NativeArenaEffectServices&(std::uint32_t)> resolver={});
    void UnbindModelResources();
    void BindSound(INativeEffectSound*);
    // Optional ActorManager slot12 override; borrower outlives message tasks.
    void BindOwnerMessages(INativeEffectOwnerMessages*);
    // Bind before creating cameras; targets outlive this factory.
    void BindDialogTargets(NativeEffectDialogTargets&);
    void StartCamera(std::uint32_t) override;
    void StopCamera(std::uint32_t) override;
    std::uint32_t CameraEffectFrame(std::uint32_t) override;
    void StartSound(std::uint32_t,std::uint8_t once) override;
    void StopSound(std::uint32_t) override;
    std::int32_t ReadMessageId(std::uint32_t,std::string_view,std::int32_t) override;
    void SendMessage(std::uint32_t,std::int32_t,std::uint32_t) override;
    void StartNestedEffect(std::uint32_t object,std::uint8_t loop);
    void ClearNestedEffect(std::uint32_t facet);
    void PlayEffect(std::uint32_t facet,std::uint8_t loop) override;
    [[nodiscard]] NativeParticleConfiguration& Configuration(std::uint32_t object);
    std::uint8_t Advance(std::uint32_t object,double delta);
    // Native slot+76 gates visibility before dispatching the tank draw.
    std::uint8_t Draw(std::uint32_t object,std::uint32_t context,
                      std::uint32_t proximity_gate,IEffectProximityRuntime&);
    [[nodiscard]] std::uint32_t Frame(std::uint32_t object);
    [[nodiscard]] double Duration(std::uint32_t object);
    void SetMode(std::uint32_t object,std::uint32_t mode);
    bool ResetPlayback(std::uint32_t object);
    void PreparePlayback(std::uint32_t object);
    void ReleasePlayback(std::uint32_t object,INativeDialogParticleQueues&);
    std::string_view ObjectTypeName(std::uint32_t object) override;
    bool UseFallbackQueue(std::uint32_t object) override;
    bool UseCameraQueue(std::uint32_t object) override;
    bool EffectsEnabled() override;
    bool EffectIsActive(std::uint32_t) override;
    std::uint32_t ResolveFacet(std::uint32_t,std::string_view) override;
    void Show(std::uint32_t) override;
    void Hide(std::uint32_t) override;
    void StopEmission(std::uint32_t) override;
    void StartEmission(std::uint32_t,float,std::uint8_t) override;
    void ReadScale(std::uint32_t,std::array<float,3>&) override;
    void SetScale(std::uint32_t,const std::array<float,3>&,std::uint32_t) override;
    void SetColorFacetEnabled(std::uint32_t,std::uint8_t) override;
    void ReadColor(std::uint32_t,std::array<std::uint8_t,4>&) override;
    void SetColor(std::uint32_t,const std::array<std::uint8_t,4>&) override;
    std::array<float,3> ReadPosition(std::uint32_t) override;
    void Move(std::uint32_t,const std::array<float,3>&,bool relative) override;
    void SetTrackPosition(std::uint32_t,const std::array<float,3>&) override;
    void SetTrackDirection(std::uint32_t,const std::array<float,3>&) override;
    void Face(std::uint32_t,const std::array<float,3>&) override;
    void SetFaceRotation(std::uint32_t,const std::array<float,3>&) override;
    void ActivateFace(std::uint32_t) override;
    std::uint8_t ControlEnabled(std::uint32_t) override;
    void SetControlEnabled(std::uint32_t,std::uint8_t) override;
    void ReadRotation(std::uint32_t,std::array<float,3>&) override;
    void ApplyRotation(std::uint32_t,const std::array<float,3>&,bool relative) override;
    void SetCameraLockedLookAt(std::uint32_t,bool) override;
    void ApplyRevolution(std::uint32_t,const std::array<float,3>&,const std::array<float,6>&) override;
    bool CameraIsActive(std::uint32_t) override;
    bool CameraHasTarget(std::uint32_t) override;
    void ReadCameraTarget(std::uint32_t,std::array<float,3>&) override;
    void PauseEffect(std::uint32_t) override;
    void RewindEffect(std::uint32_t) override;
    float RemainingEffectTime(std::uint32_t) override;
    void StopEffect(std::uint32_t) override;
    // Queue service is borrowed and must outlive referencing shadow tasks.
    // BindNestedDialogs also binds this service for production scene effects.
    void BindShadowQueues(INativeDialogParticleQueues&);
    bool PrepareShadowCount(std::uint32_t,std::int32_t) override;
    std::uint32_t ShadowChild(std::uint32_t,std::int32_t) override;
    std::uint32_t ShadowFrame(std::uint32_t) override;
    void DispatchShadow(std::uint32_t,bool) override;
private:
    void ClearShadowChildren(std::uint32_t);
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
