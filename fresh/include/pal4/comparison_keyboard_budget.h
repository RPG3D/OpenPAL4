#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

// Development input transport, shared by the native host and optional original
// observer build. It controls device samples, never simulation time or actors.
class ComparisonKeyboardBudget final {
public:
    static constexpr std::uint32_t kMessage = 0x850au;
    static constexpr std::uint32_t kMaxSamples = 600u;
    bool Configure(std::uint32_t mask, std::uint32_t samples) {
        if ((mask == 0u) != (samples == 0u) || (mask & ~15u) || samples > kMaxSamples)
            return false;
        mask_ = mask;
        remaining_ = samples;
        scene_bound_ = false;
        boundary_reason_ = 0u;
        return true;
    }
    void Cancel() { mask_ = remaining_ = 0u; scene_bound_ = false; }
    std::uint32_t PendingSamples() const { return remaining_; }
    std::uint32_t BoundaryReason() const { return boundary_reason_; }
    void ObserveScene(std::string_view world, std::string_view area, bool active) {
        if (!remaining_) return;
        if (!active) { Cancel(); boundary_reason_ = 1u; return; }
        if (!scene_bound_) {
            world_ = world; area_ = area; scene_bound_ = true;
        } else if (world_ != world || area_ != area) {
            Cancel(); boundary_reason_ = 2u;
        }
    }
    void Apply(std::array<std::uint8_t, 256>& keys) {
        if (!remaining_) return;
        keys.fill(0u);
        constexpr std::array<unsigned, 4> scans{0x11u, 0x1fu, 0x1eu, 0x20u};
        for (unsigned i = 0; i < scans.size(); ++i)
            if (mask_ & (1u << i)) keys[scans[i]] = 0x80u;
        --remaining_;
    }
private:
    std::uint32_t mask_{};
    std::uint32_t remaining_{};
    std::uint32_t boundary_reason_{}; // 1 left Scene, 2 changed world/area.
    bool scene_bound_{};
    std::string world_, area_;
};

} // namespace pal4
