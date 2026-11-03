#include "pal4/world_manager_event_subscriptions.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kManager = 0x1000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IWorldManagerEventRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint32_t> groups;
    std::vector<std::string> calls;
    std::uint32_t registrations = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write:" + std::to_string(address));
    }
    std::uint32_t EventKey(std::string_view name) override {
        constexpr std::array<std::string_view, 4> names{
            "re_join_scene", "re_leave_scene",
            "me_join_scene", "me_leave_scene"};
        const auto found = std::find(names.begin(), names.end(), name);
        Check(found != names.end(), "event name");
        const auto index = static_cast<std::uint32_t>(found - names.begin());
        calls.emplace_back("key:" + std::string(name));
        return 0x2000u + index;
    }
    std::uint32_t EventRouter() override {
        calls.emplace_back("router");
        return 0x4000u;
    }
    std::uint32_t Subscribe(std::uint32_t router,
                            std::uint32_t mode,
                            std::uint32_t key,
                            std::uint32_t owner,
                            std::uint32_t callback) override {
        Check(router == 0x4000u && mode == 3u &&
                  owner == kManager &&
                  key == 0x2000u + registrations &&
                  callback == (registrations % 2u == 0u ?
                      pal4::WorldManagerEventSubscriptions::kJoinCallback :
                      pal4::WorldManagerEventSubscriptions::kLeaveCallback),
              "subscription arguments and callback");
        calls.emplace_back("subscribe:" + std::to_string(registrations));
        return 0x3000u + registrations++;
    }
    std::uint32_t FindEventGroup(std::uint32_t router,
                                 std::uint32_t key) override {
        Check(router == 0x4000u, "group lookup router");
        calls.emplace_back("find:" + std::to_string(key));
        return groups[key];
    }
    void ReleaseOwnedSlot(std::uint32_t slot) override {
        calls.emplace_back("release:" + std::to_string(slot));
    }
    void FreeNode(std::uint32_t node) override {
        calls.emplace_back("free:" + std::to_string(node));
    }
};

std::size_t Position(const Runtime& runtime, std::string_view name) {
    const auto found = std::find(runtime.calls.begin(), runtime.calls.end(),
                                 name);
    Check(found != runtime.calls.end(), "expected call missing");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void SubscribeFourEvents() {
    Runtime runtime;
    Check(pal4::WorldManagerEventSubscriptions::Subscribe(
              kManager, runtime) == 0x3003u &&
              runtime.registrations == 4u,
          "four event subscriptions return final handle");
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        Check(runtime.words[kManager + 84u + 8u * index] ==
                  0x3000u + index &&
                  runtime.words[kManager + 88u + 8u * index] ==
                      0x2000u + index,
              "handle/key pairs at manager offsets");
    }
    Check(Position(runtime, "key:re_join_scene") <
                  Position(runtime, "subscribe:0") &&
              Position(runtime, "subscribe:0") <
                  Position(runtime, "key:re_leave_scene") &&
              Position(runtime, "subscribe:2") <
                  Position(runtime, "key:me_leave_scene"),
          "event keys and subscriptions follow IDA order");
}

void UnsubscribeListBranches() {
    Runtime runtime;
    runtime.words[kManager + 84u] = 0x3000u;
    runtime.words[kManager + 88u] = 0x2000u;
    runtime.words[kManager + 92u] = 0x3001u;
    runtime.words[kManager + 96u] = 0x2001u;
    runtime.words[kManager + 100u] = 0u;
    runtime.words[kManager + 104u] = 0x7777u;
    runtime.words[kManager + 108u] = 0x3003u;
    runtime.words[kManager + 112u] = 0x2003u;
    runtime.groups[0x2000u] = 0x6000u;
    runtime.words[0x6004u] = 0x5000u;
    runtime.words[0x6008u] = 2u;
    runtime.words[0x5000u] = 0x5100u;
    runtime.words[0x5100u] = 0x5200u;
    runtime.words[0x5104u] = 0x5000u;
    runtime.words[0x510cu] = 0x9999u;
    runtime.words[0x5200u] = 0x5000u;
    runtime.words[0x5204u] = 0x5100u;
    runtime.words[0x520cu] = 0x3000u;
    runtime.words[0x5004u] = 0x5200u;
    runtime.groups[0x2001u] = 0x6100u;
    runtime.words[0x6104u] = 0x5300u;
    runtime.words[0x5300u] = 0x5300u;
    Check(pal4::WorldManagerEventSubscriptions::Unsubscribe(
              kManager, runtime) == 0u &&
              runtime.words[0x5100u] == 0x5000u &&
              runtime.words[0x5004u] == 0x5100u &&
              runtime.words[0x6008u] == 1u,
          "matching second node is unlinked and count decremented");
    Check(runtime.words[kManager + 84u] == 0u &&
              runtime.words[kManager + 88u] == 0u &&
              runtime.words[kManager + 92u] == 0u &&
              runtime.words[kManager + 96u] == 0u &&
              runtime.words[kManager + 104u] == 0x7777u &&
              runtime.words[kManager + 108u] == 0u &&
              runtime.words[kManager + 112u] == 0u,
          "active slots clear while zero-handle key stays untouched");
    Check(Position(runtime, "release:21000") <
                  Position(runtime, "free:20992") &&
              Position(runtime, "free:20992") <
                  Position(runtime, "write:24584"),
          "owned payload releases before node free and count write");
}

void SubscribeReplacesOldHandle() {
    Runtime runtime;
    runtime.words[kManager + 84u] = 0x4444u;
    runtime.words[kManager + 88u] = 0x2000u;
    runtime.groups[0x2000u] = 0x6000u;
    runtime.words[0x6004u] = 0x5000u;
    runtime.words[0x6008u] = 1u;
    runtime.words[0x5000u] = 0x5100u;
    runtime.words[0x5004u] = 0x5100u;
    runtime.words[0x5100u] = 0x5000u;
    runtime.words[0x5104u] = 0x5000u;
    runtime.words[0x510cu] = 0x4444u;
    Check(pal4::WorldManagerEventSubscriptions::Subscribe(
              kManager, runtime) == 0x3003u &&
              runtime.words[0x6008u] == 0u &&
              runtime.words[kManager + 84u] == 0x3000u &&
              Position(runtime, "free:20736") <
                  Position(runtime, "key:re_join_scene"),
          "repeat subscribe removes old node before new registration");
}

}  // namespace

int main() {
    SubscribeFourEvents();
    UnsubscribeListBranches();
    SubscribeReplacesOldHandle();
}
