#pragma once

#include <cstddef>
#include <cstdint>

#include "pal4/package_tea.h"

namespace pal4 {

struct PackageFileBufferState final {
    std::uint8_t loaded = 0;
    std::uint8_t reserved[3]{};
    std::uint32_t data = 0;
    std::uint32_t auxiliary = 0;
    std::uint32_t bytes = 0;
    std::uint32_t unobserved_tail = 0;
};
static_assert(sizeof(PackageFileBufferState) == 20u);
static_assert(offsetof(PackageFileBufferState, data) == 4u);
static_assert(offsetof(PackageFileBufferState, bytes) == 12u);

class IPackageFileBufferRuntime {
public:
    virtual ~IPackageFileBufferRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Read(std::uint32_t handle,
                                             std::uint32_t destination,
                                             std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Decrypt(
        std::uint32_t data, std::uint32_t word_count,
        const char* key) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t data) = 0;
};

class PackageFileBuffer final {
public:
    static constexpr const char* kDecryptionKey = PackageTea::kKey;

    // IDA 0x7932A0.
    [[nodiscard]] static PackageFileBufferState& Initialize(
        PackageFileBufferState& state) noexcept;
    // IDA 0x7932B0.
    [[nodiscard]] static std::uint32_t ReleaseThunk(
        PackageFileBufferState& state, IPackageFileBufferRuntime& runtime);
    // IDA 0x7932C0.
    [[nodiscard]] static std::uint32_t LoadFromHandle(
        PackageFileBufferState& state, std::uint32_t handle,
        std::uint32_t bytes, bool encrypted,
        IPackageFileBufferRuntime& runtime);
    // IDA 0x7933D0.
    [[nodiscard]] static std::uint32_t Release(
        PackageFileBufferState& state, IPackageFileBufferRuntime& runtime);
};

}  // namespace pal4
