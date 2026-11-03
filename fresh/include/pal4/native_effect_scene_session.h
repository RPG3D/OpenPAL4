#pragma once
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <array>
#include "pal4/global_data_settings.h"
#include "pal4/effect_system_object_f_queue.h"
#include "pal4/native_dialog_properties.h"

namespace pal4 {
class NativePkFileManagerRuntime;
class NativeScriptEventRouter;
class NativeScriptEffectRuntime;
class IScriptPlayerPositionRuntime;
class INativeGobEffectService;
class NativeEffectXmlObjects;
class NativeEffectHandleContextRuntime;
class IEffectHandleContentRuntime;
class NativePlayerWeaponEffectHost;
class INativePlayerWeaponResources;
class NativeEffectCreation;
class NativeActorControlSession;
class NativeCameraManagerOwner;
class IEffectHandleMutationRuntime;
class IBaseObjectAttachedEffectRuntime;
class NativeArenaSkillManagerOwner;
class INativeEffectSound;
class NativeGameArenaManagerOwner;
class IGameArenaReleaseRuntime;

// Owns the native global effect system and one arena group. Borrowed packages
// and actor services must outlive their use. Destroy before RenderWare shutdown.
// The caller supplies an active camera for Draw and schedules Update separately.
class NativeEffectSceneSession final : public IGlobalDataScaleRuntime {
public:
    NativeEffectSceneSession(const std::filesystem::path&,NativePkFileManagerRuntime&);
    ~NativeEffectSceneSession();
    NativeEffectSceneSession(const NativeEffectSceneSession&)=delete;
    NativeEffectSceneSession& operator=(const NativeEffectSceneSession&)=delete;
    void BindSound(INativeEffectSound*);
    // Install/clear only with drained groups; captured actor owners outlive use.
    void BindDialogFacetReader(NativeDialogProperties::Reader);
    void BindActors(IScriptPlayerPositionRuntime*) noexcept;
    // Borrow the active RwCamera for camera-facing effect updates. Rebind when
    // the camera changes, before Update; the caller retains camera ownership.
    void BindCamera(std::uint32_t camera);
    // True only while the shared callback slot selects the CG frame renderer.
    // Stop restores the normal renderer while retaining reusable CG resources.
    bool HasScreenEffectCapture() const;
    void UpdateScreenEffectFrame(double seconds);
    std::uint32_t BeginScreenEffectFrame();
    void EndScreenEffectFrame();
    void DrawScreenEffectFrame();
    // Borrowed arena must outlive model playback. Clear before unbinding;
    // unbinding also releases pooled model owners before the world is destroyed.
    void BindArena(std::uint32_t);
    void UnbindArena();
    NativeScriptEffectRuntime& Script() noexcept;
    NativeEffectXmlObjects& XmlEffects() noexcept;
    NativeEffectHandleContextRuntime& ContextHandles() noexcept;
    IEffectHandleContentRuntime& ContentHandles() noexcept;
    IEffectHandleMutationRuntime& MutationHandles() noexcept;
    IBaseObjectAttachedEffectRuntime& AttachedEffects() noexcept;
    NativeArenaSkillManagerOwner& ArenaSkills() noexcept;
    std::uint32_t SceneGroupAddress() const noexcept;
    // Returned adapter borrows this session and resources; retire player handles
    // and destroy the adapter before destroying either dependency.
    std::unique_ptr<NativePlayerWeaponEffectHost> CreateWeaponHost(INativePlayerWeaponResources&);
    INativeGobEffectService& GobEffects() noexcept;
    std::uint32_t CreatedGobEffects() const noexcept;
    void Update(double seconds);
    void Draw(std::uint32_t context);
    // Drain active/pending effects at scene leave; retain reusable owners/cache.
    void Clear();
    // Full arena exit522A00 then636540. Unlike Clear, this also drains every
    // registered group (including UI), cache and shared pool. Retire external
    // effect handles first. Then521370 clears owned arena skill records, whose
    // handles may already be stale after the drain. BindArena re-enables the
    // scene group for reuse.
    void ReleaseArenaEffects();
    // Loaded-scene51B29A..51B36E: fog/weather, effects, five borrowed ranges,
    // then skill records. Reuses the shared release order before actors retire.
    void ReleaseArenaEffectsAndRecords(NativeGameArenaManagerOwner&);
    // Borrows the same creating services for the full shared arena sequence.
    std::unique_ptr<IGameArenaReleaseRuntime> CreateArenaReleaseRuntime(NativeGameArenaManagerOwner&);
    // Original arena408/416 handles. The caller keeps their arena alive.
    // Native adapters expose effects, not the original incidental EAX/AL values.
    void SelectWorldWeather(std::int32_t type, NativeActorControlSession&);
    void SyncWorldWeather(NativeCameraManagerOwner&);
    void StopWorldWeather();
    std::uint32_t ActiveCount() const noexcept;
    std::uint32_t PendingCount() const noexcept;
    std::size_t LiveParticles() const;
    // UIFrameManager's first260 bytes are a separate effect group sharing this
    // session's global factory/cache/pool. Bind only zero-initialized storage;
    // unbind before releasing the manager, after scene Dialogs have drained.
    // Pool deletion still needs both queue owners. System leave uses ClearUiEffects;
    // Scene Clear never clears the UI group.
    // ActorManager's separate EffectSystem base. Caller owns zero-initialized
    // first304 bytes in a400-byte manager and an actual520-byte CombatSystem.
    // Owns the effect base and game-object host catalog; ten actor-event
    // subscriptions are a separate borrowed service.
    void BindCombatEffects(std::uint32_t manager,std::uint32_t combat,NativeScriptEventRouter&);
    // Original53E1A0: real camera/base enable, then host CG handler catalog.
    // Requires drained Combat payloads; does not enter PALIV_COMBAT.
    bool InitializeCombatGameObject(std::uint32_t game,std::uint32_t camera);
    void DeinitializeCombatGameObject();
    // Original53E1E0 at battle end owns cleanup of remaining group payloads.
    // Call after battle tasks unwind, with actor/effect borrowers still alive.
    // Cold unbind/reinitialize APIs retain their stricter empty-payload guards.
    void EndCombatGameObject();
    [[nodiscard]] std::size_t CombatScreenEffectHandlerCount() const;
    void UnbindCombatEffects();
    void BindCombatCamera(std::uint32_t camera);
    int UpdateCombatEffects(double seconds);
    void DrawCombatEffects(std::uint32_t context);
    void ClearCombatEffects();
    NativeEffectCreation& CombatEffectCreation();
    //529F90/52A180 hand effects use the ActorManager's context/timed trees.
    //These handles must never be passed to the scene-bound ContextHandles().
    EffectSystemHandleWords LoadCombatXmlEffect(std::string_view name,std::uint32_t clump);
    bool CombatXmlEffectReady(const EffectSystemHandleWords&) const;
    void RemoveCombatXmlEffect(EffectSystemHandleWords&);
    void DrainCombatXmlEffect(EffectSystemHandleWords&);
    std::size_t CombatXmlEffectCount() const;
    [[nodiscard]] std::uint32_t CombatActiveCount() const;
    [[nodiscard]] std::uint32_t CombatTotalCount() const;
    void ReleaseCombatEffect(EffectSystemHandleWords&,bool force=false);
    void SetCombatEffectFacet(const EffectSystemHandleWords&,std::string_view,std::uint32_t facet);
    bool CombatEffectVector(const EffectSystemHandleWords&,std::string_view,
        std::array<float,3>&,const NativeDialogProperties::Reader&);
    void BindUiEffects(std::uint32_t manager);
    void UnbindUiEffects();
    void BindUiCamera(std::uint32_t camera);
    int UpdateUiEffects(double seconds);
    void DrawUiEffects(std::uint32_t context);
    void ClearUiEffects();
    NativeEffectCreation& UiEffectCreation();
    // Native Logo and other UI consumers retain typed handles. Reject handles
    // from other groups before touching their payloads or the shared pool.
    bool UiDialogNeedsUpdate(const EffectSystemHandleWords&) const;
    std::uint32_t UiEffectFrame(const EffectSystemHandleWords&) const;
    std::uint8_t SetUiEffectScale(const EffectSystemHandleWords&, const std::array<float, 3>&, bool additive = false);
    void ReleaseUiEffect(EffectSystemHandleWords&, bool force = false);
    std::uint32_t UiActiveCount() const;
    void SetAtomicPluginFloatData(float value) override;
    bool HasEffectSystem() override;
    std::int32_t SetEffectSystemSquaredValue(float value) override;
    float EffectDistanceSquared() const;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
