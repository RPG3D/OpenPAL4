#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct DecressTaskState final {
    std::array<std::uint32_t, 21u> words{};
};
static_assert(sizeof(DecressTaskState) == 0x54u);

class IDecressTaskRuntime {
public:
    virtual ~IDecressTaskRuntime() = default;
    virtual void ConstructBase(DecressTaskState& state) = 0;
    virtual void DestroyBase(DecressTaskState& state) = 0;
};

class IDecressTaskFacetRuntime {
public:
    virtual ~IDecressTaskFacetRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ResolveFacet(
        std::uint32_t object, std::string_view class_name) = 0;
    virtual void StopFacet(std::uint32_t facet) = 0;
    virtual void StartFacet(std::uint32_t facet, float duration,
                            std::uint8_t increase) = 0;
};

class IDecressTaskConfigRuntime {
public:
    virtual ~IDecressTaskConfigRuntime() = default;
    [[nodiscard]] virtual std::int32_t ReadInt(
        std::uint32_t config, std::string_view key,
        std::int32_t default_value) = 0;
    [[nodiscard]] virtual float ReadFloat(
        std::uint32_t config, std::string_view key,
        float default_value) = 0;
};

class DecressTask final {
public:
    static constexpr std::uint32_t kVtable = 0x84ac20u;

    // IDA 0x61dde0/0x61de10/0x61de40.
    static DecressTaskState& Construct(DecressTaskState& state,
                                       IDecressTaskRuntime& runtime);
    [[nodiscard]] static std::string_view ClassName();
    static DecressTaskState& Destroy(DecressTaskState& state,
                                     IDecressTaskRuntime& runtime);
    // IDA 0x61de50/0x61de90/0x61dea0.
    [[nodiscard]] static std::uint8_t Start(
        DecressTaskState& state, IDecressTaskFacetRuntime& runtime);
    [[nodiscard]] static std::uint8_t Reset(DecressTaskState& state);
    [[nodiscard]] static bool LoadConfig(
        DecressTaskState& state, std::uint32_t config,
        IDecressTaskConfigRuntime& runtime);
};

}  // namespace pal4
