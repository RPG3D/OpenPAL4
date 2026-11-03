#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct MoveToTargetTaskState final {
    std::array<std::uint32_t, 36u> words{};
};
static_assert(sizeof(MoveToTargetTaskState) == 0x90u);

// Native object, string storage and facet dispatch remain runtime boundaries.
class IMoveToTargetTaskRuntime {
public:
    virtual ~IMoveToTargetTaskRuntime() = default;
    virtual void ConstructBase(MoveToTargetTaskState& state) = 0;
    virtual void DestroyBase(MoveToTargetTaskState& state) = 0;
    [[nodiscard]] virtual std::uint8_t RefCount(std::uint32_t data) = 0;
    virtual void SetRefCount(std::uint32_t data, std::uint8_t count) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual bool ResolveFacet(std::uint32_t object,
                                            std::string_view class_name,
                                            std::uint32_t& facet) = 0;
    [[nodiscard]] virtual std::uint8_t ReadFacetFlag(std::uint32_t facet) = 0;
    virtual void WriteFacetFlag(std::uint32_t facet, std::uint8_t flag) = 0;
    virtual void SetTarget(std::uint32_t facet,
                           const std::array<std::uint32_t, 3u>& value) = 0;
    virtual void SetOrigin(std::uint32_t facet,
                           const std::array<std::uint32_t, 3u>& value) = 0;
    virtual void Enable(std::uint32_t facet, bool enabled) = 0;
    [[nodiscard]] virtual std::uint32_t SetPosition(
        std::uint32_t facet, const std::array<std::uint32_t, 3u>& value,
        bool relative) = 0;
};

struct MoveToTargetStringTemporary final {
    std::array<std::uint32_t, 4u> words{};
};

// XML, string-buffer ownership and CRT random calls for IDA 0x61eeb0.
class IMoveToTargetConfigRuntime {
public:
    virtual ~IMoveToTargetConfigRuntime() = default;
    [[nodiscard]] virtual MoveToTargetStringTemporary ReadTargetName(
        std::uint32_t config, std::string_view key,
        std::uint32_t default_address) = 0;
    virtual void AssignTargetName(
        MoveToTargetTaskState& state,
        const MoveToTargetStringTemporary& temporary) = 0;
    virtual void ReleaseTargetName(
        MoveToTargetStringTemporary& temporary) = 0;
    [[nodiscard]] virtual float ReadFloat(
        std::uint32_t config, std::string_view key,
        float default_value) = 0;
    [[nodiscard]] virtual std::int32_t ReadTreeInt(
        std::uint32_t config, std::string_view key,
        std::int32_t default_value) = 0;
    [[nodiscard]] virtual float ReadTreeFloat(
        std::uint32_t config, std::string_view key,
        float default_value) = 0;
    [[nodiscard]] virtual std::int32_t RandomInt() = 0;
};

// Native facet and RenderWare matrix services used by IDA 0x61f410.
class IMoveToTargetTrajectoryRuntime {
public:
    virtual ~IMoveToTargetTrajectoryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ResolveMoveFacet(
        std::uint32_t object, std::string_view class_name) = 0;
    [[nodiscard]] virtual std::array<float, 3u> ReadFacetPosition(
        std::uint32_t facet) = 0;
    [[nodiscard]] virtual std::array<float, 3u> ReadTargetOffset(
        std::uint32_t target_owner,
        const MoveToTargetTaskState& state) = 0;
    [[nodiscard]] virtual std::uint32_t GetObjectTransform(
        std::uint32_t object) = 0;
    virtual void SetupTransformMatrix(
        std::array<std::uint32_t, 16u>& matrix,
        std::uint32_t transform) = 0;
    virtual void MultiplyOwnerMatrix(
        std::array<std::uint32_t, 16u>& matrix,
        std::uint32_t object) = 0;
    virtual void TransformPosition(
        std::array<float, 3u>& position,
        const std::array<std::uint32_t, 16u>& matrix) = 0;
    [[nodiscard]] virtual float VectorLength(
        const std::array<float, 3u>& vector) = 0;
};

class MoveToTargetTask final {
public:
    static constexpr std::uint32_t kVtable = 0x84aca0u;

    // IDA 0x61ecf0: stack_byte is the native unwritten stack byte at +108.
    static MoveToTargetTaskState& Construct(
        MoveToTargetTaskState& state, std::uint8_t stack_byte,
        IMoveToTargetTaskRuntime& runtime);
    // IDA 0x61ed20/0x61ed50.
    [[nodiscard]] static std::string_view ClassName();
    static MoveToTargetTaskState& Destroy(
        MoveToTargetTaskState& state, IMoveToTargetTaskRuntime& runtime);
    // IDA 0x61eda0/0x61ede0/0x61ee60.
    [[nodiscard]] static std::uint8_t Start(
        MoveToTargetTaskState& state, IMoveToTargetTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Update(
        MoveToTargetTaskState& state, float delta,
        IMoveToTargetTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Stop(
        MoveToTargetTaskState& state, IMoveToTargetTaskRuntime& runtime);
    // IDA 0x61eeb0. Target name, position, rotation and bias configuration.
    [[nodiscard]] static bool LoadConfig(
        MoveToTargetTaskState& state, std::uint32_t config,
        IMoveToTargetConfigRuntime& runtime);
    // IDA 0x61f410. Resolve target, transform it and derive velocity.
    static void InitializeTrajectory(
        MoveToTargetTaskState& state,
        IMoveToTargetTrajectoryRuntime& runtime);
};

}  // namespace pal4
