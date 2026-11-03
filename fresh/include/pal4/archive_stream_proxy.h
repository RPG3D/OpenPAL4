#pragma once

#include "pal4/archive_stream_adapter.h"

#include <cstddef>
#include <cstdint>

namespace pal4 {

struct ArchiveStreamProxyState {
    std::uint32_t vtable{};
    std::uint32_t adapter{};
};
static_assert(sizeof(ArchiveStreamProxyState) == 8u);
static_assert(offsetof(ArchiveStreamProxyState, adapter) == 4u);

class IArchiveStreamProxyRuntime : public IArchiveStreamRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void FreeMemory(std::uint32_t address) = 0;
};

class ArchiveStreamProxyRuntimeStub final : public IArchiveStreamProxyRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void FreeMemory(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t InvokeVirtual(
        std::uint32_t, std::uint32_t, std::uintptr_t,
        std::uint32_t) override { return 0u; }
};

class ArchiveStreamProxy final {
public:
    static constexpr std::uint32_t kVtable = 0x8496e0u;
    static constexpr std::uint32_t kBaseVtable = 0x8426acu;

    explicit ArchiveStreamProxy(IArchiveStreamProxyRuntime& runtime)
        : runtime_(runtime), adapter_(runtime) {}

    // 0x5ec980: this object's +4 word is a pointer to an adapter.
    ArchiveStreamProxyState& Construct(ArchiveStreamProxyState& state,
                                       std::uint32_t adapter);
    // 0x5ec9a0 is a vtable deleting-destructor code island.
    [[nodiscard]] std::uint32_t Destroy(
        ArchiveStreamProxyState& state, std::uint32_t object,
        std::uint32_t flags);
    // 0x5ec9c0..0x5ec9e0: forward through the adapter's stream pointer.
    [[nodiscard]] std::uint32_t CallSlot0(
        const ArchiveStreamProxyState& state,
        std::uintptr_t arg0 = 0u, std::uint32_t arg1 = 0u);
    [[nodiscard]] std::uint32_t CallSlot4(
        const ArchiveStreamProxyState& state,
        std::uintptr_t arg0 = 0u, std::uint32_t arg1 = 0u);
    [[nodiscard]] std::uint32_t CallSlot8(
        const ArchiveStreamProxyState& state,
        std::uintptr_t arg0 = 0u, std::uint32_t arg1 = 0u);
    // 0x5ec9f0/0x5eca00: tail-forward the caller buffer/count to slot +12/+16.
    [[nodiscard]] std::uint32_t Read(
        const ArchiveStreamProxyState& state, void* buffer,
        std::uint32_t bytes);
    [[nodiscard]] std::uint32_t Write(
        const ArchiveStreamProxyState& state, const void* buffer,
        std::uint32_t bytes);

private:
    [[nodiscard]] std::uint32_t Stream(
        const ArchiveStreamProxyState& state);

    IArchiveStreamProxyRuntime& runtime_;
    ArchiveStreamAdapter adapter_;
};

}  // namespace pal4
