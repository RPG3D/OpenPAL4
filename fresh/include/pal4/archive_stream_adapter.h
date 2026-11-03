#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

struct ArchiveStreamAdapterState {
    std::uint32_t stream{};
    std::uint8_t mode{};
    std::array<std::uint8_t, 3> padding{};
};
static_assert(sizeof(ArchiveStreamAdapterState) == 8u);
static_assert(offsetof(ArchiveStreamAdapterState, mode) == 4u);

class IArchiveStreamRuntime {
public:
    virtual ~IArchiveStreamRuntime() = default;
    [[nodiscard]] virtual std::uint32_t InvokeVirtual(
        std::uint32_t stream, std::uint32_t slot_offset,
        std::uintptr_t buffer, std::uint32_t bytes) = 0;
};

class ArchiveStreamRuntimeStub final : public IArchiveStreamRuntime {
public:
    [[nodiscard]] std::uint32_t InvokeVirtual(
        std::uint32_t, std::uint32_t, std::uintptr_t,
        std::uint32_t) override { return 0u; }
};

class ArchiveStreamAdapter final {
public:
    explicit ArchiveStreamAdapter(IArchiveStreamRuntime& runtime)
        : runtime_(runtime) {}

    // 0x5ec2f0: retain the stream pointer and low mode byte.
    ArchiveStreamAdapterState& Construct(ArchiveStreamAdapterState& state,
                                         std::uint32_t stream,
                                         std::uint8_t mode);
    // 0x5ec310/0x5ec320: dispatch write/read virtual slots.
    [[nodiscard]] std::uint32_t Write(
        const ArchiveStreamAdapterState& state, const void* buffer,
        std::uint32_t bytes);
    [[nodiscard]] std::uint32_t Read(
        const ArchiveStreamAdapterState& state, void* buffer,
        std::uint32_t bytes);

private:
    IArchiveStreamRuntime& runtime_;
};

}  // namespace pal4
