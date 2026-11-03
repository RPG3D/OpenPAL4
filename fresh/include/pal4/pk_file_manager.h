#pragma once

#include "pal4/script_manager_core.h"

#include <cstdint>
#include <functional>

namespace pal4 {

class IPkFileManagerRuntime {
public:
    virtual ~IPkFileManagerRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetSingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t RegisterSingleton(
        std::uint32_t singleton, const char* name, std::uint32_t priority,
        std::function<std::uint32_t()> create,
        std::function<std::uint32_t(std::uint32_t)> destroy) = 0;
    [[nodiscard]] virtual IScriptManagerCoreRuntime& Scripts() = 0;
    [[nodiscard]] virtual ScriptManagerTreeGlobals& TreeGlobals() = 0;
    [[nodiscard]] virtual std::uint32_t NativeFilePosition(
        std::uint32_t file) = 0;
    virtual void NativeFileSeek(
        std::uint32_t file, std::int32_t offset,
        std::int32_t origin) = 0;
};

class PkFileManagerRuntimeStub final : public IPkFileManagerRuntime {
public:
    [[nodiscard]] std::uint32_t GetSingletonManager() override { return 0; }
    [[nodiscard]] std::uint32_t RegisterSingleton(
        std::uint32_t, const char*, std::uint32_t,
        std::function<std::uint32_t()>,
        std::function<std::uint32_t(std::uint32_t)>) override { return 0; }
    [[nodiscard]] IScriptManagerCoreRuntime& Scripts() override {
        return scripts_;
    }
    [[nodiscard]] ScriptManagerTreeGlobals& TreeGlobals() override {
        return tree_globals_;
    }
    [[nodiscard]] std::uint32_t NativeFilePosition(
        std::uint32_t) override { return 0; }
    void NativeFileSeek(
        std::uint32_t, std::int32_t,
        std::int32_t) override {}

private:
    ScriptManagerCoreRuntimeStub scripts_{};
    ScriptManagerTreeGlobals tree_globals_{};
};

class PkFileManager final {
public:
    static constexpr std::uint32_t kObjectBytes = 0xb0146cu;
    static constexpr std::uint32_t kFirstScriptOffset = 0x7cu;
    static constexpr std::uint32_t kScriptStride = 0x1001d0u;
    static constexpr std::uint32_t kScriptCount = 11u;
    static constexpr std::uint32_t kGlobalScriptIndex = 2u;

    explicit PkFileManager(IPkFileManagerRuntime& runtime) : runtime_(runtime) {}

    [[nodiscard]] std::uint32_t Init();
    [[nodiscard]] std::uint32_t Create();
    [[nodiscard]] std::uint32_t Destroy(std::uint32_t object);
    [[nodiscard]] std::uint32_t Construct(std::uint32_t object);
    [[nodiscard]] std::uint32_t Cleanup(std::uint32_t object);
    [[nodiscard]] std::uint32_t ProcessFile(
        std::uint32_t object, std::uint32_t file);
    [[nodiscard]] std::uint32_t LoadAllScripts(std::uint32_t object);
    [[nodiscard]] std::uint32_t CleanupAndProcess(std::uint32_t object);
    [[nodiscard]] std::uint32_t InstallScriptCallbacks(
        std::uint32_t object);
    [[nodiscard]] std::uint32_t RestoreFileCallbacks(
        std::uint32_t object);
    [[nodiscard]] std::uint32_t PopFileCallbacks(
        std::uint32_t object);
    [[nodiscard]] std::uint32_t GetLastScriptData(
        std::uint32_t object);
    [[nodiscard]] std::uint32_t AddScriptData(
        std::uint32_t object, std::uint32_t value);
    [[nodiscard]] std::uint32_t RemoveScriptData(
        std::uint32_t object);
    [[nodiscard]] std::uint32_t LoadScript(
        const char* path, std::uint32_t loader);
    [[nodiscard]] std::uint32_t DestroyScript(
        std::uint32_t script);
    [[nodiscard]] std::uint32_t AddAndProcessScript(
        std::uint32_t value);
    [[nodiscard]] std::uint32_t AddPrimaryScriptAndProcess();
    [[nodiscard]] std::uint32_t AddObjectScriptAndProcess();
    [[nodiscard]] std::uint32_t AddGlobalScriptAndProcess();
    [[nodiscard]] std::uint32_t AddSceneScriptAndProcess();
    [[nodiscard]] std::uint32_t AddWeaponScriptAndProcess();
    [[nodiscard]] std::uint32_t AddUiScriptAndProcess();
    [[nodiscard]] std::uint32_t AddEffectScriptAndProcess();
    [[nodiscard]] std::uint32_t AddMatFxScriptAndProcess();
    [[nodiscard]] std::uint32_t Add2dScriptAndProcess();
    [[nodiscard]] std::uint32_t CleanupAllScripts();
    [[nodiscard]] std::uint32_t GetScriptSize(std::uint32_t file);
    // IDA 0x672340. Allocates the next 24-byte callback-record page.
    [[nodiscard]] static std::uint32_t AllocateCallbackPage(
        std::uint32_t array, IScriptManagerCoreRuntime& memory);
    // IDA 0x672490. Releases the final callback-record page.
    [[nodiscard]] static std::uint32_t RemoveCallbackPage(
        std::uint32_t array, IScriptManagerCoreRuntime& memory);
    // IDA 0x672520. Allocates the next 4-byte script-token page.
    [[nodiscard]] static std::uint32_t AllocateScriptDataPage(
        std::uint32_t array, IScriptManagerCoreRuntime& memory);
    // IDA 0x672700. Replaces a paged array's pointer map.
    [[nodiscard]] static std::uint32_t GrowPageMap(
        std::uint32_t array, std::uint32_t capacity,
        IScriptManagerCoreRuntime& memory);
    // IDA 0x672770. Clears a four-word paged-array iterator.
    [[nodiscard]] static std::uint32_t ResetPageIterator(
        std::uint32_t iterator, IScriptManagerCoreRuntime& memory);
    // IDA 0x6727d0/0x672800. Sets a four-word page iterator.
    [[nodiscard]] static std::uint32_t SetCallbackPageIterator(
        std::uint32_t iterator, std::uint32_t current,
        std::uint32_t cursor, IScriptManagerCoreRuntime& memory);
    [[nodiscard]] static std::uint32_t SetScriptDataPageIterator(
        std::uint32_t iterator, std::uint32_t current,
        std::uint32_t cursor, IScriptManagerCoreRuntime& memory);

private:
    [[nodiscard]] static std::uint32_t AllocatePage(
        std::uint32_t array, std::uint32_t page_bytes,
        IScriptManagerCoreRuntime& memory);
    [[nodiscard]] std::uint32_t AddEmbeddedScriptAndProcess(
        std::uint32_t index);
    IPkFileManagerRuntime& runtime_;
    bool initialized_ = false;
    std::uint32_t manager_ = 0;
};

}  // namespace pal4
