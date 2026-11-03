#include "pal4/actor_message_adapter.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class AccessRuntime final : public pal4::IActorMessageAccessRuntime {
public:
    std::vector<std::string> calls;
    std::uint32_t registration_result = 0x7777u;
    pal4::ActorMessageAccessState* state = nullptr;

    std::uint32_t GetSingletonManager() override {
        Check(state != nullptr && (state->guard & 1u) != 0u,
              "guard is published before manager lookup");
        calls.emplace_back("manager");
        return 0x1234u;
    }
    std::uint32_t Register(std::uint32_t manager, std::string_view name,
                           std::uint32_t id, std::uint32_t factory,
                           std::uint32_t destroy) override {
        Check(manager == 0x1234u && name == "palActorMessageAdapter" &&
                  id == 0x3ebu && factory == 0x40ff40u &&
                  destroy == 0x40ffa0u,
              "IDA singleton registration arguments");
        calls.emplace_back("register");
        return registration_result;
    }
};

class ObjectRuntime final : public pal4::IActorMessageAdapterRuntime {
public:
    pal4::ActorMessageAdapter object;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::uint32_t object_address = 0x1000u;
    std::uint32_t sentinel_address = 0x2000u;
    bool throw_in_factory = false;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        if (bytes == 0x2cu) {
            events.emplace_back("allocate-object");
            return object_address;
        }
        Check(bytes == 0x10u, "sentinel allocation size");
        events.emplace_back("allocate-sentinel");
        return sentinel_address;
    }
    pal4::ActorMessageAdapter& Access(std::uint32_t address) override {
        Check(address == object_address, "object address");
        return object;
    }
    void Free(std::uint32_t address) override {
        events.emplace_back("free:" + std::to_string(address));
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t InvokeFactory(std::uint32_t table) override {
        Check(table == object_address, "sentinel uses adapter callback table");
        events.emplace_back("factory");
        if (throw_in_factory) throw std::runtime_error("factory");
        return 0x3000u;
    }
    std::uint32_t InvokeInsert(std::uint32_t, std::uint32_t,
                               std::uint32_t, std::uint32_t) override {
        Check(false, "insertion is not used by this lifecycle");
        return 0u;
    }
    void InvokeObject(std::uint32_t callback,
                      std::uint32_t payload) override {
        Check(callback == object_address + 4u,
              "destruction uses adapter free callback slot");
        events.emplace_back("destroy:" + std::to_string(payload));
    }
};

void RegistrationIsGuardedAndCachesFailure() {
    pal4::ActorMessageAccessState state{0x80u, 0u};
    AccessRuntime runtime;
    runtime.state = &state;
    Check(pal4::ActorMessageAccess::Get(state, runtime) == 0x7777u &&
              state.guard == 0x81u && state.instance == 0x7777u &&
              runtime.calls == std::vector<std::string>{"manager", "register"},
          "first registration and cached result");
    runtime.registration_result = 0u;
    Check(pal4::ActorMessageAccess::Get(state, runtime) == 0x7777u &&
              runtime.calls.size() == 2u,
          "cached registration skips runtime calls");
    state = {};
    runtime.calls.clear();
    Check(pal4::ActorMessageAccess::Get(state, runtime) == 0u &&
              pal4::ActorMessageAccess::Get(state, runtime) == 0u &&
              runtime.calls.size() == 2u,
          "failed registration is also guarded");
}

void ConstructSetsCallbacksAndSentinel() {
    ObjectRuntime runtime;
    auto& state = runtime.object.Collection();
    for (std::uint32_t offset = 0x0cu; offset <= 0x24u; offset += 4u) {
        state.Write(offset, 0xdeadbeefu);
    }
    runtime.object.SetTrailingOwnedObject(0xbeefu);
    Check(pal4::ActorMessageAdapter::Create(runtime) ==
              runtime.object_address,
          "factory returns constructed object");
    Check(state.Read(0u) == 0x40fee0u &&
              state.Read(4u) == 0x5fc6c0u &&
              state.Read(8u) == 0x40feb0u,
          "three callback slots retain IDA targets");
    Check(state.Read(0x0cu) == 0u && state.Read(0x10u) == 0u &&
              state.Read(0x18u) == 0u && state.Read(0x20u) == 0x2000u &&
              state.Read(0x24u) == 0x2000u &&
              runtime.object.TrailingOwnedObject() == 0u,
          "collection and tail are initialized");
    Check(runtime.words.at(0x2000u) == 0x2000u &&
              runtime.words.at(0x2004u) == 0x2000u &&
              runtime.words.at(0x2008u) == 0x3000u &&
              runtime.words.at(0x200cu) == 0x1000u,
          "sentinel is circular and owns factory payload");
    Check(runtime.events == std::vector<std::string>{
              "allocate-object", "allocate-sentinel", "factory"},
          "construction order and empty collection fast path");
}

void AllocationAndConstructorFailuresReleaseStorage() {
    ObjectRuntime runtime;
    runtime.object_address = 0u;
    Check(pal4::ActorMessageAdapter::Create(runtime) == 0u &&
              runtime.events == std::vector<std::string>{"allocate-object"},
          "object allocation failure returns null");
    runtime.object_address = 0x1000u;
    runtime.events.clear();
    runtime.sentinel_address = 0u;
    Check(pal4::ActorMessageAdapter::Create(runtime) ==
              0x1000u && runtime.object.Collection().Read(0x0cu) == 1u,
          "sentinel allocation failure sets collection error but keeps object");
    runtime.sentinel_address = 0x2000u;
    runtime.events.clear();
    runtime.throw_in_factory = true;
    bool threw = false;
    try {
        static_cast<void>(pal4::ActorMessageAdapter::Create(runtime));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.events == std::vector<std::string>{
              "allocate-object", "allocate-sentinel", "factory",
              "free:8192", "free:4096"},
          "constructor exception frees sentinel before object");
}

void DestructionClearsElementsThenStorage() {
    ObjectRuntime runtime;
    static_cast<void>(pal4::ActorMessageAdapter::Create(runtime));
    runtime.events.clear();
    auto& state = runtime.object.Collection();
    state.Write(0x18u, 1u);
    state.Write(0x20u, 0x7777u);
    state.Write(0x10u, 7u);
    runtime.words[0x4004u] = 0x2000u;
    runtime.words[0x4008u] = 0x5000u;
    runtime.words[0x400cu] = 0x1000u;
    runtime.words[0x2004u] = 0x4000u;
    runtime.object.SetTrailingOwnedObject(0x6000u);
    pal4::ActorMessageAdapter::DestroyAndFree(
        runtime.object_address, runtime);
    Check(runtime.events == std::vector<std::string>{
              "destroy:20480", "free:16384", "destroy:12288",
              "free:8192", "free:24576", "free:4096"},
          "element, sentinel, trailing storage, object release order");
    Check(state.Read(0x10u) == 0u && state.Read(0x18u) == 0u &&
              state.Read(0x20u) == 0x2000u &&
              runtime.object.TrailingOwnedObject() == 0u,
          "clear resets counters and trailing storage");
    runtime.events.clear();
    pal4::ActorMessageAdapter::DestroyAndFree(0u, runtime);
    Check(runtime.events.empty(), "null destructor callback is a no-op");
}

void ClearThunkRetainsSentinelAndOuterAllocation() {
    ObjectRuntime runtime;
    static_cast<void>(pal4::ActorMessageAdapter::Create(runtime));
    runtime.events.clear();
    auto& state = runtime.object.Collection();
    state.Write(0x18u, 1u);
    state.Write(0x20u, 0x7777u);
    runtime.words[0x4004u] = 0x2000u;
    runtime.words[0x4008u] = 0x5000u;
    runtime.words[0x400cu] = 0x1000u;
    runtime.words[0x2004u] = 0x4000u;
    pal4::ActorMessageAdapter::Clear(runtime.object_address, runtime);
    Check(runtime.events == std::vector<std::string>{
              "destroy:20480", "free:16384"} &&
              state.Read(0x18u) == 0u && state.Read(0x20u) == 0x2000u &&
              runtime.words.at(0x2000u) == 0x2000u,
          "clear thunk releases elements but retains sentinel and adapter");
}

}  // namespace

int main() {
    RegistrationIsGuardedAndCachesFailure();
    ConstructSetsCallbacksAndSentinel();
    AllocationAndConstructorFailuresReleaseStorage();
    DestructionClearsElementsThenStorage();
    ClearThunkRetainsSentinelAndOuterAllocation();
    std::cout << "5 actor message adapter test cases passed\n";
}
