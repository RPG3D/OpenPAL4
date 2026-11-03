#include "pal4/combat_console_lifecycle.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kConsole = 0x1000u;
constexpr std::uint32_t kHead = 0x2000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatConsoleRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t singleton_calls = 0u;
    std::uint32_t registrations = 0u;
    bool fail_object_allocation = false;
    bool throw_on_head = false;

    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("byte:" + std::to_string(address));
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("word:" + std::to_string(address));
    }
    std::uint32_t Allocate(std::uint32_t bytes_requested) override {
        calls.emplace_back("allocate:" +
                           std::to_string(bytes_requested));
        if (bytes_requested == 0xfcu)
            return fail_object_allocation ? 0u : kConsole;
        Check(bytes_requested == 16u, "base queue sentinel size");
        if (throw_on_head) throw std::runtime_error("head allocation failed");
        return kHead;
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.emplace_back("free:" + std::to_string(address));
        return address + 1u;
    }
    void DeleteAnimation(std::uint32_t) override {}
    void AdvanceAnimation(std::uint32_t, float) override {}
    void CallAnimationEvent(std::uint32_t,
                            std::uint32_t) override {}
    std::uint32_t AnimationFinished(std::uint32_t) override {
        return 0u;
    }
    std::uint32_t EventKey(std::string_view) override {
        calls.emplace_back("key:" + std::to_string(registrations));
        return 0x3000u + registrations;
    }
    std::uint32_t EventRouter() override { return 0x4000u; }
    std::uint32_t Register(std::uint32_t router,
                           std::uint32_t mode,
                           std::uint32_t key,
                           std::uint32_t owner,
                           std::uint32_t callback) override {
        Check(router == 0x4000u && owner == kConsole &&
                  key == 0x3000u + registrations &&
                  mode == (registrations >= 16u && registrations <= 19u ?
                      3u : 2u) && callback != 0u,
              "console event registration parameters");
        calls.emplace_back("register:" +
                           std::to_string(registrations));
        return 0x5000u + registrations++;
    }
    std::uint32_t FindEventGroup(std::uint32_t,
                                 std::uint32_t) override {
        return 0u;
    }
    void ReleaseOwnedSlot(std::uint32_t) override {}
    void FreeNode(std::uint32_t) override {}
    std::uint32_t SingletonManager() override {
        calls.emplace_back("singleton-manager");
        return 0x8000u;
    }
    std::uint32_t RegisterSingleton(
        std::uint32_t manager, std::string_view name,
        std::uint32_t id, std::uint32_t factory,
        std::uint32_t destroy_callback) override {
        Check(manager == 0x8000u && name == "cbCombatConsole" &&
                  id == 0x406u && factory == 0x53ff40u &&
                  destroy_callback == 0x53ffa0u &&
                  (bytes[pal4::CombatConsoleLifecycle::kSingletonGuard] & 1u),
              "singleton identity, callbacks and guard timing");
        ++singleton_calls;
        calls.emplace_back("singleton-register");
        return 0xabcdu;
    }
};

std::size_t Position(const Runtime& runtime, std::string_view call) {
    const auto found = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(found != runtime.calls.end(), "expected call missing");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void SingletonGuardAndFactory() {
    Runtime runtime;
    runtime.bytes[pal4::CombatConsoleLifecycle::kSingletonGuard] = 0x80u;
    Check(pal4::CombatConsoleLifecycle::Initialize(runtime) == 0xabcdu &&
              runtime.singleton_calls == 1u &&
              runtime.bytes[pal4::CombatConsoleLifecycle::kSingletonGuard] ==
                  0x81u &&
              runtime.words[pal4::CombatConsoleLifecycle::kSingletonSlot] ==
                  0xabcdu &&
              Position(runtime, "singleton-manager") <
                  Position(runtime, "singleton-register"),
          "first access registers singleton and stores returned object");
    Check(pal4::CombatConsoleLifecycle::Initialize(runtime) == 0xabcdu &&
              runtime.singleton_calls == 1u,
          "later access reuses singleton slot");

    Runtime allocated;
    Check(pal4::CombatConsoleLifecycle::CreateInstance(allocated) ==
              kConsole &&
              Position(allocated, "allocate:252") <
                  Position(allocated, "allocate:16") &&
              allocated.words[kConsole + 4u] == kHead &&
              allocated.words[kConsole + 60u] == 0x3e4ccccdu &&
              allocated.bytes[kConsole] == 0u &&
              allocated.bytes[kConsole + 12u] == 0u,
          "factory allocates 0xfc object and constructs queue base");
    for (std::uint32_t index = 0u; index < 23u; ++index)
        Check(allocated.words[kConsole + 68u + 8u * index] == 0u &&
                  allocated.words[kConsole + 72u + 8u * index] == 0u,
              "constructor zeroes every handle/key pair");

    Runtime failed;
    failed.fail_object_allocation = true;
    Check(pal4::CombatConsoleLifecycle::CreateInstance(failed) == 0u &&
              failed.calls == std::vector<std::string>{"allocate:252"},
          "object allocation failure returns null");

    Runtime throwing;
    throwing.throw_on_head = true;
    bool caught = false;
    try {
        (void)pal4::CombatConsoleLifecycle::CreateInstance(throwing);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && throwing.calls.back() == "free:4096",
          "failed base construction frees allocated console");
}

void FactoryDestructorAndObjectDestructor() {
    Runtime factory;
    (void)pal4::CombatConsoleLifecycle::Construct(kConsole, factory);
    Check(pal4::CombatConsoleLifecycle::DestroyInstance(
              kConsole, factory) == kConsole + 1u &&
              Position(factory, "free:8192") <
                  Position(factory, "free:4096") &&
              factory.words[kConsole + 4u] == 0u,
          "factory destroy releases queue sentinel then console allocation");
    Check(pal4::CombatConsoleLifecycle::DestroyInstance(0u, factory) == 0u,
          "null factory destroy is defined as zero");

    Runtime object;
    (void)pal4::CombatConsoleLifecycle::Construct(kConsole, object);
    Check(pal4::CombatConsoleLifecycle::SubscribeThunk(
              kConsole, object) == 0x5016u &&
              object.registrations == 23u,
          "subscription thunk forwards to all 23 event bindings");
    Check(pal4::CombatConsoleLifecycle::Destroy(
              kConsole, object) == 0u &&
              object.words[kConsole + 68u] == 0u &&
              object.words[kConsole + 244u] == 0u &&
              object.words[kConsole + 4u] == kHead &&
              std::find(object.calls.begin(), object.calls.end(),
                        "free:8192") == object.calls.end(),
          "object destructor unsubscribes then clears queue without freeing head");

    Runtime thunk;
    (void)pal4::CombatConsoleLifecycle::Construct(kConsole, thunk);
    Check(pal4::CombatConsoleLifecycle::DestroyQueueThunk(
              kConsole, thunk) == kHead + 1u &&
              thunk.words[kConsole + 4u] == 0u,
          "queue destroy jump releases its sentinel and forwards free result");
}

}  // namespace

int main() {
    try {
        SingletonGuardAndFactory();
        FactoryDestructorAndObjectDestructor();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
