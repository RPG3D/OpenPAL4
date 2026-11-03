#pragma once
#ifdef FormatMessage
#undef FormatMessage
#endif

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace pal4 {

struct SmithRequirement {
    std::int32_t id;
    std::int32_t quantity;
};

class ISmithWindowRequirementTextRuntime {
public:
    virtual ~ISmithWindowRequirementTextRuntime() = default;
    virtual void RegisterSmithSystem() = 0;
    [[nodiscard]] virtual std::vector<SmithRequirement> LoadRequirements(
        std::int32_t prescription_id) = 0;
    [[nodiscard]] virtual std::string_view DialogTemplate(
        std::uint32_t id) = 0;
    [[nodiscard]] virtual bool HasItemRecord(std::int32_t id) = 0;
    [[nodiscard]] virtual std::string_view ItemName(std::int32_t id) = 0;
    [[nodiscard]] virtual bool HasElementRecord(std::int32_t id) = 0;
    [[nodiscard]] virtual std::string_view ElementName(std::int32_t id) = 0;
    [[nodiscard]] virtual std::string FormatRequirement(
        std::string_view format, std::string_view name,
        std::int32_t quantity) = 0;
};

class SmithWindowRequirementText final {
public:
    // IDA 0x49ecb0.
    [[nodiscard]] static std::string Format(
        std::int32_t prescription_id,
        ISmithWindowRequirementTextRuntime& runtime);
};

}  // namespace pal4
