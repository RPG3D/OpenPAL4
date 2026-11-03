#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

enum class SkillPropertyKind { Basic, Effect };

class ISkillPropertyRegistryRuntime {
public:
    virtual ~ISkillPropertyRegistryRuntime() = default;
    virtual void InitializeCollection(std::uint32_t address) = 0;
    [[nodiscard]] virtual int RegisterCleanup(std::uint32_t address) = 0;
    virtual void BeginTemporaryText() = 0;
    virtual void ReleaseTemporaryText() = 0;
    virtual void AddNamedProperty(SkillPropertyKind kind,
                                  std::string_view name,
                                  std::uint32_t offset) = 0;
    virtual void AddParameterProperty(std::uint32_t text_address,
                                      std::uint32_t offset) = 0;
    virtual void Sort(std::uint32_t collection) = 0;
    virtual void CleanupCollection(std::uint32_t collection) = 0;
    [[nodiscard]] virtual std::uint8_t ProcessEvent(
        std::uint32_t input, std::uint32_t collection) = 0;
};

class SkillPropertyRegistry final {
public:
    static constexpr std::uint32_t kCollectionAddress = 0x8ef210;
    static constexpr std::uint32_t kParameterTextAddress = 0x8ef248;

    // IDA 0x4f2db0: guarded collection setup and skill property registration.
    [[nodiscard]] std::uint32_t Initialize(
        ISkillPropertyRegistryRuntime& runtime);
    // IDA 0x4f37e0: release the static property collection.
    void Cleanup(ISkillPropertyRegistryRuntime& runtime) const;
    // IDA 0x4f37f0: initialize the registry before processing an event.
    [[nodiscard]] std::uint8_t ProcessEvent(
        std::uint32_t input, ISkillPropertyRegistryRuntime& runtime);

private:
    bool collection_initialized_ = false;
    bool properties_initialized_ = false;
};

}  // namespace pal4
