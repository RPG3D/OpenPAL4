#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

struct ActorManagerRegistryText {
    std::string value;
};

class IActorManagerPropertyRegistryRuntime {
public:
    virtual ~IActorManagerPropertyRegistryRuntime() = default;
    virtual void RegisterCleanup(std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void InitializeText(std::uint32_t address) = 0;
    virtual void InitializeOwnedSlot(std::uint32_t address) = 0;
    virtual void SetText(ActorManagerRegistryText& text,
                         std::string_view value) = 0;
    virtual void AppendEmptyText(ActorManagerRegistryText& text) = 0;
    virtual void AssignText(std::uint32_t address,
                            const ActorManagerRegistryText& text) = 0;
    virtual void ReleaseText(ActorManagerRegistryText& text) = 0;
    virtual void AssignOwnedSlot(std::uint32_t address,
                                 std::uint32_t object) = 0;
    virtual void ReleaseOwnedSlot(std::uint32_t object) = 0;
    virtual void ConstructSetter(std::uint32_t object,
                                 std::uint32_t callback) = 0;
    virtual void PushBack(std::uint32_t collection,
                          std::uint32_t entry) = 0;
    [[nodiscard]] virtual std::uint32_t GameData() = 0;
    [[nodiscard]] virtual std::uint32_t GameObject() = 0;
    virtual void ProcessAll(std::uint32_t collection,
                            std::uint32_t source,
                            std::uint32_t offset) = 0;
    virtual void Sort(std::uint32_t collection) = 0;
    virtual void CleanupCollection(std::uint32_t collection) = 0;
};

class IActorManagerPlayerPropertyRegistryRuntime
    : public IActorManagerPropertyRegistryRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ScriptEventSystem() = 0;
};

class ActorManagerPropertyRegistry final {
public:
    static constexpr std::uint32_t kAddress = 0x8f610cu;
    static constexpr std::uint32_t kCleanup = 0x530f50u;
    static constexpr std::uint32_t kDbKeyVtable = 0x843368u;
    static constexpr std::uint32_t kDbKeyBaseVtable = 0x843380u;
    static constexpr std::uint32_t kUNameVtable = 0x844f64u;
    static constexpr std::uint32_t kUNameBaseVtable = 0x842bbcu;
    static constexpr std::uint32_t kGetterVtable = 0x844f5cu;
    static constexpr std::uint32_t kGetterCallback = 0x530f40u;
    static constexpr std::uint32_t kSetterCallback = 0x530e00u;

    [[nodiscard]] std::uint32_t ForwardInitialize(
        IActorManagerPropertyRegistryRuntime& runtime);
    [[nodiscard]] std::uint32_t Initialize(
        IActorManagerPropertyRegistryRuntime& runtime);
    static void Cleanup(IActorManagerPropertyRegistryRuntime& runtime);

    [[nodiscard]] bool CollectionConstructed() const { return constructed_; }
    [[nodiscard]] bool EntriesInitialized() const { return initialized_; }

private:
    bool constructed_ = false;
    bool initialized_ = false;
};

class ActorManagerPlayerPropertyRegistry final {
public:
    static constexpr std::uint32_t kAddress = 0x8f61acu;
    static constexpr std::uint32_t kCleanup = 0x538010u;
    static constexpr std::uint32_t kDbKeyOffset = 1196u;
    static constexpr std::uint32_t kUNameVtable = 0x845268u;
    static constexpr std::uint32_t kGetterVtable = 0x845260u;

    [[nodiscard]] std::uint32_t ForwardInitialize(
        IActorManagerPlayerPropertyRegistryRuntime& runtime);
    [[nodiscard]] std::uint32_t Initialize(
        IActorManagerPlayerPropertyRegistryRuntime& runtime);
    static void Cleanup(IActorManagerPlayerPropertyRegistryRuntime& runtime);

    [[nodiscard]] bool CollectionConstructed() const { return constructed_; }
    [[nodiscard]] bool EntriesInitialized() const { return initialized_; }

private:
    bool constructed_ = false;
    bool initialized_ = false;
};

}  // namespace pal4
