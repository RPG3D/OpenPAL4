#pragma once

#include "pal4/input_manager.h"
#include "pal4/comparison_keyboard_budget.h"

namespace pal4 {

class Win32InputRuntime final : public IInputRuntime {
public:
    explicit Win32InputRuntime(bool background_for_tests = false,
                               bool comparison_messages = false)
        : background_for_tests_(background_for_tests || comparison_messages),
          comparison_messages_(comparison_messages) {}
    void ObserveComparisonMessage(std::uint32_t message, std::uint32_t key,
                                  std::uint32_t parameters);
    std::uint32_t ComparisonPendingSamples() const { return comparison_budget_.PendingSamples(); }
    std::uint32_t ComparisonBoundaryReason() const { return comparison_budget_.BoundaryReason(); }
    void ObserveComparisonScene(std::string_view world, std::string_view area, bool active) {
        if(comparison_messages_)comparison_budget_.ObserveScene(world,area,active);
    }
    std::int32_t CreateDirectInput(std::uint32_t instance,
                                   std::uint32_t version,
                                   std::uint32_t interface_id,
                                   std::uint32_t& object) override;
    std::int32_t CreateDevice(std::uint32_t direct_input,
                              std::uint32_t device_id,
                              std::uint32_t& device) override;
    void SetDataFormat(std::uint32_t device,
                       std::uint32_t format_id) override;
    void SetCooperativeLevel(std::uint32_t device, std::uint32_t window,
                             std::uint32_t flags) override;
    std::int32_t AcquireDevice(std::uint32_t device) override;
    std::int32_t UnacquireDevice(std::uint32_t device) override;
    std::uint32_t ReleaseDevice(std::uint32_t device) override;
    std::int32_t GetDeviceState(std::uint32_t device, std::uint32_t size,
                                InputManager& manager,
                                std::uint32_t destination) override;
    std::uint32_t TimeMilliseconds() override;
    void GetCursorPosition(std::int32_t& x, std::int32_t& y) override;
private:
    bool background_for_tests_{};
    bool comparison_messages_{};
    std::array<std::uint8_t, 256> comparison_keys_{};
    ComparisonKeyboardBudget comparison_budget_;
};

}  // namespace pal4
