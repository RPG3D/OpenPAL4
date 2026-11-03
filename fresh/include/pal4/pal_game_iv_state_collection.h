#pragma once

#include <cstdint>
#include <functional>
#include <string_view>

namespace pal4 {

class IPalGameIVStateCollectionRuntime {
public:
    virtual ~IPalGameIVStateCollectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual bool EqualsString(std::uint32_t record,
                                             std::string_view text) = 0;
    virtual void ConstructString(std::uint32_t record,
                                 std::string_view text) = 0;
    virtual void CopyString(std::uint32_t destination,
                            std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupString(
        std::uint32_t record) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupInputTemporary(
        std::string_view text) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyCleanupResult() = 0;
    virtual void InvokeCallback(std::uint32_t callback,
                                std::uint32_t string_data,
                                std::uint32_t argument) = 0;
};

class PalGameIVStateCollection final {
public:
    [[nodiscard]] static std::uint32_t AddOrUpdate(
        std::uint32_t game, std::string_view text,
        IPalGameIVStateCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t ForEach(
        std::uint32_t game, std::uint32_t callback,
        std::uint32_t argument, IPalGameIVStateCollectionRuntime& runtime);
    // Typed callback for the same5B6A40 traversal. The string pointer is
    // borrowed only during the callback; collection ownership stays external.
    [[nodiscard]] static std::uint32_t Visit(std::uint32_t game,
        const std::function<void(std::uint32_t)>& callback,
        IPalGameIVStateCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t Clear(
        std::uint32_t game, IPalGameIVStateCollectionRuntime& runtime);
};

}  // namespace pal4
