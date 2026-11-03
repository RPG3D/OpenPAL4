#include "pal4/smith_system_access.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct RegistrationRuntime final : pal4::ISmithSystemRegistrationRuntime {
    std::vector<std::string> calls;
    std::uint32_t memory = 0x5000u;
    bool throw_on_construct = false;
    bool throw_on_register = false;

    std::uint32_t RegisterSingleton(
        std::string_view name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t type_info) override {
        Check(name == "palSmithSystem" && id == 1054u &&
              factory == pal4::SmithSystemAccess::kFactory &&
              type_info == pal4::SmithSystemAccess::kTypeInfo,
              "smith singleton registration parameters");
        calls.emplace_back("register");
        if (throw_on_register) throw std::runtime_error("injected registration failure");
        return 0x1234u;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 1u, "singleton object allocation size");
        calls.emplace_back("allocate"); return memory;
    }
    std::uint32_t Construct(std::uint32_t address) override {
        Check(address == 0x5000u, "singleton constructor memory");
        calls.emplace_back("construct");
        if (throw_on_construct) throw std::runtime_error("injected constructor failure");
        return address;
    }
    void Free(std::uint32_t address) override {
        Check(address == 0x5000u, "failed constructor releases allocation");
        calls.emplace_back("free");
    }
};

struct EventRuntime final : pal4::ISmithUiEventRuntime {
    std::vector<std::string> calls;
    std::uint8_t stored = 0u;
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == 0x6000u, "smith flag byte address");
        stored = value;
        calls.emplace_back("write-byte");
    }
    std::uint32_t UIFrameManager() override {
        calls.emplace_back("ui"); return 0x7000u;
    }
    std::uint8_t DispatchEvent(
        std::uint32_t manager, std::uint32_t descriptor,
        std::uint32_t first, std::uint32_t second) override {
        Check(manager == 0x7000u && second == 0u,
              "smith event manager and second word");
        if (descriptor == pal4::SmithUiEvents::kFlagEvent) {
            Check(first <= 1u, "smith flag event first word");
            calls.emplace_back(first ? "flag-on" : "flag-off");
            return first ? 0x51u : 0x52u;
        }
        Check(descriptor == pal4::SmithUiEvents::kStateEvent && first == 1u,
              "smith state event descriptor and first word");
        calls.emplace_back("state"); return 0x53u;
    }
};

void TestRegistration() {
    RegistrationRuntime runtime;
    std::uint8_t guard = 0x80u;
    std::uint32_t cached = 0u;
    Check(pal4::SmithSystemAccess::Register(guard, cached, runtime) == 0x1234u &&
          guard == 0x81u && cached == 0x1234u &&
          runtime.calls == std::vector<std::string>{"register"},
          "registration sets guard and caches result");
    runtime.calls.clear();
    Check(pal4::SmithSystemAccess::Register(guard, cached, runtime) == 0x1234u &&
          runtime.calls.empty(), "guarded registration uses cached pointer");
    runtime.calls.clear();
    Check(pal4::SmithSystemAccess::Create(runtime) == 0x5000u &&
          runtime.calls == std::vector<std::string>{"allocate", "construct"},
          "factory allocates one byte and constructs");
    runtime.calls.clear(); runtime.memory = 0u;
    Check(pal4::SmithSystemAccess::Create(runtime) == 0u &&
          runtime.calls == std::vector<std::string>{"allocate"},
          "allocation failure skips constructor");
    runtime.calls.clear(); runtime.memory = 0x5000u;
    runtime.throw_on_construct = true;
    bool threw = false;
    try {
        static_cast<void>(pal4::SmithSystemAccess::Create(runtime));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.calls == std::vector<std::string>{
              "allocate", "construct", "free"},
          "constructor failure releases allocation");
    runtime.calls.clear(); runtime.throw_on_register = true;
    guard = 0x80u; cached = 0x8888u;
    threw = false;
    try {
        static_cast<void>(pal4::SmithSystemAccess::Register(guard, cached, runtime));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && guard == 0x81u && cached == 0x8888u &&
          runtime.calls == std::vector<std::string>{"register"},
          "guard write precedes failed native registration");
}

void TestEvents() {
    EventRuntime runtime;
    Check(pal4::SmithUiEvents::SetFlag(0x6000u, 0xabu, runtime) == 0x51u &&
          runtime.stored == 0xabu &&
          runtime.calls == std::vector<std::string>{
              "write-byte", "ui", "flag-on"},
          "set flag writes byte before event and forwards low-byte result");
    runtime.calls.clear();
    Check(pal4::SmithUiEvents::ClearFlags(runtime) == 0x52u &&
          runtime.stored == 0xabu &&
          runtime.calls == std::vector<std::string>{"ui", "flag-off"},
          "clear event does not mutate stored flag byte");
    runtime.calls.clear();
    Check(pal4::SmithUiEvents::SetState(0x1234u, runtime) == 0x53u &&
          runtime.calls == std::vector<std::string>{"ui", "state"},
          "state event ignores argument and forwards result");
}

}  // namespace

int main() {
    try {
        TestRegistration();
        TestEvents();
        std::cout << "smith system access tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
