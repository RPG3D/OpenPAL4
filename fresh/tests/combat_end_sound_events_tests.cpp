#include "pal4/combat_end_sound_events.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IEventSubscriptionRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t next_handle = 0x700u;
    std::uint32_t group = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("word:" + std::to_string(address) + ":" +
                           std::to_string(value));
    }
    std::uint32_t EventKey(std::string_view) override {
        throw std::logic_error("descriptor is already address-backed");
    }
    std::uint32_t EventRouter() override {
        calls.emplace_back("router");
        return 0x2000u;
    }
    std::uint32_t Register(std::uint32_t router, std::uint32_t mode,
                           std::uint32_t key, std::uint32_t owner,
                           std::uint32_t callback) override {
        Check(router == 0x2000u && mode == 2u && owner == 0x1000u,
              "event registration identity");
        calls.emplace_back("register:" + std::to_string(key) + ":" +
                           std::to_string(callback));
        return next_handle++;
    }
    std::uint32_t FindEventGroup(std::uint32_t router,
                                 std::uint32_t key) override {
        Check(router == 0x2000u &&
                  (key == pal4::CombatEndSoundEvents::kFirstDescriptor ||
                   key == pal4::CombatEndSoundEvents::kSecondDescriptor),
              "event group descriptor");
        calls.emplace_back("group:" + std::to_string(key));
        return group;
    }
    void ReleaseOwnedSlot(std::uint32_t slot) override {
        calls.emplace_back("release:" + std::to_string(slot));
    }
    void FreeNode(std::uint32_t node) override {
        calls.emplace_back("free:" + std::to_string(node));
    }
};

void ConstructAndRegister() {
    Runtime runtime;
    Check(pal4::CombatEndSoundEvents::Construct(
              0x1000u, runtime) == 0x1000u &&
              runtime.words[0x1000u] == 0x700u &&
              runtime.words[0x1004u] ==
                  pal4::CombatEndSoundEvents::kFirstDescriptor &&
              runtime.words[0x1008u] == 0x701u &&
              runtime.words[0x100cu] ==
                  pal4::CombatEndSoundEvents::kSecondDescriptor,
          "constructor clears and installs two slots");
    Check(runtime.calls[0] == "word:4096:0" &&
              runtime.calls[1] == "word:4100:0" &&
              runtime.calls[2] == "word:4104:0" &&
              runtime.calls[3] == "word:4108:0" &&
              runtime.calls[4] == "router" &&
              runtime.calls[5] == "register:9145220:5741520" &&
              runtime.calls[6] == "word:4096:1792" &&
              runtime.calls[7] == "word:4100:9145220" &&
              runtime.calls[8] == "router" &&
              runtime.calls[9] == "register:9145228:5741824" &&
              runtime.calls[10] == "word:4108:9145228" &&
              runtime.calls[11] == "word:4104:1793",
          "registers callback addresses and preserves second write order");
}

void RemoveLinkedHandler() {
    Runtime runtime;
    constexpr std::uint32_t owner = 0x1000u;
    constexpr std::uint32_t head = 0x4000u;
    constexpr std::uint32_t node = 0x4100u;
    runtime.words[owner] = 0x700u;
    runtime.words[owner + 4u] =
        pal4::CombatEndSoundEvents::kFirstDescriptor;
    runtime.words[owner + 8u] = 0x701u;
    runtime.words[owner + 12u] =
        pal4::CombatEndSoundEvents::kSecondDescriptor;
    runtime.group = 0x3000u;
    runtime.words[runtime.group + 4u] = head;
    runtime.words[runtime.group + 8u] = 1u;
    runtime.words[head] = node;
    runtime.words[head + 4u] = node;
    runtime.words[node] = head;
    runtime.words[node + 4u] = head;
    runtime.words[node + 12u] = 0x700u;
    pal4::CombatEndSoundEvents::Remove(owner, runtime);
    Check(runtime.words[head] == head &&
              runtime.words[head + 4u] == head &&
              runtime.words[runtime.group + 8u] == 0u &&
              runtime.words[owner] == 0u &&
              runtime.words[owner + 4u] == 0u &&
              runtime.words[owner + 8u] == 0u &&
              runtime.words[owner + 12u] == 0u,
          "matching list node is unlinked and both slots are cleared");
    bool released = false;
    bool freed = false;
    for (const auto& call : runtime.calls) {
        released |= call == "release:16648";
        freed |= call == "free:16640";
    }
    Check(released && freed, "handler payload and node are released");
}

}  // namespace

int main() {
    try {
        ConstructAndRegister();
        RemoveLinkedHandler();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
