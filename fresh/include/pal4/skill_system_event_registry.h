#pragma once

#include "pal4/skill_system_lifecycle.h"

#include <array>
#include <cstdint>

namespace pal4 {

struct SkillEventRegistration {
    std::uint32_t id;
    std::uint32_t callback;
};

class SkillSystemEventRegistry final {
public:
    static constexpr std::uint32_t kLogFileAddress = 0x8b93acu;
    static constexpr std::uint32_t kVectorOffset = 2344u;
    static constexpr std::uint32_t kEventCount = 39u;

    // IDA 0x58a170: initialize event state and register 39 sorted callbacks.
    static void Initialize(std::uint32_t object,
                           ISkillSystemLifecycleRuntime& runtime);

    [[nodiscard]] static const std::array<SkillEventRegistration,
                                          kEventCount>& Entries();
};

}  // namespace pal4
