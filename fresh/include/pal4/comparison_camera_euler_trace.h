#pragma once
#include <array>
#include <cstdint>
#include <cstdlib>
#include <type_traits>

namespace pal4 {
// Main-thread observer only. Store the actual quaternion/Euler words without
// floating-point conversion, allocation, arithmetic, or FPU-control writes.
class ComparisonCameraEulerTrace final {
public:
    static constexpr std::uint32_t kCapacity = 128u;
    static constexpr std::uint32_t kMessage = 0x851au;
    struct Entry {
        std::uint64_t sequence;
        std::array<std::uint32_t,4> input;
        std::array<std::uint32_t,3> output;
        std::uint16_t cw_before, cw_after;
    };
    explicit ComparisonCameraEulerTrace(bool enabled = false) noexcept : enabled_(enabled) {}
    bool Enabled() const noexcept { return enabled_; }
    std::uint32_t Count() const noexcept { return count_; }
    std::uint64_t Dropped() const noexcept { return dropped_; }
    std::uint64_t Sequence() const noexcept { return sequence_; }
    const std::array<Entry,kCapacity>& Entries() const noexcept { return entries_; }
    static std::uint16_t CaptureControlWord() noexcept {
        std::uint16_t result{};
#if defined(_MSC_VER) && defined(_M_IX86)
        __asm fnstcw result
#elif defined(__i386__) || defined(__x86_64__)
        __asm__ volatile("fnstcw %0" : "=m"(result));
#endif
        return result;
    }
    void Record(const std::array<std::uint32_t,4>& input,
                const std::array<std::uint32_t,3>& output,
                std::uint16_t before, std::uint16_t after) noexcept {
        if (!enabled_) return;
        const auto sequence = ++sequence_;
        if (count_ == kCapacity) { ++dropped_; return; }
        entries_[count_++] = {sequence, input, output, before, after};
    }
private:
    bool enabled_;
    std::uint32_t count_{};
    std::uint64_t dropped_{}, sequence_{};
    std::array<Entry,kCapacity> entries_{};
};
static_assert(std::is_trivial_v<ComparisonCameraEulerTrace::Entry> &&
              std::is_standard_layout_v<ComparisonCameraEulerTrace::Entry>);
inline ComparisonCameraEulerTrace& GetComparisonCameraEulerTrace() {
    static ComparisonCameraEulerTrace trace(std::getenv("PAL4_COMPARE_SCRIPTED_INPUT") &&
                                           std::getenv("PAL4_COMPARE_ATB_TRACE"));
    return trace;
}
}
