#include "pal4/combat_console_element_factory.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatConsoleElementFactoryRuntime {
    std::uint32_t allocation = 0xa000u;
    std::uint32_t expected_size = 0x40u;
    std::vector<std::string> calls;
    std::uint32_t vtable = 0u;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == expected_size, "element size");
        calls.emplace_back("allocate");
        return allocation;
    }
    void ConstructBase(std::uint32_t object) override {
        Check(object == allocation, "base object address");
        calls.emplace_back("base");
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == allocation, "vtable at object start");
        vtable = value;
        calls.emplace_back("vtable");
    }
};

void SuccessAndAllocationFailure() {
    for (std::uint32_t kind = 0u; kind < 8u; ++kind) {
        Runtime runtime;
        runtime.expected_size = kind == 4u ? 0x64u :
                                kind == 2u || kind == 3u || kind == 5u ?
                                    0x44u : 0x40u;
        const auto create = [&](Runtime& target) {
            if (kind == 0u)
                return pal4::CombatConsoleElementFactory::Create(target);
            if (kind == 1u)
                return pal4::CombatResultIconFactory::Create(target);
            if (kind == 2u)
                return pal4::CombatResultSoundFactory::Create(target);
            if (kind == 3u)
                return pal4::CombatResultMoveFactory::Create(target);
            if (kind == 4u)
                return pal4::CombatResultAudioFactory::Create(target);
            if (kind == 5u)
                return pal4::CombatResultStateFactory::Create(target);
            if (kind == 6u)
                return pal4::CombatResultPanelFactory::Create(target);
            return pal4::CombatResultEndFactory::Create(target);
        };
        Check(create(runtime) == 0xa000u &&
                  runtime.vtable ==
                      (kind == 0u ? 0x8453ccu :
                       kind == 1u ? 0x8453e8u :
                       kind == 2u ? 0x845404u :
                       kind == 3u ? 0x845420u :
                       kind == 4u ? 0x845440u :
                       kind == 5u ? 0x84545cu :
                       kind == 6u ? 0x845478u : 0x845494u) &&
                  runtime.calls ==
                      std::vector<std::string>{"allocate", "base", "vtable"},
              "base construction precedes each derived vtable");
        Runtime failed;
        failed.allocation = 0u;
        failed.expected_size = runtime.expected_size;
        Check(create(failed) == 0u &&
                  failed.calls == std::vector<std::string>{"allocate"},
              "failed allocation skips both constructors");
    }
}

struct DestructorRuntime final : pal4::ICombatResultElementDestructorRuntime {
    std::vector<std::string> calls;
    std::uint32_t DestroyBase(std::uint32_t object) override {
        Check(object == 0xa000u, "base destruction address");
        calls.emplace_back("base");
        return 0xb000u;
    }
    void Free(std::uint32_t object) override {
        Check(object == 0xa000u, "free original object");
        calls.emplace_back("free");
    }
};

void DestructorModes() {
    DestructorRuntime runtime;
    Check(pal4::CombatResultElementDestructor::DestroyBase(
              0xa000u, runtime) == 0xb000u &&
              runtime.calls == std::vector<std::string>{"base"},
          "destructor thunk forwards base result");
    for (const std::uint8_t flags : {0u, 1u, 2u, 3u}) {
        runtime.calls.clear();
        Check(pal4::CombatResultElementDestructor::Delete(
                  0xa000u, flags, runtime) == 0xa000u &&
                  runtime.calls ==
                      ((flags & 1u) != 0u ?
                           std::vector<std::string>{"base", "free"} :
                           std::vector<std::string>{"base"}),
              "deleting destructor tests only low flag bit");
    }
}

}  // namespace

int main() {
    try {
        SuccessAndAllocationFailure();
        DestructorModes();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
