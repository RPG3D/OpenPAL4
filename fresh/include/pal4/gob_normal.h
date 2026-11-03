#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IGobSnapshotRuntime;

class IGobNormalConstructRuntime {
public:
    virtual ~IGobNormalConstructRuntime() = default;
    virtual void ConstructBase(std::uint32_t object) = 0;
    virtual void DestructBase(std::uint32_t object) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class IGobNormalSnapshotRuntime {
public:
    virtual ~IGobNormalSnapshotRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string_view ReadCString(std::uint32_t address) = 0;
    [[nodiscard]] virtual IGobSnapshotRuntime& SnapshotRuntime() = 0;
    [[nodiscard]] virtual std::uint32_t ValidateSnapshot(std::uint32_t snapshot) = 0;
    [[nodiscard]] virtual std::uint32_t SetVisibility(
        std::uint32_t object, std::uint32_t visible) = 0;
};

class IGobNormalScriptRuntime {
public:
    virtual ~IGobNormalScriptRuntime()=default;
    virtual std::uint32_t ReadWord(std::uint32_t)=0;
    virtual std::string_view ReadCString(std::uint32_t)=0;
    virtual void CleanupScriptPackages() = 0;
    virtual void InitializeScript() = 0;
    [[nodiscard]] virtual std::string CurrentSceneScriptPath() = 0;
    [[nodiscard]] virtual bool ScriptCacheContains(std::string_view key) = 0;
    [[nodiscard]] virtual std::uint32_t CreateScriptInterpreter(
        std::string_view path, std::string_view key,
        std::string_view entry) = 0;
    [[nodiscard]] virtual bool AddScriptModule(std::uint32_t interpreter) = 0;
    virtual void PushStackModule(std::uint32_t interpreter) = 0;
    virtual void ReleaseInterpreter(std::uint32_t interpreter) = 0;
    virtual void ShowScriptNotFound() = 0;
    [[nodiscard]] virtual std::uint32_t CleanupScriptPath() = 0;
    [[nodiscard]] virtual std::uint32_t ConsumeResearchUse(
        std::uint32_t object) = 0;
};

class IGobNormalRuntime : public IGobNormalConstructRuntime,
                         public IGobNormalSnapshotRuntime, public IGobNormalScriptRuntime {
public:
    ~IGobNormalRuntime() override = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) override = 0;
    [[nodiscard]] virtual std::string_view ReadCString(std::uint32_t address) override = 0;
    virtual void CleanupBeforeSceneLoad(std::uint32_t object) = 0;
    [[nodiscard]] virtual bool UsesObjectPackage(std::string_view path) = 0;
    [[nodiscard]] virtual bool UsesWeaponPackage(std::string_view path) = 0;
    virtual void LoadObjectPackage() = 0;
    virtual void LoadWeaponPackage() = 0;
    [[nodiscard]] virtual std::uint32_t CurrentScenePackage() = 0;
    virtual void LoadScenePackage(std::uint32_t package) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeBaseModel(
        std::uint32_t object, std::uint32_t first, std::string_view path,
        std::uint32_t fourth, std::uint32_t fifth) = 0;
    [[nodiscard]] virtual IGobSnapshotRuntime& SnapshotRuntime() override = 0;
    [[nodiscard]] virtual std::uint32_t ValidateSnapshot(
        std::uint32_t snapshot) override = 0;
    [[nodiscard]] virtual std::uint32_t SetVisibility(
        std::uint32_t object, std::uint32_t visible) override = 0;
};

class GobNormal final {
public:
    static constexpr std::uint32_t kSize = 0x1dcu;
    static constexpr std::uint32_t kVtable = 0x8490e8u;

    [[nodiscard]] static std::uint32_t Create(IGobNormalRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IGobNormalConstructRuntime& runtime);
    [[nodiscard]] static std::uint32_t ValidateScript();
    static void Destruct(std::uint32_t object,
                         IGobNormalConstructRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeSceneAndConfig(
        std::uint32_t object, std::uint32_t first, std::string_view path,
        std::uint32_t fourth, std::uint32_t fifth,
        IGobNormalRuntime& runtime);
    [[nodiscard]] static std::uint32_t RunScriptAndConsumeUse(
        std::uint32_t object, IGobNormalScriptRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateStateSnapshot(
        std::uint32_t object, IGobNormalSnapshotRuntime& runtime);
    [[nodiscard]] static std::uint32_t ApplyStateSnapshot(
        std::uint32_t object, std::uint32_t snapshot,
        IGobNormalSnapshotRuntime& runtime);
};

}  // namespace pal4
