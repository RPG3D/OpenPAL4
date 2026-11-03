#pragma once

#include "pal4/pal_script_tree.h"

#include <cstdint>
#include <string>

namespace pal4 {

struct ScriptManagerTreeGlobals final {
    std::uint32_t instance_count{};
    std::uint32_t shared_nil{};
};

class IScriptManagerCoreRuntime : public IPalScriptTreeClearRuntime {
public:
    virtual void InitializePackage(std::uint32_t package) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateMemory(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ReleasePackageBlocks(
        std::uint32_t package) = 0;
    [[nodiscard]] virtual std::string ReadCString(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t OpenPackage(
        std::uint32_t package, const std::string& path) = 0;
    [[nodiscard]] virtual std::uint8_t ClosePackage(
        std::uint32_t package) = 0;
    [[nodiscard]] virtual std::uint32_t PredecessorNode(
        std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t InsertNodeAndRebalance(
        std::uint32_t tree, std::uint32_t child,
        std::uint32_t parent, std::uint32_t key,
        std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t DeleteNode(
        std::uint32_t tree, std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint8_t CloseScriptHandle(
        std::uint32_t package, std::uint32_t handle) = 0;
    virtual void ReadPackageData(void* buffer, std::uint32_t bytes,
                                 std::uint32_t handle) = 0;
    [[nodiscard]] virtual std::int32_t SeekPackage(
        std::uint32_t package, std::uint32_t handle,
        std::int32_t offset, std::int32_t origin) = 0;
};

class ScriptManagerCoreRuntimeStub final : public IScriptManagerCoreRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t) override { return 0; }
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    [[nodiscard]] std::uint32_t SharedNil() override { return 0; }
    void FreeMemory(std::uint32_t) override {}
    void FirstElement(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t TemporaryCursor() override { return 0; }
    [[nodiscard]] std::uint32_t ReleaseMemory(
        std::uint32_t) override { return 0; }
    void InitializePackage(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t AllocateMemory(
        std::uint32_t) override { return 0; }
    [[nodiscard]] std::uint32_t ReleasePackageBlocks(
        std::uint32_t) override { return 0; }
    [[nodiscard]] std::string ReadCString(
        std::uint32_t) override { return {}; }
    [[nodiscard]] std::uint8_t OpenPackage(
        std::uint32_t, const std::string&) override { return 0; }
    [[nodiscard]] std::uint8_t ClosePackage(
        std::uint32_t) override { return 0; }
    [[nodiscard]] std::uint32_t PredecessorNode(
        std::uint32_t) override { return 0; }
    [[nodiscard]] std::uint32_t InsertNodeAndRebalance(
        std::uint32_t, std::uint32_t, std::uint32_t,
        std::uint32_t, std::uint32_t) override { return 0; }
    [[nodiscard]] std::uint32_t DeleteNode(
        std::uint32_t, std::uint32_t) override { return 0; }
    [[nodiscard]] std::uint8_t CloseScriptHandle(
        std::uint32_t, std::uint32_t) override { return 0; }
    void ReadPackageData(void*, std::uint32_t,
                         std::uint32_t) override {}
    [[nodiscard]] std::int32_t SeekPackage(
        std::uint32_t, std::uint32_t,
        std::int32_t, std::int32_t) override { return 0; }
};

class ScriptManagerCore final {
public:
    static constexpr std::uint32_t kPackageOffset = 4u;
    static constexpr std::uint32_t kPackageOpenOffset = 0x1000acu;
    static constexpr std::uint32_t kPackagePathOffset = 0x1000b8u;
    static constexpr std::uint32_t kTreeOffset = 0x1001c0u;
    static constexpr std::uint32_t kTreeHeaderOffset = 0x1001c4u;
    static constexpr std::uint32_t kTreeCountOffset = 0x1001ccu;

    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, ScriptManagerTreeGlobals& globals,
        IScriptManagerCoreRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t object, ScriptManagerTreeGlobals& globals,
        IScriptManagerCoreRuntime& runtime);
    [[nodiscard]] static std::uint8_t Load(
        std::uint32_t object, const char* path, std::uint32_t options,
        IScriptManagerCoreRuntime& runtime);
    [[nodiscard]] static std::uint8_t CleanupPackage(
        std::uint32_t object, IScriptManagerCoreRuntime& runtime);
    [[nodiscard]] static std::uint32_t InsertScript(
        std::uint32_t object, std::uint32_t key,
        std::uint32_t value, IScriptManagerCoreRuntime& runtime);
    [[nodiscard]] static std::uint32_t RemoveScript(
        std::uint32_t object, std::uint32_t key,
        IScriptManagerCoreRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetScript(
        std::uint32_t object, std::uint32_t key,
        IScriptManagerCoreRuntime& runtime);
    [[nodiscard]] static std::uint8_t CloseScript(
        std::uint32_t object, std::uint32_t key,
        IScriptManagerCoreRuntime& runtime);
    [[nodiscard]] static std::uint32_t ReadScript(
        std::uint32_t object, std::uint32_t key,
        void* buffer, std::uint32_t bytes,
        IScriptManagerCoreRuntime& runtime);
    [[nodiscard]] static bool IsScriptLoaded(
        std::uint32_t object, std::uint32_t key,
        IScriptManagerCoreRuntime& runtime);
    [[nodiscard]] static std::int32_t SeekScript(
        std::uint32_t object, std::uint32_t key,
        std::int32_t offset, std::int32_t origin,
        IScriptManagerCoreRuntime& runtime);
};

}  // namespace pal4
