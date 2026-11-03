#include "pal4/role_state_window_button_up.h"
#include "pal4/role_state_window_members.h"

namespace pal4 {
namespace {

class ButtonRuntimeAdapter final : public IRoleStateWindowButtonRuntime {
public:
    ButtonRuntimeAdapter(const std::uint32_t owner,
                         IRoleStateWindowPlayerRuntime& player_runtime,
                         IRoleStateWindowSwapRuntime& swap_runtime)
        : owner_(owner), player_runtime_(player_runtime),
          swap_runtime_(swap_runtime) {}

    std::uint32_t PlayerManager() override {
        return player_runtime_.PlayerManager();
    }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::uint32_t index) override {
        return player_runtime_.PlayerByIndex(manager, index);
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return player_runtime_.ReadWord(address);
    }
    void HandlePlayerSwap(RoleStateWindowState& state,
                          std::uint32_t target_index,
                          std::uint32_t selected_index) override {
        static_cast<void>(RoleStateWindowPlayerSwap::Handle(
            state, owner_, target_index, selected_index,
            player_runtime_, swap_runtime_));
    }
    void SetVisible(std::uint32_t window, bool visible) override {
        player_runtime_.SetVisible(window, visible);
    }
    void SetImage(std::uint32_t window, std::string_view imageset,
                  std::string_view image) override {
        player_runtime_.SetImage(window, imageset, image);
    }
    void ClearImage(std::uint32_t window) override {
        player_runtime_.ClearImage(window);
    }

private:
    std::uint32_t owner_;
    IRoleStateWindowPlayerRuntime& player_runtime_;
    IRoleStateWindowSwapRuntime& swap_runtime_;
};

}  // namespace

bool RoleStateWindowButtonUp::Handle(
    RoleStateWindowState& state, const std::uint32_t event,
    IRoleStateWindowButtonRuntime& runtime) {
    std::uint32_t selected = RoleStateWindowMembers::kCount;
    for (std::uint32_t i = 0u;
         i < RoleStateWindowMembers::kCount; ++i) {
        auto member = RoleStateWindowMembers::Load(state, i);
        if (member.Read(4u) == 2u) selected = i;
        RoleStateWindowSelection::SetButtonImage(member, runtime);
        RoleStateWindowMembers::Store(state, i, member);
    }

    std::uint32_t active = 0u;
    for (std::uint32_t i = 0u;
         i < RoleStateWindowMembers::kCount; ++i) {
        const auto player = runtime.PlayerByIndex(runtime.PlayerManager(), i);
        if (player != 0u &&
            runtime.ReadWord(player + 0xb08u) != 0u)
            ++active;
    }
    if (active < 2u) return true;

    const auto event_window = runtime.ReadWord(event + 8u);
    const auto role_tag = runtime.ReadWord(event_window + 0x118u);
    const auto target = role_tag - 1u;
    if (selected == RoleStateWindowMembers::kCount) {
        auto member = RoleStateWindowMembers::Load(state, target);
        RoleStateWindowSelection::SetHoveredButtonImage(member, runtime);
        RoleStateWindowMembers::Store(state, target, member);
        return true;
    }
    runtime.HandlePlayerSwap(state, target, selected);
    auto member = RoleStateWindowMembers::Load(state, selected);
    RoleStateWindowSelection::SetButtonImage(member, runtime);
    RoleStateWindowMembers::Store(state, selected, member);
    return true;
}

bool RoleStateWindowButtonUp::Handle(
    RoleStateWindowState& state, const std::uint32_t owner,
    const std::uint32_t event,
    IRoleStateWindowPlayerRuntime& player_runtime,
    IRoleStateWindowSwapRuntime& swap_runtime) {
    ButtonRuntimeAdapter adapter(owner, player_runtime, swap_runtime);
    return Handle(state, event, adapter);
}

}  // namespace pal4
