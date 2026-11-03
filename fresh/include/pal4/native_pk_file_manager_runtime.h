#pragma once

#include "pal4/native_package_archive.h"
#include "pal4/native_virtual_file_system_runtime.h"
#include "pal4/pk_file_manager.h"
#include "pal4/pk_script_facade.h"

#include <array>
#include <cstdint>
#include <functional>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace pal4 {

enum class NativeWorldArchiveKind { scene, combat };

struct NativeFilePositionApi {
    std::uint32_t (*position)(std::uint32_t file) = nullptr;
    void (*seek)(std::uint32_t file, std::int32_t offset,
                 std::int32_t origin) = nullptr;
};

class NativePkFileManagerRuntime final : public IPackageFileMountRuntime,
                                         private IPkScriptFacadeRuntime,
                                         private IPackageResourceManagerRuntime,
                                         private IPkFileManagerRuntime,
                                         private IScriptManagerCoreRuntime,
                                         private IPalScriptTreeInsertRuntime {
public:
    explicit NativePkFileManagerRuntime(NativeFilePositionApi file_api = {});
    ~NativePkFileManagerRuntime() override;
    NativePkFileManagerRuntime(const NativePkFileManagerRuntime&) = delete;
    NativePkFileManagerRuntime& operator=(const NativePkFileManagerRuntime&) = delete;

    [[nodiscard]] std::uint32_t InitializePackFileManager() override;
    void ProcessPackFiles(std::uint32_t manager,
                          std::uint32_t file_system) override;
    [[nodiscard]] std::uint32_t LoadAllScripts();
    [[nodiscard]] std::uint32_t AddPrimaryScriptAndProcess();
    [[nodiscard]] std::uint32_t AddGlobalScriptAndProcess();
    [[nodiscard]] std::uint32_t AddObjectScriptAndProcess();
    [[nodiscard]] std::uint32_t AddUiScriptAndProcess();
    // Borrowed UI index5 archive, already selected by AddUiScriptAndProcess.
    [[nodiscard]] std::optional<std::vector<std::uint8_t>> ReadUiMember(const char* path);
    [[nodiscard]] std::uint32_t AddSceneScriptAndProcess();
    [[nodiscard]] std::uint32_t AddWeaponScriptAndProcess();
    [[nodiscard]] std::uint32_t AddMatFxScriptAndProcess();
    [[nodiscard]] std::uint32_t OpenMatFxScriptPath(const char* game_path);
    [[nodiscard]] bool CloseMatFxScriptMember(std::uint32_t handle);
    [[nodiscard]] std::uint32_t AddEffectScriptAndProcess();
    [[nodiscard]] std::uint32_t Add2dScriptAndProcess();
    [[nodiscard]] std::uint32_t Open2dScriptPath(const char* game_path);
    [[nodiscard]] bool Close2dScriptMember(std::uint32_t handle);
    [[nodiscard]] std::uint32_t OpenEffectScriptPath(const char* game_path);
    [[nodiscard]] bool CloseEffectScriptMember(std::uint32_t handle);
    [[nodiscard]] std::uint32_t CleanupAllScripts();
    // IDA66F780/66F980: preserve current callbacks while using the original
    // filesystem callbacks, including from an installed CPK/script scope.
    void PushFilesystemAccess();
    void PopFilesystemAccess();
    [[nodiscard]] std::size_t FilesystemAccessDepth() const noexcept {return filesystem_scopes_.size();}
    // Independently owned scene CPK managers (arena+316), separate from the
    // eleven embedded managers. Caller must destroy before this runtime.
    [[nodiscard]] std::uint32_t LoadSceneArchive(const std::filesystem::path&,
        NativeWorldArchiveKind kind = NativeWorldArchiveKind::scene);
    void DestroySceneArchive(std::uint32_t archive);
    [[nodiscard]] std::uint32_t OpenSceneArchiveMember(
        std::uint32_t archive, std::uint32_t key, const char* member);
    // Temporarily install the original file callbacks; restore on return/throw.
    [[nodiscard]] bool WithSceneArchive(std::uint32_t archive,
        const std::function<bool(NativePackageArchive&)>& load);
    [[nodiscard]] std::size_t SceneArchiveCount() const noexcept { return scene_archives_.size(); }
    [[nodiscard]] std::uint32_t OpenScriptMember(
        std::uint32_t script_index, const char* path);
    [[nodiscard]] std::uint32_t OpenGlobalScriptPath(const char* game_path);
    [[nodiscard]] bool CloseGlobalScriptMember(std::uint32_t handle);
    [[nodiscard]] std::uint32_t GetScriptSize(std::uint32_t file);
    void Shutdown();
    [[nodiscard]] IScriptManagerCoreRuntime& Scripts() override { return *this; }

    [[nodiscard]] std::uint32_t ManagerAddress() const { return object_; }
    [[nodiscard]] std::uint32_t BoundFileSystem() const;
    [[nodiscard]] std::uint32_t ScriptInstanceCount() const {
        return tree_globals_.instance_count;
    }
    [[nodiscard]] std::size_t ArchiveCount() const { return archives_.size(); }
    [[nodiscard]] std::size_t AllocationCount() const {
        return allocations_.size();
    }
    [[nodiscard]] std::uint32_t LoadedPackageCount() const;

private:
    [[nodiscard]] std::uint32_t InitializeManager() override;
    [[nodiscard]] std::uint32_t GetScriptManager(
        std::uint32_t manager) override;
    [[nodiscard]] IPackageResourceManagerRuntime& Resources() override {
        return *this;
    }
    [[nodiscard]] bool HasPathRewrite() override;
    void RewritePath(const char* source,
                     std::array<char, 256>& output) override;
    [[nodiscard]] std::uint32_t OpenNativeFile(const char* path) override;
    [[nodiscard]] std::uint8_t NativeFileExists(const char* path) override;
    [[nodiscard]] NativePackageArchive* ActiveArchive();
    [[nodiscard]] std::uint32_t AddScriptAndProcess(
        std::uint32_t (PkFileManager::*add_script)());
    void InstallNativeScriptCallbacks();
    static std::uint32_t __cdecl LoadByNameCallback(
        std::uint32_t unused, std::uint32_t key, const char* path);
    static std::uint32_t __cdecl UnloadCallback(std::uint32_t key);
    static std::uint32_t __cdecl ReadCallback(
        std::uint32_t key, void* buffer, std::uint32_t bytes);
    static std::uint32_t __cdecl IsLoadedCallback(std::uint32_t key);
    static std::uint64_t __cdecl SeekCallback(
        std::uint32_t key, std::int32_t offset, std::uint32_t mode);
    static std::uint32_t __cdecl FileExistsCallback(
        std::uint32_t unused, const char* path);
    static NativePkFileManagerRuntime* callback_owner_;

    [[nodiscard]] std::uint32_t GetSingletonManager() override;
    [[nodiscard]] std::uint32_t RegisterSingleton(
        std::uint32_t singleton, const char* name, std::uint32_t priority,
        std::function<std::uint32_t()> create,
        std::function<std::uint32_t(std::uint32_t)> destroy) override;
    [[nodiscard]] ScriptManagerTreeGlobals& TreeGlobals() override {
        return tree_globals_;
    }
    [[nodiscard]] std::uint32_t NativeFilePosition(std::uint32_t file) override;
    void NativeFileSeek(std::uint32_t file, std::int32_t offset,
                        std::int32_t origin) override;

    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t address) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    [[nodiscard]] std::uint32_t SharedNil() override {
        return tree_globals_.shared_nil;
    }
    void FreeMemory(std::uint32_t address) override;
    void FirstElement(std::uint32_t tree) override;
    [[nodiscard]] std::uint32_t TemporaryCursor() override;
    [[nodiscard]] std::uint32_t ReleaseMemory(std::uint32_t address) override;
    void InitializePackage(std::uint32_t package) override;
    [[nodiscard]] std::uint32_t AllocateMemory(std::uint32_t bytes) override;
    [[nodiscard]] std::uint32_t ReleasePackageBlocks(
        std::uint32_t package) override;
    [[nodiscard]] std::string ReadCString(std::uint32_t address) override;
    [[nodiscard]] std::uint8_t OpenPackage(
        std::uint32_t package, const std::string& path) override;
    [[nodiscard]] std::uint8_t ClosePackage(std::uint32_t package) override;
    [[nodiscard]] std::uint32_t PredecessorNode(std::uint32_t node) override;
    [[nodiscard]] std::uint32_t InsertNodeAndRebalance(
        std::uint32_t tree, std::uint32_t child, std::uint32_t parent,
        std::uint32_t key, std::uint32_t value) override;
    [[nodiscard]] std::uint32_t DeleteNode(
        std::uint32_t tree, std::uint32_t node) override;
    [[nodiscard]] std::uint8_t CloseScriptHandle(
        std::uint32_t package, std::uint32_t handle) override;
    void ReadPackageData(void* buffer, std::uint32_t bytes,
                         std::uint32_t handle) override;
    [[nodiscard]] std::int32_t SeekPackage(
        std::uint32_t package, std::uint32_t handle,
        std::int32_t offset, std::int32_t origin) override;

    ScriptManagerTreeGlobals tree_globals_{};
    std::unordered_map<std::uint32_t, std::unique_ptr<NativePackageArchive>> archives_;
    std::unordered_set<std::uint32_t> allocations_;
    std::unordered_set<std::uint32_t> scene_archives_;
    std::unordered_set<std::uint32_t> combat_archives_;
    std::unordered_set<std::uint32_t> mounted_scene_archives_;
    std::function<std::uint32_t(std::uint32_t)> destroy_;
    std::uint32_t object_ = 0u;
    std::uint32_t cursor_ = 0u;
    NativeFilePositionApi file_api_{};
    std::unique_ptr<PkFileManager> manager_;
    std::vector<std::uint32_t> filesystem_scopes_;
};

}  // namespace pal4
