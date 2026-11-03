#pragma once

#include "pal4/native_base_object_memory.h"
#include "pal4/world_loading_state.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace pal4 {

// Owns the 56-byte PALIV_LOADING object and its three native string buffers.
class NativeWorldLoadingStateOwner final : private IWorldLoadingStateRuntime {
public:
    NativeWorldLoadingStateOwner() = default;
    NativeWorldLoadingStateOwner(const NativeWorldLoadingStateOwner&) = delete;
    NativeWorldLoadingStateOwner& operator=(
        const NativeWorldLoadingStateOwner&) = delete;
    ~NativeWorldLoadingStateOwner() override { Destroy(); }

    [[nodiscard]] std::uint32_t Construct();
    void Destroy();
    [[nodiscard]] std::uint32_t Address() const noexcept;
    [[nodiscard]] std::size_t OutstandingAllocations() const noexcept {
        return memory_.OutstandingAllocations();
    }
    void AssignString(std::uint32_t offset, std::string_view text);
    [[nodiscard]] std::string_view String(std::uint32_t offset) const;
    [[nodiscard]] bool Enter(std::uint32_t previous_id,
                             std::uint32_t config,
                             IWorldLoadingConfigRuntime& runtime);
    [[nodiscard]] std::uint32_t Advance(IWorldLoadingFlowRuntime& runtime);
    [[nodiscard]] std::uint32_t Hide(IWorldLoadingFlowRuntime& runtime);

private:
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    void InitializeString(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t CleanupString(
        std::uint32_t address) override;
    void DestroyBase(std::uint32_t object) override;
    static void CheckStringOffset(std::uint32_t offset);

    NativeBaseObjectMemory memory_;
    std::array<std::uint32_t, 14u> image_{};
    bool constructed_ = false;
};

}  // namespace pal4
