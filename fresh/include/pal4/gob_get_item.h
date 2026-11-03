#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IGobGetItemConstructRuntime {
public:
    virtual ~IGobGetItemConstructRuntime() = default;
    virtual void ConstructBase(std::uint32_t object) = 0;
    virtual void DestructBase(std::uint32_t object) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class IGobGetItemSnapshotRuntime {
public:
    virtual ~IGobGetItemSnapshotRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void Free(std::uint32_t memory) = 0;
    [[nodiscard]] virtual std::string ReadCString(std::uint32_t address) = 0;
    virtual void ConstructSnapshot(std::uint32_t snapshot) = 0;
    virtual void AssignSnapshotName(std::uint32_t snapshot,
                                    std::string_view name) = 0;
};

class IGobGetItemStateApplyRuntime {
public:
    virtual ~IGobGetItemStateApplyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t SnapshotType(
        std::uint32_t snapshot) = 0;
    [[nodiscard]] virtual std::uint32_t SetBaseVisible(
        std::uint32_t object, std::uint32_t visible) = 0;
};

class IGobGetItemRuntime : public IGobGetItemConstructRuntime,
                           public IGobGetItemSnapshotRuntime,
                           public IGobGetItemStateApplyRuntime {
public:
    ~IGobGetItemRuntime() override = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) override = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) override = 0;
    virtual void CleanupBeforeSceneLoad(std::uint32_t object) = 0;
    [[nodiscard]] virtual bool UsesObjectPackage(
        std::string_view path) = 0;
    [[nodiscard]] virtual bool UsesWeaponPackage(
        std::string_view path) = 0;
    virtual void LoadObjectPackage() = 0;
    virtual void LoadWeaponPackage() = 0;
    [[nodiscard]] virtual std::uint32_t CurrentScenePackage() = 0;
    virtual void LoadScenePackage(std::uint32_t package) = 0;
    [[nodiscard]] virtual std::uint32_t LoadBaseModel(
        std::uint32_t object, std::uint32_t first,
        std::string_view path, std::uint32_t fourth,
        std::uint32_t fifth) = 0;
    virtual void HideHookUserDataAtomics(std::uint32_t clump) = 0;
    [[nodiscard]] virtual std::uint32_t FindXmlChild(
        std::uint32_t parent, std::string_view name) = 0;
    virtual void RemoveXmlChild(std::uint32_t parent,
                                std::uint32_t child) = 0;
    [[nodiscard]] virtual std::uint32_t CreateXmlChild(
        std::uint32_t parent, std::string_view name) = 0;
    virtual void CleanupScriptPackages() = 0;
    [[nodiscard]] virtual std::uint32_t CreateHintWindow() = 0;
    [[nodiscard]] virtual std::int32_t ReadXmlInt(
        std::uint32_t node, std::string_view key,
        std::int32_t fallback) = 0;
    [[nodiscard]] virtual bool PropertyExists(std::int32_t id) = 0;
    [[nodiscard]] virtual bool EquipmentExists(std::int32_t id) = 0;
    virtual void GrantProperty(std::int32_t id,
                               std::int32_t quantity) = 0;
    virtual void GrantEquipment(std::int32_t id) = 0;
    [[nodiscard]] virtual std::string PropertyName(
        std::int32_t id) = 0;
    [[nodiscard]] virtual std::string EquipmentName(
        std::int32_t id) = 0;
    [[nodiscard]] virtual std::string GameString(
        std::uint32_t id) = 0;
    [[nodiscard]] virtual std::string FormatHint(
        std::string_view format, std::string_view name,
        std::int32_t quantity) = 0;
    virtual void ShowHint(std::uint32_t window,
                          std::string_view text, float x,
                          float y, float scale,
                          std::uint32_t mode,
                          std::uint32_t visible) = 0;
    virtual void PlayHintSound(std::string_view path,
                               std::uint32_t mode) = 0;
    virtual void ShowIdError(std::string_view message,
                             std::string_view title) = 0;
    [[nodiscard]] virtual std::uint32_t ConsumeResearchUse(
        std::uint32_t object) = 0;
};

class GobGetItem final {
public:
    static constexpr std::uint32_t kSize = 0x1dcu;
    static constexpr std::uint32_t kVtable = 0x848dd0u;

    [[nodiscard]] static std::uint32_t TypeKey();
    [[nodiscard]] static std::uint32_t Create(
        IGobGetItemRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IGobGetItemConstructRuntime& runtime);
    static void Destruct(std::uint32_t object,
                         IGobGetItemConstructRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeSceneAndConfig(
        std::uint32_t object, std::uint32_t first,
        std::string_view path, std::uint32_t fourth,
        std::uint32_t fifth, IGobGetItemRuntime& runtime);
    [[nodiscard]] static std::uint32_t TriggerGrantAndShowHint(
        std::uint32_t object, std::uint32_t context,
        IGobGetItemRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetConfigNode(
        std::uint32_t object, IGobGetItemRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateStateSnapshot(
        std::uint32_t object, IGobGetItemSnapshotRuntime& runtime);
    [[nodiscard]] static std::uint32_t ApplyStateSnapshot(
        std::uint32_t object, std::uint32_t snapshot,
        IGobGetItemStateApplyRuntime& runtime);

private:
    static void ShowGrantHint(std::uint32_t window,
                              std::string_view name,
                              std::int32_t quantity,
                              IGobGetItemRuntime& runtime);
};

}  // namespace pal4
