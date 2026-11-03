#include "pal4/default_locale_facet.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IDefaultLocaleFacetRuntime {
public:
    void Write(const std::uint32_t address,
               const std::uint32_t value) override {
        words[address] = value;
        events.emplace_back("write");
    }

    void Free(const std::uint32_t address) override {
        freed.push_back(address);
        events.emplace_back("free");
    }

    [[nodiscard]] std::uint32_t RegisterExitHandler(
        const std::uint32_t address) override {
        registered = address;
        events.emplace_back("register");
        return registration_result;
    }

    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> freed;
    std::vector<std::string> events;
    std::uint32_t registered = 0u;
    std::uint32_t registration_result = 29u;
};

void CheckGlobalInitializationAndReset() {
    Runtime runtime;
    Check(pal4::DefaultLocaleFacet::InitializeAndRegisterExit(runtime) == 29u,
          "initialization returns CRT registration result");
    Check(runtime.words[pal4::DefaultLocaleFacet::kGlobalAddress] ==
              pal4::DefaultLocaleFacet::kVtableAddress &&
              runtime.registered == pal4::DefaultLocaleFacet::kResetEntry &&
              runtime.events ==
                  std::vector<std::string>{"write", "register"},
          "global default is set before reset callback registration");
    runtime.words[pal4::DefaultLocaleFacet::kGlobalAddress] = 0u;
    pal4::DefaultLocaleFacet::Reset(runtime);
    Check(runtime.words[pal4::DefaultLocaleFacet::kGlobalAddress] ==
              pal4::DefaultLocaleFacet::kVtableAddress,
          "exit reset restores the same default pointer");
}

void CheckDestructorFlags() {
    Runtime runtime;
    Check(pal4::LocaleFacetLifecycle::Destroy(0x1000u, 0u, runtime) ==
              0x1000u && runtime.freed.empty(),
          "nondeleting destructor returns object without freeing");
    Check(runtime.words[0x1000u] ==
              pal4::DefaultLocaleFacet::kVtableAddress,
          "destructor writes default facet vtable");
    Check(pal4::LocaleFacetLifecycle::Destroy(0x2000u, 2u, runtime) ==
              0x2000u && runtime.freed.empty(),
          "only low flag bit requests deallocation");
    Check(pal4::LocaleFacetLifecycle::Destroy(0x3000u, 3u, runtime) ==
              0x3000u && runtime.freed == std::vector<std::uint32_t>{0x3000u},
          "deleting destructor frees object and returns original address");
    Check(runtime.events == std::vector<std::string>{
              "write", "write", "write", "free"},
          "vtable write precedes optional MemFree");
}

void CheckActorMessageFacetVariant() {
    Runtime runtime;
    pal4::ActorMessageLocaleFacet::SetVtable(0x4000u, runtime);
    Check(runtime.words[0x4000u] == 0x841e80u &&
              runtime.freed.empty(),
          "second facet initializer writes its own vtable");
    Check(pal4::ActorMessageLocaleFacet::Destroy(
              0x5000u, 2u, runtime) == 0x5000u && runtime.freed.empty(),
          "high destructor flag bit does not free second facet");
    Check(pal4::ActorMessageLocaleFacet::Destroy(
              0x6000u, 1u, runtime) == 0x6000u &&
              runtime.words[0x6000u] == 0x841e80u &&
              runtime.freed == std::vector<std::uint32_t>{0x6000u},
          "low destructor flag bit frees after vtable write");
}

void CheckSystemEventFacet() {
    Runtime runtime;
    constexpr auto object = 0x7000u;
    Check(pal4::SystemEventLocaleFacet::InitializeData(object, runtime) ==
              object &&
          runtime.words[object] == 0x844928u &&
          runtime.words[object + 4u] == 0u,
          "event facet initializes vtable and second word");
    runtime.words[object + 4u] = 19u;
    pal4::SystemEventLocaleFacet::CleanupData(object, runtime);
    Check(runtime.words[object] == 0x844928u &&
          runtime.words[object + 4u] == 0u,
          "event facet cleanup resets data");
    pal4::SystemEventLocaleFacet::ResetBase(object, runtime);
    Check(runtime.words[object] == 0x84491cu,
          "base facet reset writes its vtable");
    Check(pal4::SystemEventLocaleFacet::DestroyBase(
              object, 2u, runtime) == object && runtime.freed.empty() &&
          pal4::SystemEventLocaleFacet::DestroyBase(
              object, 1u, runtime) == object &&
          runtime.freed == std::vector<std::uint32_t>{object},
          "base facet frees only when low deletion flag is set");
}

void CheckActorManagerFacet() {
    Runtime runtime;
    constexpr auto object = 0x8000u;
    pal4::ActorManagerLocaleFacet::Reset(object, runtime);
    Check(runtime.words[object] == 0x844cccu && runtime.freed.empty(),
          "actor facet reset writes its own vtable");
    runtime.events.clear();
    Check(pal4::ActorManagerLocaleFacet::Destroy(object, 2u, runtime) ==
              object && runtime.freed.empty() &&
              runtime.events == std::vector<std::string>{"write"},
          "actor facet nondeleting flags preserve the object");
    runtime.events.clear();
    Check(pal4::ActorManagerLocaleFacet::Destroy(object, 3u, runtime) ==
              object &&
              runtime.freed == std::vector<std::uint32_t>{object} &&
              runtime.events ==
                  std::vector<std::string>{"write", "free"},
          "actor facet deletion writes vtable before freeing");
}

}  // namespace

int main() {
    CheckGlobalInitializationAndReset();
    CheckDestructorFlags();
    CheckActorMessageFacetVariant();
    CheckSystemEventFacet();
    CheckActorManagerFacet();
    std::cout << "5 default locale facet test cases passed\n";
}
