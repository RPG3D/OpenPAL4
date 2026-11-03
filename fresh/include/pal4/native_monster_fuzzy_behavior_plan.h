#pragma once

#include "pal4/binary_xml_config.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace pal4 {

struct NativeMonsterFuzzyAtomic {
    std::int32_t class_id = -1;
    std::int32_t proportion = -1;
    std::int32_t wait_mode = 0;
    float wait_time = -1.0f;
    float expect_time = -1.0f;
};

struct NativeMonsterFuzzyBehaviorPlan {
    std::int32_t group_mode = 3;
    std::vector<NativeMonsterFuzzyAtomic> atomics;

    [[nodiscard]] static std::optional<NativeMonsterFuzzyBehaviorPlan>
    FromDocument(const BinaryXmlConfig& document);
};

}  // namespace pal4
