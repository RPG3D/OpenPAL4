#include "pal4/manager_object_lifecycle.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IManagerObjectLifecycleRuntime {
public:
    void WriteVtable(const std::uint32_t object,
                     const std::uint32_t vtable) override {
        Check(object == 0x1234u, "vtable write targets same object");
        events.push_back(vtable == expected_vtable ? "derived" : "base");
        Check(vtable == expected_vtable || vtable == 0x842400u,
              "only observed vtable addresses are used");
    }
    std::uint8_t Cleanup(const std::uint32_t object) override {
        Check(object == 0x1234u, "cleanup targets same object");
        events.push_back("cleanup");
        return 0xabu;
    }
    void CleanupEffect(const std::uint32_t object) override {
        Check(object == 0x1234u, "effect cleanup targets same object");
        events.push_back("effect cleanup");
    }
    void CleanupExtended(const std::uint32_t object) override {
        Check(object == 0x1234u, "extended cleanup targets same object");
        events.push_back("extended cleanup");
    }
    std::uint8_t CleanupSecondary(const std::uint32_t object) override {
        Check(object == 0x1234u, "secondary cleanup targets same object");
        events.push_back("secondary cleanup");
        return 0x5cu;
    }
    std::uint32_t ReadDword(const std::uint32_t address) override {
        Check(address == 0x1234u + 36u,
              "extended pointer is read from offset +36");
        events.push_back("read +36");
        return extended_pointer;
    }
    void WriteDword(const std::uint32_t address,
                    const std::uint32_t value) override {
        Check(address >= 0x1234u + 36u &&
                  address <= 0x1234u + 44u &&
                  (address - 0x1234u) % 4u == 0u && value == 0u,
              "extended state clears only observed dwords");
        events.push_back("clear +" + std::to_string(address - 0x1234u));
    }
    void Free(const std::uint32_t object) override {
        Check(object == 0x1234u || object == extended_pointer,
              "free targets object or its owned pointer");
        events.push_back("free");
    }
    std::uint32_t expected_vtable{0x842540u};
    std::uint32_t extended_pointer{0x7800u};
    std::vector<std::string> events;
};

void DestructorOrderingAndResult() {
    Runtime runtime;
    Check(pal4::ManagerObjectLifecycle::DestroyFirst(0x1234u, runtime) ==
              0xabu,
          "destructor preserves cleanup return byte");
    Check(runtime.events == std::vector<std::string>({
              "derived", "cleanup", "base"}),
          "destructor writes derived and base vtables around cleanup");
}

void DeletingDestructorFlagPaths() {
    for (const std::uint8_t flags : {0u, 1u, 2u, 3u}) {
        Runtime runtime;
        Check(pal4::ManagerObjectLifecycle::DeleteFirst(
                  0x1234u, flags, runtime) == 0x1234u,
              "deleting destructor returns original object address");
        auto expected = std::vector<std::string>{
            "derived", "cleanup", "base"};
        if ((flags & 1u) != 0u) expected.push_back("free");
        Check(runtime.events == expected,
              "only low flag bit controls free after destruction");
    }
    pal4::ManagerObjectLifecycleRuntimeStub stub;
    Check(pal4::ManagerObjectLifecycle::DeleteFirst(
              0x1234u, 1u, stub) == 0x1234u,
          "empty lifecycle runtime is linkable");
}

void CommonManagerVariants() {
    struct Variant { std::uint32_t key; std::uint32_t vtable; };
    constexpr Variant variants[]{
        {1u, 0x842540u}, {2u, 0x842500u}, {3u, 0x8424c0u},
        {4u, 0x842480u}, {5u, 0x842440u}, {7u, 0x842380u},
        {9u, 0x842300u}, {10u, 0x8422c0u}, {11u, 0x842280u},
        {12u, 0x842240u}, {13u, 0x842200u},
        {15u, 0x842180u}};
    for (const auto [key, vtable] : variants) {
        Runtime runtime;
        runtime.expected_vtable = vtable;
        Check(pal4::ManagerObjectLifecycle::DestroyCommon(
                  0x1234u, key, runtime) == 0xabu,
              "all common destructors propagate cleanup byte");
        Check(runtime.events == std::vector<std::string>({
                  "derived", "cleanup", "base"}),
              "each common variant has its own derived vtable");
        runtime.events.clear();
        Check(pal4::ManagerObjectLifecycle::DeleteCommon(
                  0x1234u, key, 3u, runtime) == 0x1234u &&
                  runtime.events == std::vector<std::string>({
                      "derived", "cleanup", "base", "free"}),
              "each common deleting destructor frees after cleanup");
    }
}

void ExtendedVariantReleasesOwnedPointer() {
    Runtime runtime;
    runtime.expected_vtable = 0x8423c0u;
    Check(pal4::ManagerObjectLifecycle::DestroyExtended(
              0x1234u, runtime) == 0u,
          "extended destructor returns zero");
    const auto expected = std::vector<std::string>{
        "derived", "extended cleanup", "read +36", "free",
        "clear +36", "clear +40", "clear +44", "base"};
    Check(runtime.events == expected,
          "extended destructor releases pointer then clears three fields");
    runtime.events.clear();
    Check(pal4::ManagerObjectLifecycle::DeleteExtended(
              0x1234u, 0u, runtime) == 0x1234u &&
              runtime.events == expected,
          "extended deleting destructor preserves object without low bit");
    runtime.events.clear();
    Check(pal4::ManagerObjectLifecycle::DeleteExtended(
              0x1234u, 3u, runtime) == 0x1234u &&
              runtime.events.size() == expected.size() + 1u &&
              runtime.events.back() == "free",
          "extended deleting destructor frees object after owned pointer");
    runtime.extended_pointer = 0u;
    runtime.events.clear();
    Check(pal4::ManagerObjectLifecycle::DestroyExtended(
              0x1234u, runtime) == 0u && runtime.events == expected,
          "owned pointer free is still called for null pointer");
}

void SecondaryVariantUsesItsOwnCleanup() {
    Runtime runtime;
    runtime.expected_vtable = 0x8421c0u;
    Check(pal4::ManagerObjectLifecycle::DestroySecondary(
              0x1234u, runtime) == 0x5cu &&
              runtime.events == std::vector<std::string>({
                  "derived", "secondary cleanup", "base"}),
          "key-14 destructor uses secondary cleanup and returns its byte");
    runtime.events.clear();
    Check(pal4::ManagerObjectLifecycle::DeleteSecondary(
              0x1234u, 1u, runtime) == 0x1234u &&
              runtime.events == std::vector<std::string>({
                  "derived", "secondary cleanup", "base", "free"}),
          "key-14 deleting destructor frees after secondary cleanup");
}

void EffectVariantUsesDifferentCleanup() {
    Runtime runtime;
    runtime.expected_vtable = 0x842340u;
    pal4::ManagerObjectLifecycle::DestroyEffect(0x1234u, runtime);
    Check(runtime.events == std::vector<std::string>({
              "derived", "effect cleanup", "base"}),
          "effect manager calls its own cleanup routine");
    runtime.events.clear();
    Check(pal4::ManagerObjectLifecycle::DeleteEffect(
              0x1234u, 0u, runtime) == 0x1234u &&
              runtime.events == std::vector<std::string>({
                  "derived", "effect cleanup", "base"}),
          "effect delete without low flag bit does not free");
    runtime.events.clear();
    Check(pal4::ManagerObjectLifecycle::DeleteEffect(
              0x1234u, 1u, runtime) == 0x1234u &&
              runtime.events == std::vector<std::string>({
                  "derived", "effect cleanup", "base", "free"}),
          "effect delete with low flag bit frees after cleanup");
    Check(pal4::ManagerObjectLifecycle::ConstantTrue() == 1u,
          "effect vtable helper returns constant one");
}

void BaseDeletingDestructor() {
    Runtime runtime;
    Check(pal4::ManagerObjectLifecycle::DeleteBase(
              0x1234u, 0u, runtime) == 0x1234u &&
              runtime.events == std::vector<std::string>({"base"}),
          "base deleting destructor installs only base vtable without flag");
    runtime.events.clear();
    Check(pal4::ManagerObjectLifecycle::DeleteBase(
              0x1234u, 3u, runtime) == 0x1234u &&
              runtime.events == std::vector<std::string>({"base", "free"}),
          "base deleting destructor frees only when low flag bit is set");
}

}  // namespace

int main() {
    DestructorOrderingAndResult();
    DeletingDestructorFlagPaths();
    CommonManagerVariants();
    EffectVariantUsesDifferentCleanup();
    ExtendedVariantReleasesOwnedPointer();
    SecondaryVariantUsesItsOwnCleanup();
    BaseDeletingDestructor();
    std::cout << "7 manager object lifecycle test cases passed\n";
}
