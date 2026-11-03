#pragma once

#include <cstdint>

namespace pal4 {

class IPalGameIVStateRegistryRuntime {
public:
    virtual ~IPalGameIVStateRegistryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t StateId(std::uint32_t state) = 0;
    virtual void DeleteState(std::uint32_t state,
                             std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyCleanupResult() = 0;
};

class PalGameIVStateRegistry final {
public:
    [[nodiscard]] static bool AddEntry(
        std::uint32_t game, std::uint32_t entry,
        IPalGameIVStateRegistryRuntime& runtime);
    [[nodiscard]] static bool ContainsEntry(
        std::uint32_t game, std::uint32_t entry,
        IPalGameIVStateRegistryRuntime& runtime);
    [[nodiscard]] static std::uint32_t FindEntryById(
        std::uint32_t game, std::uint32_t id,
        IPalGameIVStateRegistryRuntime& runtime);
    [[nodiscard]] static std::uint32_t Cleanup(
        std::uint32_t game, IPalGameIVStateRegistryRuntime& runtime);
};

}  // namespace pal4
