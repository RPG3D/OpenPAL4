#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

struct NpcManagerAccessState {
    std::uint8_t guard{};
    std::uint32_t instance{};
};

class INpcManagerSingletonRuntime {
public:
    virtual ~INpcManagerSingletonRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t Register(
        std::uint32_t manager, std::string_view name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t type_info) = 0;
};

class NpcManagerSingletonRuntimeStub : public INpcManagerSingletonRuntime {
public:
    std::uint32_t SingletonManager() override;
    std::uint32_t Register(std::uint32_t, std::string_view,
                           std::uint32_t, std::uint32_t,
                           std::uint32_t) override;
};

class NpcManagerAccess final {
public:
    static constexpr std::uint32_t kGuardAddress = 0x8e431cu;
    static constexpr std::uint32_t kInstanceAddress = 0x8e4318u;
    static constexpr std::uint32_t kSingletonId = 0x410u;
    static constexpr std::uint32_t kFactoryAddress = 0x43c310u;
    static constexpr std::uint32_t kTypeInfoAddress = 0x7de5d0u;

    // IDA 0x43c2d0: registers once, then returns the cached instance.
    [[nodiscard]] static std::uint32_t Get(
        NpcManagerAccessState& state,
        INpcManagerSingletonRuntime& runtime);
};

struct NpcManagerTreeGlobals {
    std::uint32_t instance_count{};
    std::uint32_t nil_address{};
};

class INpcManagerObjectRuntime {
public:
    virtual ~INpcManagerObjectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    virtual void InitializeTreeBase(std::uint32_t object) = 0;
    virtual void InitializeGrid(std::uint32_t grid) = 0;
    virtual void CleanupGrid(std::uint32_t grid) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupTreeBase(
        std::uint32_t object) = 0;
    virtual void CleanupTreeNodes(std::uint32_t tree,
                                  std::uint32_t root) = 0;
    virtual void ResetTreeBegin(std::uint32_t tree) = 0;
    [[nodiscard]] virtual std::uint32_t NextTreeNode(
        std::uint32_t node) = 0;
    virtual void DeleteTreeNode(std::uint32_t tree,
                                std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
};

class NpcManagerObjectRuntimeStub : public INpcManagerObjectRuntime {
public:
    std::uint32_t Allocate(std::uint32_t) override;
    std::uint32_t Free(std::uint32_t) override;
    void InitializeTreeBase(std::uint32_t) override;
    void InitializeGrid(std::uint32_t) override;
    void CleanupGrid(std::uint32_t) override;
    std::uint32_t CleanupTreeBase(std::uint32_t) override;
    void CleanupTreeNodes(std::uint32_t, std::uint32_t) override;
    void ResetTreeBegin(std::uint32_t) override;
    std::uint32_t NextTreeNode(std::uint32_t) override;
    void DeleteTreeNode(std::uint32_t, std::uint32_t) override;
    std::uint32_t ReadDword(std::uint32_t) override;
    void WriteDword(std::uint32_t, std::uint32_t) override;
    void WriteByte(std::uint32_t, std::uint8_t) override;
};

class NpcManagerObject final {
public:
    static constexpr std::uint32_t kObjectBytes = 0x60u;
    static constexpr std::uint32_t kTreeNodeBytes = 0x24u;
    static constexpr std::uint32_t kVtableAddress = 0x842e64u;
    static constexpr std::uint32_t kTreeCountAddress = 0x8e4320u;
    static constexpr std::uint32_t kTreeNilAddress = 0x8e4324u;

    // IDA 0x43c310: allocate the manager and call its constructor.
    [[nodiscard]] static std::uint32_t Create(
        NpcManagerTreeGlobals& globals,
        std::uint8_t incoming_stack_byte,
        INpcManagerObjectRuntime& runtime);
    // IDA 0x43c370: initialize tree, grid and vtable.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, NpcManagerTreeGlobals& globals,
        std::uint8_t incoming_stack_byte,
        INpcManagerObjectRuntime& runtime);
    // IDA 0x43c470: destroy the tree subobject at manager +0x28.
    [[nodiscard]] static std::uint32_t DestroyTree(
        std::uint32_t tree, NpcManagerTreeGlobals& globals,
        INpcManagerObjectRuntime& runtime);
    // IDA 0x43c530: virtual destructor with grid and base cleanup.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, NpcManagerTreeGlobals& globals,
        INpcManagerObjectRuntime& runtime);
};

class INpcManagerActionRuntime {
public:
    virtual ~INpcManagerActionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void CreateNpc(
        std::uint32_t manager, std::uint32_t name,
        std::uint32_t resource, std::uint32_t position,
        std::uint32_t rotation, std::uint32_t record,
        std::uint32_t enabled) = 0;
};

class NpcManagerActionRuntimeStub : public INpcManagerActionRuntime {
public:
    std::uint32_t ReadDword(std::uint32_t) override;
    void CreateNpc(std::uint32_t, std::uint32_t,
                   std::uint32_t, std::uint32_t,
                   std::uint32_t, std::uint32_t,
                   std::uint32_t) override;
};

class NpcManagerActionCallback final {
public:
    // IDA 0x43c650: create an NPC from one npcInfo record.
    [[nodiscard]] static std::uint32_t Process(
        std::uint32_t record, NpcManagerAccessState& access,
        INpcManagerSingletonRuntime& singleton_runtime,
        INpcManagerActionRuntime& action_runtime);
};

class INpcManagerInitializeRuntime {
public:
    virtual ~INpcManagerInitializeRuntime() = default;
    virtual void Cleanup(std::uint32_t manager) = 0;
    virtual void LoadInfo(std::uint32_t manager,
                          std::uint32_t first, std::uint32_t second) = 0;
    virtual void InitializeGrid(std::uint32_t grid) = 0;
    virtual void ForEachNpc(std::uint32_t manager,
                            std::uint32_t callback,
                            std::uint32_t context) = 0;
};

class NpcManagerInitializeRuntimeStub : public INpcManagerInitializeRuntime {
public:
    void Cleanup(std::uint32_t) override;
    void LoadInfo(std::uint32_t, std::uint32_t,
                  std::uint32_t) override;
    void InitializeGrid(std::uint32_t) override;
    void ForEachNpc(std::uint32_t, std::uint32_t,
                    std::uint32_t) override;
};

class NpcManagerInitializer final {
public:
    static constexpr std::uint32_t kActionCallbackAddress = 0x43c650u;

    // IDA 0x43c680: cleanup, load npcInfo, initialize grid, walk records.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t manager, std::uint32_t first,
        std::uint32_t second, INpcManagerInitializeRuntime& runtime);
};

class INpcManagerCleanupRuntime : public INpcManagerObjectRuntime {
public:
    virtual void NotifyNpc(std::uint32_t npc) = 0;
    virtual void DestroyNpc(std::uint32_t npc,
                            std::uint32_t delete_mode) = 0;
    virtual void FreeTreeText(std::uint32_t text,
                              std::uint32_t mode) = 0;
    virtual void CleanupComplexTree(std::uint32_t manager) = 0;
    virtual void ClearGrid(std::uint32_t grid) = 0;
    virtual void GetPreviousGridElement(std::uint32_t grid) = 0;
    [[nodiscard]] virtual std::uint32_t ResetGrid(
        std::uint32_t grid) = 0;
};

class NpcManagerCleanupRuntimeStub : public INpcManagerCleanupRuntime {
public:
    std::uint32_t Allocate(std::uint32_t) override;
    std::uint32_t Free(std::uint32_t) override;
    void InitializeTreeBase(std::uint32_t) override;
    void InitializeGrid(std::uint32_t) override;
    void CleanupGrid(std::uint32_t) override;
    std::uint32_t CleanupTreeBase(std::uint32_t) override;
    void CleanupTreeNodes(std::uint32_t, std::uint32_t) override;
    void ResetTreeBegin(std::uint32_t) override;
    std::uint32_t NextTreeNode(std::uint32_t) override;
    void DeleteTreeNode(std::uint32_t, std::uint32_t) override;
    std::uint32_t ReadDword(std::uint32_t) override;
    void WriteDword(std::uint32_t, std::uint32_t) override;
    void WriteByte(std::uint32_t, std::uint8_t) override;
    void NotifyNpc(std::uint32_t) override;
    void DestroyNpc(std::uint32_t, std::uint32_t) override;
    void FreeTreeText(std::uint32_t, std::uint32_t) override;
    void CleanupComplexTree(std::uint32_t) override;
    void ClearGrid(std::uint32_t) override;
    void GetPreviousGridElement(std::uint32_t) override;
    std::uint32_t ResetGrid(std::uint32_t) override;
};

class NpcManagerCleanup final {
public:
    // IDA 0x43c6c0: clear NPC objects, record tree and grid without freeing the header.
    [[nodiscard]] static std::uint32_t Clear(
        std::uint32_t manager, const NpcManagerTreeGlobals& globals,
        INpcManagerCleanupRuntime& runtime);
};

}  // namespace pal4
