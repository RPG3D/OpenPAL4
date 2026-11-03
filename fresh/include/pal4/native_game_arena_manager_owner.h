#pragma once

#include "pal4/arena_sequence_resources.h"
#include "pal4/script_bgm_callbacks.h"
#include "pal4/dname_node_event_record.h"
#include "pal4/game_arena_manager_lifecycle.h"
#include "pal4/game_arena_record_compactor.h"
#include "pal4/game_world_object.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_rw_geometry_processor.h"
#include "pal4/game_world_scene_script_loader.h"

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <filesystem>
#include <functional>
#include <string_view>

namespace pal4 {
class NativePkFileManagerRuntime;
class IGameArenaReleaseRuntime;
class IFogConfigFileRuntime;

// Owns the original empty palGameArena image, including its nested members.
// Loaded-world release requires an explicitly scoped, creator-bound scene runtime.
class NativeGameArenaManagerOwner final : private IGameArenaLifecycleRuntime,
                                          private IGameWorldDictionaryRuntime,
                                          private IArenaSequenceRuntime,
                                          private IDNameNodeRuntime,
                                          private IGameArenaRecordMemory,
                                          public IRwGeometryQueueAllocator,
                                          public IScriptBgmAreaName {
public:
    NativeGameArenaManagerOwner() = default;
    NativeGameArenaManagerOwner(const NativeGameArenaManagerOwner&) = delete;
    NativeGameArenaManagerOwner& operator=(
        const NativeGameArenaManagerOwner&) = delete;
    ~NativeGameArenaManagerOwner() override { Destroy(); }

    [[nodiscard]] std::uint32_t Construct();
    void Destroy();
    [[nodiscard]] std::uint32_t Address() const noexcept { return address_; }
    [[nodiscard]] std::uint32_t ReadyFlag() const;
    [[nodiscard]] std::string_view LoadedWorldName() const;
    [[nodiscard]] std::string_view LoadedAreaName() const;
    // IDA51B38A/51B3A6: copy world/area before their owned names are freed.
    void CaptureSceneNames();
    // Original three collision-cache name pairs; preserves this owner's allocator.
    void StoreCollisionCacheText(std::uint32_t address, std::string_view text);
    [[nodiscard]] std::string_view PreviousWorldName() const;
    [[nodiscard]] std::string_view PreviousAreaName() const;
    void SetSceneName(std::string_view name);
    [[nodiscard]] std::string_view SceneName() const override;
    // IDA51A820: publish the original C8/D4 names before deferred SDK push.
    [[nodiscard]] bool QueueSceneScript(
        const std::function<bool(const GameWorldSceneScriptRequest&)>& queue);
    [[nodiscard]] std::string_view SceneScriptSource() const;
    [[nodiscard]] std::string_view SceneScriptPath() const;
    void RenderSequence(std::uint32_t pass);
    // Storage operations use this owner's allocator; payload operations require
    // the same services that created the XML/resource objects.
    void ResetSequenceResources(IArenaSequenceRuntime* payloads = nullptr);
    [[nodiscard]] std::uint32_t AddActiveSceneObject(
        std::uint32_t object, float context);
    // IDA51B2E5..51B362: retire borrowed records before scene owners die.
    void ClearActiveSceneRecords();
    // IDA51B29A..51B2C7: reset/destroy/free the arena-owned FogConfig at +504.
    void ReleaseFogDefaults();
    // IDA518CC5..518E07: optional scene config, owned by this arena.
    [[nodiscard]] bool LoadFogConfig(std::string_view path,
                                     IFogConfigFileRuntime& runtime);
    void BindSceneArchives(NativePkFileManagerRuntime&);
    [[nodiscard]] bool LoadSceneArchive(const std::filesystem::path&);
    void ReleaseSceneArchive();
    // Scoped runtime binding; always restored on success or exception.
    [[nodiscard]] bool ReleaseScene(IGameArenaReleaseRuntime&);
    void StorePreviousSceneText(std::uint32_t field, std::string_view text);
    [[nodiscard]] std::size_t OutstandingAllocations() const noexcept {
        return memory_.OutstandingAllocations();
    }
    [[nodiscard]] std::uint32_t AllocateQueueBytes(
        std::uint32_t bytes) override { return memory_.Allocate(bytes); }
    void FreeQueueBytes(std::uint32_t address) override {
        (void)memory_.Free(address);
    }

private:
    void StoreOwnedText(std::uint32_t offset, std::string_view text);
    [[nodiscard]] std::string_view OwnedText(std::uint32_t offset) const;
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t bytes) override;
    [[nodiscard]] std::uint32_t Free(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t address) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    [[nodiscard]] std::uint32_t Call(
        GameArenaLifecycleCall operation, std::uint32_t address,
        std::uint32_t auxiliary) override;
    [[nodiscard]] std::uint32_t Call(
        ArenaSequenceCall operation,
        std::initializer_list<std::uint32_t> arguments) override;
    void InitializeDictionary(GameWorldObjectState& state,
                              std::uint32_t word_offset) override;
    void CleanupDictionary(GameWorldObjectState& state,
                           std::uint32_t word_offset) override;

    NativeBaseObjectMemory memory_;
    std::uint32_t address_ = 0u;
    std::uint32_t sequence_nil_ = 0u;
    std::uint32_t sequence_references_ = 0u;
    IArenaSequenceRuntime* sequence_payloads_ = nullptr;
    NativePkFileManagerRuntime* scene_archives_ = nullptr;
    IGameArenaReleaseRuntime* release_runtime_ = nullptr;
    bool attempted_ = false;
};

}  // namespace pal4
