#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IGobSnapshotRuntime;

class IGobMarkerConstructRuntime {
public:
    virtual ~IGobMarkerConstructRuntime() = default;
    virtual void ConstructBase(std::uint32_t object) = 0;
    virtual void DestructBase(std::uint32_t object) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class IGobMarkerSnapshotCaptureRuntime {
public:
    virtual ~IGobMarkerSnapshotCaptureRuntime() = default;
    virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual std::string_view ReadCString(std::uint32_t address) = 0;
    virtual IGobSnapshotRuntime& SnapshotRuntime() = 0;
};

class IGobMarkerRuntime : public IGobMarkerConstructRuntime,
                          public IGobMarkerSnapshotCaptureRuntime {
public:
    virtual ~IGobMarkerRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void ConstructBase(std::uint32_t object) = 0;
    virtual void DestructBase(std::uint32_t object) = 0;
    virtual void CleanupBeforeSceneLoad(std::uint32_t object) = 0;
    [[nodiscard]] virtual bool UsesObjectPackage(std::string_view path) = 0;
    [[nodiscard]] virtual bool UsesWeaponPackage(std::string_view path) = 0;
    virtual void LoadObjectPackage() = 0;
    virtual void LoadWeaponPackage() = 0;
    [[nodiscard]] virtual std::uint32_t CurrentScenePackage() = 0;
    virtual void LoadScenePackage(std::uint32_t package) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeBaseModel(
        std::uint32_t object, std::uint32_t first,
        std::string_view path, std::uint32_t fourth,
        std::uint32_t fifth) = 0;
    virtual void CleanupScriptPackages() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::string_view ReadCString(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual IGobSnapshotRuntime& SnapshotRuntime() = 0;
};

class GobMarker final {
public:
    static constexpr std::uint32_t kSize = 0x1dcu;
    static constexpr std::uint32_t kVtable = 0x849030u;

    [[nodiscard]] static std::uint32_t Create(IGobMarkerRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IGobMarkerConstructRuntime& runtime);
    [[nodiscard]] static std::uint32_t TypeKey();
    static void Destruct(std::uint32_t object, IGobMarkerConstructRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeSceneAndConfig(
        std::uint32_t object, std::uint32_t first,
        std::string_view path, std::uint32_t fourth,
        std::uint32_t fifth, IGobMarkerRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateStateSnapshot(
        std::uint32_t object, IGobMarkerSnapshotCaptureRuntime& runtime);
};

}  // namespace pal4
