#pragma once
#include "pal4/arena_skill_manager.h"
#include "pal4/effect_handle.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/script_arena_skill_state.h"
#include "pal4/effect_system.h"
#include <array>
#include <functional>
#include <map>
#include <memory>
#include <filesystem>

namespace pal4 {
class NativeArenaSignpostActor;
class NativePalGameIVTextureStorage;
class NativePkFileManagerRuntime;
struct PlayerManagerObjectState;
// The resource creator supplies both shared actor cleanup services and its
// complete deleting destructor. A resource-clump cleanup alone is insufficient.
class INativeArenaSkillActors : public IArenaActorRuntime {
public:
    virtual void DestroySkillActor(std::uint32_t actor) = 0;
};

class NativeArenaSkillManagerOwner final : private IArenaSkillRuntime,
    public IScriptArenaSkillStateRuntime {
public:
    explicit NativeArenaSkillManagerOwner(IEffectHandleContentRuntime& effects);
    ~NativeArenaSkillManagerOwner() override;
    NativeArenaSkillManagerOwner(const NativeArenaSkillManagerOwner&) = delete;
    NativeArenaSkillManagerOwner& operator=(const NativeArenaSkillManagerOwner&) = delete;
    void Clear();
    void Destroy();
    void BindActors(INativeArenaSkillActors* actors);
    void BindSignpostActors(NativePalGameIVTextureStorage&,NativePkFileManagerRuntime&,
        const std::filesystem::path&,PlayerManagerObjectState&);
    void UnbindSignpostActors();
    std::uint32_t ShowSignpost(std::uint32_t player_index);
    void UpdateSignpostFade(float elapsed);
    void UpdateSceneSignposts(float elapsed);
    // Original521080: renders non-null actors in the first record array only.
    [[nodiscard]] std::uint32_t RenderFrame(std::uint32_t camera);
    std::size_t SignpostActorCount() const noexcept {return signposts_.size();}
    using SignpostEffect = std::function<EffectSystemHandle(std::string_view, const std::array<float,3>&)>;
    void BindScene(std::uint32_t arena, SignpostEffect create);
    // Actual521C00 placement effect. Caller transfers the returned live handle
    // into its record or releases it before retiring the borrowed scene.
    [[nodiscard]] EffectSystemHandle CreateSignpostEffect(const std::array<float,3>&);
    void UnbindScene();
    std::uint32_t ArenaSkillManager() override { return manager_; }
    std::uint32_t CleanupPlayerData(std::uint32_t manager) override;
    std::uint32_t UpdatePlayerData(std::uint32_t manager) override;
    std::uint32_t Address() const noexcept { return manager_; }
    // All record/array creators must use the same allocator as their cleanup.
    NativeBaseObjectMemory& Storage() noexcept { return memory_; }
private:
    std::uint32_t ReadWord(std::uint32_t) override;
    void WriteWord(std::uint32_t, std::uint32_t) override;
    void WriteByte(std::uint32_t, std::uint8_t) override;
    std::uint32_t Allocate(std::uint32_t) override;
    std::uint32_t Free(std::uint32_t) override;
    std::uint32_t CreateArenaActor() override;
    std::uint32_t Call(ArenaActorCall, std::initializer_list<std::uint32_t>) override;
    std::uint32_t SkillCall(ArenaSkillCall, std::initializer_list<std::uint32_t>) override;
    std::uint32_t RegisterSingleton(std::string_view, std::uint32_t,
        std::uint32_t, std::uint32_t) override;
    void PlayActionSound(std::string_view) override;
    bool HasRecords();
    void CheckPlayerDataServices(bool restoring);
    void RequireIdle() const;
    bool OwnsActor(std::uint32_t) const;
    NativeBaseObjectMemory memory_;
    IEffectHandleContentRuntime& effects_;
    INativeArenaSkillActors* actors_{};
    std::uint32_t manager_{};
    std::uint32_t arena_{};
    SignpostEffect create_signpost_;
    EffectSystemHandleWords temporary_effect_{};
    NativePalGameIVTextureStorage* game_{};
    NativePkFileManagerRuntime* packages_{};
    PlayerManagerObjectState* players_{};
    std::filesystem::path root_;
    std::map<std::uint32_t,std::unique_ptr<NativeArenaSignpostActor>> signposts_;
    bool busy_{},effect_busy_{},rendering_{};
};
}
