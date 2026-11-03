#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

struct MonsterManagerAccessState {
    std::uint8_t guard{};
    std::uint32_t instance{};
};

class IMonsterManagerSingletonRuntime {
public:
    virtual ~IMonsterManagerSingletonRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t Register(
        std::uint32_t manager, std::string_view name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t type_info) = 0;
};

class MonsterManagerSingletonRuntimeStub : public IMonsterManagerSingletonRuntime {
public:
    std::uint32_t SingletonManager() override;
    std::uint32_t Register(std::uint32_t, std::string_view,
                           std::uint32_t, std::uint32_t,
                           std::uint32_t) override;
};

class MonsterManagerAccess final {
public:
    static constexpr std::uint32_t kGuardAddress = 0x8e42acu;
    static constexpr std::uint32_t kInstanceAddress = 0x8e42a8u;
    static constexpr std::uint32_t kSingletonId = 0x411u;
    static constexpr std::uint32_t kFactoryAddress = 0x4377b0u;
    static constexpr std::uint32_t kTypeInfoAddress = 0x7de5d0u;

    [[nodiscard]] static std::uint32_t Get(
        MonsterManagerAccessState& state,
        IMonsterManagerSingletonRuntime& runtime);
};

struct MonsterManagerTreeGlobals {
    std::uint32_t instance_count{};
    std::uint32_t nil_address{};
};

class IMonsterManagerObjectRuntime {
public:
    virtual ~IMonsterManagerObjectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    virtual void InitializeTreeBase(std::uint32_t object) = 0;
    virtual void InitializeGrid(std::uint32_t grid) = 0;
    virtual void CleanupGrid(std::uint32_t grid) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupTreeBase(std::uint32_t object) = 0;
    virtual void CleanupTreeNodes(std::uint32_t tree,
                                  std::uint32_t root) = 0;
    virtual void ResetTreeBegin(std::uint32_t tree) = 0;
    [[nodiscard]] virtual std::uint32_t NextTreeNode(
        std::uint32_t node) = 0;
    virtual void DeleteTreeNode(std::uint32_t tree,
                                std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
};

class MonsterManagerObjectRuntimeStub : public IMonsterManagerObjectRuntime {
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

class MonsterManagerObject final {
public:
    static constexpr std::uint32_t kObjectBytes = 0x60u;
    static constexpr std::uint32_t kTreeNodeBytes = 0x24u;
    static constexpr std::uint32_t kVtableAddress = 0x842da0u;
    static constexpr std::uint32_t kTreeCountAddress = 0x8e42b0u;
    static constexpr std::uint32_t kTreeNilAddress = 0x8e42b4u;

    [[nodiscard]] static std::uint32_t Create(
        MonsterManagerTreeGlobals& globals,
        std::uint8_t incoming_stack_byte,
        IMonsterManagerObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, MonsterManagerTreeGlobals& globals,
        std::uint8_t incoming_stack_byte,
        IMonsterManagerObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyTree(
        std::uint32_t tree, MonsterManagerTreeGlobals& globals,
        IMonsterManagerObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, MonsterManagerTreeGlobals& globals,
        IMonsterManagerObjectRuntime& runtime);
};

class IMonsterManagerActionRuntime {
public:
    virtual ~IMonsterManagerActionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void CreateMonster(
        std::uint32_t manager, std::uint32_t name,
        std::uint32_t secondary_name, std::uint32_t position,
        std::uint32_t facing, std::uint32_t record,
        std::uint32_t enabled) = 0;
};

class MonsterManagerActionRuntimeStub : public IMonsterManagerActionRuntime {
public:
    std::uint32_t ReadDword(std::uint32_t) override;
    void CreateMonster(std::uint32_t, std::uint32_t,
                       std::uint32_t, std::uint32_t,
                       std::uint32_t, std::uint32_t,
                       std::uint32_t) override;
};

class MonsterManagerActionCallback final {
public:
    [[nodiscard]] static std::uint32_t Process(
        std::uint32_t record,
        MonsterManagerAccessState& access,
        IMonsterManagerSingletonRuntime& singleton_runtime,
        IMonsterManagerActionRuntime& action_runtime);
};

class IMonsterManagerInitializeRuntime {
public:
    virtual ~IMonsterManagerInitializeRuntime() = default;
    virtual void Cleanup(std::uint32_t manager) = 0;
    virtual void LoadInfo(std::uint32_t manager,
                          std::uint32_t first, std::uint32_t second) = 0;
    virtual void InitializeGrid(std::uint32_t grid) = 0;
    virtual void ForEachMonster(std::uint32_t manager,
                                std::uint32_t callback,
                                std::uint32_t context) = 0;
};

class MonsterManagerInitializeRuntimeStub : public IMonsterManagerInitializeRuntime {
public:
    void Cleanup(std::uint32_t) override;
    void LoadInfo(std::uint32_t, std::uint32_t,
                  std::uint32_t) override;
    void InitializeGrid(std::uint32_t) override;
    void ForEachMonster(std::uint32_t, std::uint32_t,
                        std::uint32_t) override;
};

class MonsterManagerInitializer final {
public:
    static constexpr std::uint32_t kActionCallbackAddress = 0x437af0u;
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t manager, std::uint32_t first,
        std::uint32_t second,
        IMonsterManagerInitializeRuntime& runtime);
};

}  // namespace pal4
