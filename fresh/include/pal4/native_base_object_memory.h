#pragma once

#include "pal4/d3d_face_path.h"
#include "pal4/game_config_state.h"
#include "pal4/string_buffer.h"

#include <cstddef>
#include <cstdint>
#include <unordered_set>
#include <vector>

namespace pal4 {

// Owns game-facing buffers through the linked target dlmalloc boundary.
// Host C++ service objects and allocation tracking retain the host CRT.
class NativeBaseObjectMemory final : public IStringBufferStorageRuntime,
                                     public IGameConfigMemory {
public:
    class PathRuntime final : public ID3dFacePathRuntime {
    public:
        explicit PathRuntime(NativeBaseObjectMemory& owner) : owner_(owner) {}
        [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;
        void WriteWord(std::uint32_t address, std::uint32_t value) override;
        [[nodiscard]] std::uint32_t Allocate(std::uint32_t size) override;
        void Free(std::uint32_t address) override;
    private:
        NativeBaseObjectMemory& owner_;
    };

    NativeBaseObjectMemory() : path_(*this) {}
    NativeBaseObjectMemory(const NativeBaseObjectMemory&) = delete;
    NativeBaseObjectMemory& operator=(const NativeBaseObjectMemory&) = delete;
    ~NativeBaseObjectMemory() override;

    [[nodiscard]] std::uint32_t Allocate(std::uint32_t size) override;
    [[nodiscard]] std::uint32_t Free(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t ReadDword(std::uint32_t address) override;
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t address) const;
    void WriteDword(std::uint32_t address, std::uint32_t value) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    [[nodiscard]] PathRuntime& Path() noexcept { return path_; }
    [[nodiscard]] std::size_t OutstandingAllocations() const noexcept {
        return allocations_.size();
    }
    // Stable diagnostic snapshot for ownership checks across restore/release.
    [[nodiscard]] std::vector<std::uint32_t> AllocationAddresses() const;

private:
    PathRuntime path_;
    std::unordered_set<std::uint32_t> allocations_;
};

}  // namespace pal4
