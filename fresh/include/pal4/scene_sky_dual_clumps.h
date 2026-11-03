#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct SkyClumpLayer final {
    std::uint32_t clump = 0u;
    std::uint32_t first_matrix = 0u;
    std::uint32_t second_matrix = 0u;
    std::array<float, 3> first_translation{};
    std::array<float, 3> second_translation{};
    std::uint32_t active = 0u;
};

struct SceneSkyDualState final {
    std::uint32_t vtable = 0u;
    SkyClumpLayer first;
    SkyClumpLayer second;
};

static_assert(sizeof(SkyClumpLayer) == 40);
static_assert(sizeof(SceneSkyDualState) == 84);
static_assert(offsetof(SceneSkyDualState, first) == 4);
static_assert(offsetof(SceneSkyDualState, second) == 44);
static_assert(offsetof(SkyClumpLayer, active) == 36);

struct SkyClumpPipeline final {
    std::uint32_t mode = 0u;
    std::uint32_t texture = 0u;
    std::uint32_t option_one = 0u;
    std::uint32_t option_two = 0u;
    std::uint32_t first_matrix = 0u;
    std::uint32_t second_matrix = 0u;
};

class ISceneSkyDualCleanupRuntime {
public:
    virtual ~ISceneSkyDualCleanupRuntime() = default;
    virtual void SetCleanupPipeline(std::uint32_t clump) = 0;
    virtual void DestroyClump(std::uint32_t clump) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyMatrix(
        std::uint32_t matrix) = 0;
};

class ISceneSkyDualRuntime : public ISceneSkyDualCleanupRuntime {
public:
    [[nodiscard]] virtual std::uint32_t LoadClump(
        std::string_view filename, std::string_view source) = 0;
    virtual void PrepareImage(std::string_view source) = 0;
    [[nodiscard]] virtual std::uint32_t ReadTexture(
        std::string_view filename) = 0;
    virtual void SetTextureFlags(std::uint32_t texture,
                                 std::uint8_t filter,
                                 std::uint8_t addressing) = 0;
    [[nodiscard]] virtual std::uint32_t CreateMatrix() = 0;
    virtual void TranslateMatrix(std::uint32_t matrix,
                                 const std::array<float, 3>& translation,
                                 std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t UpdateMatrix(
        std::uint32_t matrix) = 0;
    virtual void SetClumpPipeline(std::uint32_t clump,
                                  const SkyClumpPipeline& pipeline) = 0;
    virtual void DestroyTexture(std::uint32_t texture) = 0;
    virtual void ClearVertexColor(std::uint32_t clump,
                                  std::uint32_t mask) = 0;
    virtual void SetVertexColor(std::uint32_t clump,
                                std::uint32_t color) = 0;
};

class SceneSkyDualRuntimeStub final : public ISceneSkyDualRuntime {
public:
    void SetCleanupPipeline(std::uint32_t) override {}
    void DestroyClump(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t DestroyMatrix(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t LoadClump(
        std::string_view, std::string_view) override { return 0u; }
    void PrepareImage(std::string_view) override {}
    [[nodiscard]] std::uint32_t ReadTexture(
        std::string_view) override { return 0u; }
    void SetTextureFlags(std::uint32_t, std::uint8_t,
                         std::uint8_t) override {}
    [[nodiscard]] std::uint32_t CreateMatrix() override { return 0u; }
    void TranslateMatrix(std::uint32_t,
                         const std::array<float, 3>&,
                         std::uint32_t) override {}
    [[nodiscard]] std::uint32_t UpdateMatrix(
        std::uint32_t) override { return 0u; }
    void SetClumpPipeline(std::uint32_t,
                          const SkyClumpPipeline&) override {}
    void DestroyTexture(std::uint32_t) override {}
    void ClearVertexColor(std::uint32_t, std::uint32_t) override {}
    void SetVertexColor(std::uint32_t, std::uint32_t) override {}
};

class SceneSkyDualClumps final {
public:
    static constexpr std::uint32_t kVtable = 0x84a004u;

    // IDA 0x5fba70.
    [[nodiscard]] static SceneSkyDualState& Initialize(
        SceneSkyDualState& state);
    // IDA 0x5fbac0.
    [[nodiscard]] static std::uint32_t Destroy(
        SceneSkyDualState& state, ISceneSkyDualCleanupRuntime& runtime);
    // IDA 0x5fbad0.
    [[nodiscard]] static bool Load(
        SceneSkyDualState& state, std::string_view source,
        std::string_view first_model, std::string_view first_texture,
        std::string_view second_model, std::string_view second_texture,
        std::uint32_t option_one, std::uint32_t option_two,
        ISceneSkyDualRuntime& runtime);
    // IDA 0x5fbe60.
    [[nodiscard]] static std::uint32_t Cleanup(
        SceneSkyDualState& state, ISceneSkyDualCleanupRuntime& runtime);
    // IDA 0x5fbec0.
    [[nodiscard]] static std::uint32_t UpdateMatrices(
        SceneSkyDualState& state, float value,
        ISceneSkyDualRuntime& runtime);
};

}  // namespace pal4
