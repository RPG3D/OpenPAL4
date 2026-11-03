#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

struct ScriptUniqueCollectionState {
    std::array<std::uint32_t, 4> words{};

    [[nodiscard]] std::uint32_t& At(std::size_t offset) {
        return words[offset / 4u];
    }
    [[nodiscard]] const std::uint32_t& At(std::size_t offset) const {
        return words[offset / 4u];
    }
};
static_assert(sizeof(ScriptUniqueCollectionState) == 16u);

class IScriptUniqueCollectionRuntime {
public:
    virtual ~IScriptUniqueCollectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class ScriptUniqueCollection final {
public:
    explicit ScriptUniqueCollection(IScriptUniqueCollectionRuntime& runtime)
        : runtime_(runtime) {}

    // 0x5eba60: append an absent dword, growing the raw buffer if needed.
    [[nodiscard]] std::uint32_t AddUnique(
        ScriptUniqueCollectionState& state, std::uint32_t value);
    // 0x5ebc10: erase the first matching dword and shift later values left.
    void Remove(ScriptUniqueCollectionState& state, std::uint32_t value);

private:
    IScriptUniqueCollectionRuntime& runtime_;
};

}  // namespace pal4
