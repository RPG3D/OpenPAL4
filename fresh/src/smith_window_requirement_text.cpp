#include "pal4/smith_window_requirement_text.h"

#include <cstdint>
#include <string>

namespace pal4 {

std::string SmithWindowRequirementText::Format(
    const std::int32_t prescription_id,
    ISmithWindowRequirementTextRuntime& runtime) {
    runtime.RegisterSmithSystem();
    const auto requirements = runtime.LoadRequirements(prescription_id);
    std::string result;
    for (const auto& requirement : requirements) {
        const std::string format(runtime.DialogTemplate(0xe8u));
        if (runtime.HasItemRecord(requirement.id)) {
            result += ' ';
            result += runtime.FormatRequirement(
                format, runtime.ItemName(requirement.id),requirement.quantity);
        } else if (runtime.HasElementRecord(requirement.id)) {
            result += ' ';
            result += runtime.FormatRequirement(
                format, runtime.ElementName(requirement.id),requirement.quantity);
        }
    }
    return result;
}

}  // namespace pal4
