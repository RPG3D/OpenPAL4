#pragma once

#include <cstdint>

namespace pal4 {

class IStandardStreamRuntime {
public:
    virtual ~IStandardStreamRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ConstructNarrow(
        std::uint32_t object_address) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructWide(
        std::uint32_t object_address) = 0;
    [[nodiscard]] virtual int RegisterExit(
        std::uint32_t callback_address) = 0;
    virtual void FlushNarrow(std::uint32_t stream_address) = 0;
    virtual void CleanupWide(std::uint32_t stream_address) = 0;
};

class StandardStreamLifecycle final {
public:
    // 0x419e40: this constructor only returns its own pointer.
    [[nodiscard]] static constexpr std::uint32_t ConstructIostreamInit(
        std::uint32_t address) noexcept { return address; }
    static constexpr std::uint32_t kNarrowObjectAddress = 0x8e3f31u;
    static constexpr std::uint32_t kWideObjectAddress = 0x8e3f30u;
    static constexpr std::uint32_t kNarrowExitAddress = 0x40bbf0u;
    static constexpr std::uint32_t kWideExitAddress = 0x40bc30u;

    StandardStreamLifecycle(std::uint32_t& narrow_count,
                            std::uint32_t& wide_count)
        : narrow_count_(narrow_count), wide_count_(wide_count) {}

    [[nodiscard]] std::uint32_t ConstructNarrow(
        IStandardStreamRuntime& runtime) const;
    [[nodiscard]] int RegisterNarrowExit(
        IStandardStreamRuntime& runtime) const;
    [[nodiscard]] int InitializeNarrow(
        IStandardStreamRuntime& runtime) const;
    void CleanupNarrow(IStandardStreamRuntime& runtime);

    [[nodiscard]] std::uint32_t ConstructWide(
        IStandardStreamRuntime& runtime) const;
    [[nodiscard]] int RegisterWideExit(
        IStandardStreamRuntime& runtime) const;
    [[nodiscard]] int InitializeWide(
        IStandardStreamRuntime& runtime) const;
    void CleanupWide(IStandardStreamRuntime& runtime);

private:
    std::uint32_t& narrow_count_;
    std::uint32_t& wide_count_;
};

}  // namespace pal4
