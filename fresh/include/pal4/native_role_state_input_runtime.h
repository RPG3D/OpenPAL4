#pragma once
#include "pal4/role_state_window_input_event.h"
#include <functional>
namespace pal4 {
class InputManager;
class NativeInventorySession;
struct PlayerManagerObjectState;
// Borrows live input, inventory and players. DLL visibility and the actual
// frame event dispatcher remain application endpoints and outlive this adapter.
class NativeRoleStateInputRuntime final : public IRoleStateWindowInputRuntime {
public:
    using Event = std::function<void(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)>;
    NativeRoleStateInputRuntime(pal4::InputManager&,NativeInventorySession&,
        PlayerManagerObjectState&,std::uint32_t manager,Event,
        std::function<bool(std::uint32_t,bool)> visible);
    std::uint32_t ReadWord(std::uint32_t) override;
    std::uint32_t InputManager() override;
    std::uint32_t InventoryManager() override;
    void UpdateAndClampCounter(std::uint32_t,std::uint32_t) override;
    std::uint32_t FrameManager() override;
    void ProcessEvent(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) override;
    void UpdatePlayerResource(std::uint32_t,std::uint32_t,std::uint32_t) override;
    bool IsVisible(std::uint32_t,bool) override;
private:
    pal4::InputManager& input_;
    NativeInventorySession& inventory_;
    PlayerManagerObjectState& players_;
    std::uint32_t manager_;
    Event event_;
    std::function<bool(std::uint32_t,bool)> visible_;
};
}
