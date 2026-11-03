#include "pal4/combat_event_registry_callbacks.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void EveryIdaCallback() {
    using Callbacks = pal4::CombatEventRegistryCallbacks;
    Check(Callbacks::Resolve(0x583080u) == &Callbacks::NullSub4 &&
              Callbacks::Resolve(0x583540u) == &Callbacks::NullSub80,
          "first and last native addresses map to matching functions");
    for (std::uint32_t index = 0u; index < Callbacks::kCount; ++index) {
        const auto callback = Callbacks::Resolve(
            Callbacks::kFirstAddress + index * 16u);
        Check(callback != nullptr,
              "every single-byte IDA callback has a C++ function");
        callback();
    }
    Check(Callbacks::Resolve(0x58307fu) == nullptr &&
              Callbacks::Resolve(0x583081u) == nullptr &&
              Callbacks::Resolve(0x583550u) == nullptr,
          "misaligned and out-of-range addresses do not resolve");
}

}  // namespace

int main() {
    try {
        EveryIdaCallback();
        std::cout << "combat event registry callbacks tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
