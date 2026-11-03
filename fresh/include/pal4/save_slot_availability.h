#pragma once

#include <cstdint>

namespace pal4 {

class ISaveSlotAvailabilityRuntime {
public:
    virtual ~ISaveSlotAvailabilityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GameDatabase() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
};

class SaveSlotAvailability final {
public:
    [[nodiscard]] static bool IsAvailable(
        std::uint32_t record, ISaveSlotAvailabilityRuntime& runtime);
    [[nodiscard]] static std::uint32_t CountAvailable(
        ISaveSlotAvailabilityRuntime& runtime);
};

}  // namespace pal4
