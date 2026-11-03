#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IGobClimbRuntime {
public:
    virtual ~IGobClimbRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void Free(std::uint32_t memory) = 0;
    virtual void ConstructBase(std::uint32_t object) = 0;
    virtual void DestructBase(std::uint32_t object) = 0;
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
    [[nodiscard]] virtual std::uint32_t FindXmlChild(
        std::uint32_t parent, std::string_view name) = 0;
    virtual void RemoveXmlChild(std::uint32_t parent,
                                std::uint32_t child) = 0;
    [[nodiscard]] virtual std::uint32_t CreateXmlChild(
        std::uint32_t parent, std::string_view name) = 0;
    virtual void CleanupScriptPackages() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t FindXmlAttribute(
        std::uint32_t node, std::string_view name) = 0;
    [[nodiscard]] virtual float XmlAttributeToFloat(
        std::uint32_t attribute, float fallback) = 0;
    virtual void NormalizeHorizontal(float& x, float& z) = 0;
    [[nodiscard]] virtual float HeadingDegrees(float x, float z) = 0;
    [[nodiscard]] virtual float ClumpFacingDegrees(
        std::uint32_t object) = 0;
    struct Position {
        float x;
        float y;
        float z;
    };
    [[nodiscard]] virtual std::uint32_t BeginActorMotion(
        const Position& near_end, const Position& far_end,
        float heading_degrees, std::uint32_t side) = 0;
    [[nodiscard]] virtual float InteractionThreshold(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::string ReadCString(
        std::uint32_t address) = 0;
    virtual void ConstructSnapshot(std::uint32_t snapshot) = 0;
    virtual void AssignSnapshotName(std::uint32_t snapshot,
                                    std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t SnapshotType(
        std::uint32_t snapshot) = 0;
    virtual void SetBaseVisible(std::uint32_t object,
                                std::uint32_t visible) = 0;
};

class GobClimb final {
public:
    static constexpr std::uint32_t kSize = 0x1dcu;
    static constexpr std::uint32_t kVtable = 0x848b60u;

    [[nodiscard]] static std::uint32_t TypeKey();
    [[nodiscard]] static std::uint32_t Create(
        IGobClimbRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IGobClimbRuntime& runtime);
    static void Destruct(std::uint32_t object,
                         IGobClimbRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeSceneAndConfig(
        std::uint32_t object, std::uint32_t first,
        std::string_view path, std::uint32_t fourth,
        std::uint32_t fifth, IGobClimbRuntime& runtime);
    using Position = IGobClimbRuntime::Position;
    [[nodiscard]] static std::uint32_t GetConfigNode(
        std::uint32_t object, IGobClimbRuntime& runtime);
    [[nodiscard]] static std::uint32_t BeginMotionFromConfig(
        std::uint32_t object, const Position& actor_position,
        IGobClimbRuntime& runtime);
    [[nodiscard]] static bool IsWithinEndpointRange(
        std::uint32_t object, const Position& actor_position,
        float* squared_distance, IGobClimbRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateStateSnapshot(
        std::uint32_t object, IGobClimbRuntime& runtime);
    static void ApplyStateSnapshot(std::uint32_t object,
                                   std::uint32_t snapshot,
                                   IGobClimbRuntime& runtime);

private:
    [[nodiscard]] static bool ReadEndpoints(
        std::uint32_t object, Position& a, Position& b,
        IGobClimbRuntime& runtime);
};

}  // namespace pal4
