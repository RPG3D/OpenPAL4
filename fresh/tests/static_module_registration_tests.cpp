#include "pal4/static_module_registration.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::array<std::uint32_t, 232> kInitializerTargets{{
    0x45ad50u, 0x45aeb0u, 0x45b010u, 0x45b170u, 0x45b2d0u, 0x45b430u, 0x45b590u, 0x45b6f0u,
    0x45b850u, 0x45b9b0u, 0x45bb10u, 0x45bc70u, 0x45bdd0u, 0x45bf30u, 0x45c090u, 0x45c1f0u,
    0x45c3a0u, 0x45c500u, 0x45c660u, 0x45c7c0u, 0x45c920u, 0x45ca80u, 0x45cbe0u, 0x45cd40u,
    0x45cea0u, 0x45d000u, 0x45d160u, 0x45d2a0u, 0x45d3e0u, 0x45d520u, 0x45d660u, 0x45d7a0u,
    0x45d8e0u, 0x45da20u, 0x45db60u, 0x45dca0u, 0x45dde0u, 0x45df20u, 0x45e060u, 0x45e1a0u,
    0x45e2e0u, 0x45e420u, 0x45e560u, 0x45e6a0u, 0x45e7e0u, 0x45e920u, 0x45ea60u, 0x45eba0u,
    0x45ece0u, 0x45ee20u, 0x45ef60u, 0x45f0a0u, 0x45f1e0u, 0x45f320u, 0x45f460u, 0x45f5a0u,
    0x45f6e0u, 0x45f820u, 0x45f960u, 0x45faa0u, 0x45fbe0u, 0x45fd20u, 0x45fe60u, 0x45ffa0u,
    0x4600e0u, 0x460220u, 0x460360u, 0x4604a0u, 0x4605e0u, 0x460720u, 0x460860u, 0x4609a0u,
    0x460ae0u, 0x460c20u, 0x460d60u, 0x460ec0u, 0x461020u, 0x461180u, 0x4612e0u, 0x461440u,
    0x4615a0u, 0x461700u, 0x461860u, 0x4619c0u, 0x461b20u, 0x461c80u, 0x461de0u, 0x461f40u,
    0x4620a0u, 0x462200u, 0x462360u, 0x4624c0u, 0x462620u, 0x462780u, 0x4628e0u, 0x462a40u,
    0x462ba0u, 0x462d00u, 0x462e60u, 0x462fc0u, 0x463120u, 0x463280u, 0x4633e0u, 0x463540u,
    0x4636a0u, 0x463800u, 0x463960u, 0x463ac0u, 0x463c20u, 0x463d80u, 0x463ee0u, 0x464040u,
    0x4641a0u, 0x464300u, 0x464460u, 0x4645c0u, 0x464720u, 0x464880u, 0x4649e0u, 0x464b40u,
    0x464ca0u, 0x464e00u, 0x464f60u, 0x4650c0u, 0x465220u, 0x465380u, 0x4654e0u, 0x465640u,
    0x4657a0u, 0x465900u, 0x465a60u, 0x465bc0u, 0x465d20u, 0x465e80u, 0x465fe0u, 0x466140u,
    0x4662a0u, 0x466400u, 0x466560u, 0x4666c0u, 0x466820u, 0x466980u, 0x466ae0u, 0x466c40u,
    0x466da0u, 0x466f00u, 0x467060u, 0x4671c0u, 0x467320u, 0x467480u, 0x4675e0u, 0x467740u,
    0x4678a0u, 0x467a00u, 0x467b60u, 0x467cc0u, 0x467e20u, 0x467f80u, 0x4680e0u, 0x468240u,
    0x4683a0u, 0x468500u, 0x468660u, 0x4687c0u, 0x468920u, 0x468a80u, 0x468be0u, 0x468d40u,
    0x468ea0u, 0x469000u, 0x469160u, 0x4692c0u, 0x469420u, 0x469580u, 0x4696e0u, 0x469840u,
    0x4699a0u, 0x469b00u, 0x469c60u, 0x469dc0u, 0x469f20u, 0x46a080u, 0x46a1e0u, 0x46a340u,
    0x46a4a0u, 0x46a600u, 0x46a760u, 0x46a8c0u, 0x46aa20u, 0x46ab80u, 0x46ace0u, 0x46ae40u,
    0x46afa0u, 0x46b100u, 0x46b260u, 0x46b3c0u, 0x46b520u, 0x46b680u, 0x46b7e0u, 0x46b940u,
    0x46baa0u, 0x46bc00u, 0x46bd60u, 0x46bec0u, 0x46c020u, 0x46c180u, 0x46c2e0u, 0x46c440u,
    0x46c5a0u, 0x46c700u, 0x46c860u, 0x46c9c0u, 0x46cb20u,
    0x46cc80u, 0x46cde0u, 0x46cf40u, 0x46d0a0u, 0x46d200u, 0x46d360u, 0x46d4c0u, 0x46d620u,
    0x46d780u, 0x46d8e0u, 0x46da40u, 0x46dba0u, 0x46dd00u, 0x46de60u, 0x46dfc0u, 0x46e120u,
    0x46e280u, 0x46e3e0u, 0x46e540u,
}};

struct Call {
    std::string_view operation;
    std::uint32_t first = 0u;
    std::uint32_t second = 0u;
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::IStaticModuleRuntime {
    std::vector<Call> calls;
    int atexit_result = -19;

    void Initialize(const std::uint32_t object,
                    const std::uint32_t target) override {
        calls.push_back({"initialize", object, target});
    }
    void Cleanup(const std::uint32_t object,
                 const std::uint32_t target) override {
        calls.push_back({"cleanup", object, target});
    }
    int RegisterAtExit(const std::uint32_t callback) override {
        calls.push_back({"atexit", callback});
        return atexit_result;
    }
};

void EveryRegistrationMatchesIda() {
    using pal4::StaticModuleRegistration;
    for (std::uint32_t i = 0u; i < kInitializerTargets.size(); ++i) {
        const auto expected_registration = 0x510290u + i * 0x40u;
        const auto expected_object = 0x8f2d40u - i * 0x38u;
        const auto descriptor = StaticModuleRegistration::Describe(i);
        Check(descriptor.registration_entry == expected_registration &&
                  descriptor.initializer_entry == expected_registration + 0x10u &&
                  descriptor.cleanup_entry == expected_registration + 0x30u &&
                  descriptor.object_address == expected_object &&
                  descriptor.initializer_target == kInitializerTargets[i] &&
                  descriptor.cleanup_target == 0x45c330u,
              "static module descriptor matches IDA for every index");

        Runtime registered;
        Check(StaticModuleRegistration::Register(i, registered) == -19 &&
                  registered.calls == std::vector<Call>{
                      {"initialize", expected_object, kInitializerTargets[i]},
                      {"atexit", expected_registration + 0x30u}},
              "static module wrapper initializes before CRT registration");

        Runtime direct;
        StaticModuleRegistration::Initialize(i, direct);
        StaticModuleRegistration::Cleanup(i, direct);
        Check(direct.calls == std::vector<Call>{
                  {"initialize", expected_object, kInitializerTargets[i]},
                  {"cleanup", expected_object, 0x45c330u}},
              "direct initializer and cleanup dispatch through original targets");
    }
}

void EverySecondRegistrationMatchesIda() {
    using pal4::StaticModuleRegistration;
    using pal4::StaticModuleSeries;
    for (std::uint32_t i = 0u; i < kInitializerTargets.size(); ++i) {
        const auto registration = 0x514a60u + i * 0x40u;
        const auto object = 0x8f6010u - i * 0x38u;
        const auto descriptor = StaticModuleRegistration::Describe(
            StaticModuleSeries::Second, i);
        Check(descriptor.registration_entry == registration &&
                  descriptor.initializer_entry == registration + 0x10u &&
                  descriptor.cleanup_entry == registration + 0x30u &&
                  descriptor.object_address == object &&
                  descriptor.initializer_target == kInitializerTargets[i] &&
                  descriptor.cleanup_target == 0x45c330u,
              "second static-module descriptor matches every IDA triple");

        Runtime registered;
        Check(StaticModuleRegistration::Register(
                  StaticModuleSeries::Second, i, registered) == -19 &&
                  registered.calls == std::vector<Call>{
                      {"initialize", object, kInitializerTargets[i]},
                      {"atexit", registration + 0x30u}},
              "second wrapper initializes before atexit registration");

        Runtime direct;
        StaticModuleRegistration::Initialize(StaticModuleSeries::Second,
                                             i, direct);
        StaticModuleRegistration::Cleanup(StaticModuleSeries::Second,
                                          i, direct);
        Check(direct.calls == std::vector<Call>{
                  {"initialize", object, kInitializerTargets[i]},
                  {"cleanup", object, 0x45c330u}},
              "second direct entrypoints pass exact object and targets");
    }
}

void EveryThirdRegistrationMatchesIda() {
    using pal4::StaticModuleRegistration;
    using pal4::StaticModuleSeries;
    for (std::uint32_t i = 0u; i < kInitializerTargets.size(); ++i) {
        const auto registration = 0x58b520u + i * 0x40u;
        const auto object = 0x8fdd68u - i * 0x38u;
        const auto descriptor = StaticModuleRegistration::Describe(
            StaticModuleSeries::Third, i);
        Check(descriptor.registration_entry == registration &&
                  descriptor.initializer_entry == registration + 0x10u &&
                  descriptor.cleanup_entry == registration + 0x30u &&
                  descriptor.object_address == object &&
                  descriptor.initializer_target == kInitializerTargets[i] &&
                  descriptor.cleanup_target == 0x45c330u,
              "third static-module descriptor matches every IDA triple");

        Runtime registered;
        Check(StaticModuleRegistration::Register(
                  StaticModuleSeries::Third, i, registered) == -19 &&
                  registered.calls == std::vector<Call>{
                      {"initialize", object, kInitializerTargets[i]},
                      {"atexit", registration + 0x30u}},
              "third wrapper initializes before atexit registration");

        Runtime direct;
        StaticModuleRegistration::Initialize(StaticModuleSeries::Third,
                                             i, direct);
        StaticModuleRegistration::Cleanup(StaticModuleSeries::Third,
                                          i, direct);
        Check(direct.calls == std::vector<Call>{
                  {"initialize", object, kInitializerTargets[i]},
                  {"cleanup", object, 0x45c330u}},
              "third direct entries forward exact object and targets");
    }
}

void EveryFourthRegistrationMatchesIda() {
    using pal4::StaticModuleRegistration;
    using pal4::StaticModuleSeries;
    for (std::uint32_t i = 0u; i < kInitializerTargets.size(); ++i) {
        const auto registration = 0x5b7430u + i * 0x40u;
        const auto object = 0x901370u - i * 0x38u;
        const auto descriptor = StaticModuleRegistration::Describe(
            StaticModuleSeries::Fourth, i);
        Check(descriptor.registration_entry == registration &&
                  descriptor.initializer_entry == registration + 0x10u &&
                  descriptor.cleanup_entry == registration + 0x30u &&
                  descriptor.object_address == object &&
                  descriptor.initializer_target == kInitializerTargets[i] &&
                  descriptor.cleanup_target == 0x45c330u,
              "fourth static-module descriptor matches every IDA triple");

        Runtime registered;
        Check(StaticModuleRegistration::Register(
                  StaticModuleSeries::Fourth, i, registered) == -19 &&
                  registered.calls == std::vector<Call>{
                      {"initialize", object, kInitializerTargets[i]},
                      {"atexit", registration + 0x30u}},
              "fourth wrapper initializes before atexit registration");

        Runtime direct;
        StaticModuleRegistration::Initialize(StaticModuleSeries::Fourth,
                                             i, direct);
        StaticModuleRegistration::Cleanup(StaticModuleSeries::Fourth,
                                          i, direct);
        Check(direct.calls == std::vector<Call>{
                  {"initialize", object, kInitializerTargets[i]},
                  {"cleanup", object, 0x45c330u}},
              "fourth direct entries forward exact object and targets");
    }
}

void EveryFifthRegistrationMatchesIda() {
    using pal4::StaticModuleRegistration;
    using pal4::StaticModuleSeries;
    for (std::uint32_t i = 0u; i < kInitializerTargets.size(); ++i) {
        const auto registration = 0x5bee10u + i * 0x40u;
        const auto object = 0x904630u - i * 0x38u;
        const auto descriptor = StaticModuleRegistration::Describe(
            StaticModuleSeries::Fifth, i);
        Check(descriptor.registration_entry == registration &&
                  descriptor.initializer_entry == registration + 0x10u &&
                  descriptor.cleanup_entry == registration + 0x30u &&
                  descriptor.object_address == object &&
                  descriptor.initializer_target == kInitializerTargets[i] &&
                  descriptor.cleanup_target == 0x45c330u,
              "fifth static-module descriptor matches every IDA triple");

        Runtime registered;
        Check(StaticModuleRegistration::Register(
                  StaticModuleSeries::Fifth, i, registered) == -19 &&
                  registered.calls == std::vector<Call>{
                      {"initialize", object, kInitializerTargets[i]},
                      {"atexit", registration + 0x30u}},
              "fifth wrapper initializes before atexit registration");

        Runtime direct;
        StaticModuleRegistration::Initialize(StaticModuleSeries::Fifth,
                                             i, direct);
        StaticModuleRegistration::Cleanup(StaticModuleSeries::Fifth,
                                          i, direct);
        Check(direct.calls == std::vector<Call>{
                  {"initialize", object, kInitializerTargets[i]},
                  {"cleanup", object, 0x45c330u}},
              "fifth direct entries forward exact object and targets");
    }
}

void EverySixthRegistrationMatchesIda() {
    using pal4::StaticModuleRegistration;
    using pal4::StaticModuleSeries;
    for (std::uint32_t i = 0u; i < 231u; ++i) {
        const auto registration = 0x5c4470u + i * 0x40u;
        const auto object = 0x9078b8u - i * 0x38u;
        const auto target = kInitializerTargets[i + 1u];
        const auto descriptor = StaticModuleRegistration::Describe(
            StaticModuleSeries::Sixth, i);
        Check(descriptor.registration_entry == registration &&
                  descriptor.initializer_entry == registration + 0x10u &&
                  descriptor.cleanup_entry == registration + 0x30u &&
                  descriptor.object_address == object &&
                  descriptor.initializer_target == target &&
                  descriptor.cleanup_target == 0x45c330u,
              "sixth static-module descriptor matches every IDA triple");

        Runtime registered;
        Check(StaticModuleRegistration::Register(
                  StaticModuleSeries::Sixth, i, registered) == -19 &&
                  registered.calls == std::vector<Call>{
                      {"initialize", object, target},
                      {"atexit", registration + 0x30u}},
              "sixth wrapper initializes before atexit registration");

        Runtime direct;
        StaticModuleRegistration::Initialize(StaticModuleSeries::Sixth,
                                             i, direct);
        StaticModuleRegistration::Cleanup(StaticModuleSeries::Sixth,
                                          i, direct);
        Check(direct.calls == std::vector<Call>{
                  {"initialize", object, target},
                  {"cleanup", object, 0x45c330u}},
              "sixth direct entries forward exact object and targets");
    }
}

void EverySeventhRegistrationMatchesIda() {
    using pal4::StaticModuleRegistration;
    using pal4::StaticModuleSeries;
    for (std::uint32_t i = 0u; i < 232u; ++i) {
        const auto registration = 0x5c84a0u + i * 0x40u;
        const auto object = 0x90abb8u - i * 0x38u;
        const auto target = kInitializerTargets[i];
        const auto descriptor = StaticModuleRegistration::Describe(
            StaticModuleSeries::Seventh, i);
        Check(descriptor.registration_entry == registration &&
                  descriptor.initializer_entry == registration + 0x10u &&
                  descriptor.cleanup_entry == registration + 0x30u &&
                  descriptor.object_address == object &&
                  descriptor.initializer_target == target &&
                  descriptor.cleanup_target == 0x45c330u,
              "seventh static-module descriptor matches every IDA triple");

        Runtime registered;
        Check(StaticModuleRegistration::Register(
                  StaticModuleSeries::Seventh, i, registered) == -19 &&
                  registered.calls == std::vector<Call>{
                      {"initialize", object, target},
                      {"atexit", registration + 0x30u}},
              "seventh wrapper initializes before atexit registration");

        Runtime direct;
        StaticModuleRegistration::Initialize(StaticModuleSeries::Seventh,
                                             i, direct);
        StaticModuleRegistration::Cleanup(StaticModuleSeries::Seventh,
                                          i, direct);
        Check(direct.calls == std::vector<Call>{
                  {"initialize", object, target},
                  {"cleanup", object, 0x45c330u}},
              "seventh direct entries forward exact object and targets");
    }
}

void EveryEighthRegistrationMatchesIda() {
    using pal4::StaticModuleRegistration;
    using pal4::StaticModuleSeries;
    for (std::uint32_t i = 0u; i < 232u; ++i) {
        const auto registration = 0x5cf5b0u + i * 0x40u;
        const auto object = 0x90de88u - i * 0x38u;
        const auto descriptor = StaticModuleRegistration::Describe(
            StaticModuleSeries::Eighth, i);
        Check(descriptor.registration_entry == registration &&
                  descriptor.initializer_entry == registration + 0x10u &&
                  descriptor.cleanup_entry == registration + 0x30u &&
                  descriptor.object_address == object &&
                  descriptor.initializer_target == kInitializerTargets[i] &&
                  descriptor.cleanup_target == 0x45c330u,
              "eighth descriptor matches every IDA-verified triple");

        Runtime registered;
        Check(StaticModuleRegistration::Register(
                  StaticModuleSeries::Eighth, i, registered) == -19 &&
                  registered.calls == std::vector<Call>{
                      {"initialize", object, kInitializerTargets[i]},
                      {"atexit", registration + 0x30u}},
              "eighth registration initializes before atexit");

        Runtime direct;
        StaticModuleRegistration::Initialize(StaticModuleSeries::Eighth,
                                             i, direct);
        StaticModuleRegistration::Cleanup(StaticModuleSeries::Eighth,
                                          i, direct);
        Check(direct.calls == std::vector<Call>{
                  {"initialize", object, kInitializerTargets[i]},
                  {"cleanup", object, 0x45c330u}},
              "eighth direct entries forward exact object and targets");
    }
}

void RejectsOutOfRangeIndex() {
    Runtime runtime;
    for (const auto index : {232u, 0xffffffffu}) {
        bool threw = false;
        try {
            static_cast<void>(pal4::StaticModuleRegistration::Describe(index));
        } catch (const std::out_of_range&) {
            threw = true;
        }
        Check(threw, "module descriptor rejects invalid index");
        threw = false;
        try {
            static_cast<void>(pal4::StaticModuleRegistration::Register(
                index, runtime));
        } catch (const std::out_of_range&) {
            threw = true;
        }
        Check(threw && runtime.calls.empty(),
              "invalid registration has no runtime side effects");
        threw = false;
        try {
            static_cast<void>(pal4::StaticModuleRegistration::Describe(
                pal4::StaticModuleSeries::Second, index));
        } catch (const std::out_of_range&) {
            threw = true;
        }
        Check(threw, "second series rejects invalid index");
    }
    for (const auto index : {232u, 0xffffffffu}) {
        bool threw = false;
        try {
            static_cast<void>(pal4::StaticModuleRegistration::Describe(
                pal4::StaticModuleSeries::Third, index));
        } catch (const std::out_of_range&) {
            threw = true;
        }
        Check(threw, "third series rejects invalid index");
        threw = false;
        try {
            static_cast<void>(pal4::StaticModuleRegistration::Describe(
                pal4::StaticModuleSeries::Fourth, index));
        } catch (const std::out_of_range&) {
            threw = true;
        }
        Check(threw, "fourth series rejects invalid index");
        threw = false;
        try {
            static_cast<void>(pal4::StaticModuleRegistration::Describe(
                pal4::StaticModuleSeries::Fifth, index));
        } catch (const std::out_of_range&) {
            threw = true;
        }
        Check(threw, "fifth series rejects invalid index");
    }
    for (const auto index : {231u, 0xffffffffu}) {
        bool threw = false;
        try {
            static_cast<void>(pal4::StaticModuleRegistration::Describe(
                pal4::StaticModuleSeries::Sixth, index));
        } catch (const std::out_of_range&) {
            threw = true;
        }
        Check(threw, "sixth series rejects invalid index");
        threw = false;
        try {
            static_cast<void>(pal4::StaticModuleRegistration::Register(
                pal4::StaticModuleSeries::Sixth, index, runtime));
        } catch (const std::out_of_range&) {
            threw = true;
        }
        Check(threw && runtime.calls.empty(),
              "invalid sixth registration has no side effects");
    }
    for (const auto index : {232u, 0xffffffffu}) {
        bool threw = false;
        try {
            static_cast<void>(pal4::StaticModuleRegistration::Describe(
                pal4::StaticModuleSeries::Seventh, index));
        } catch (const std::out_of_range&) {
            threw = true;
        }
        Check(threw, "seventh series rejects invalid index");
        threw = false;
        try {
            static_cast<void>(pal4::StaticModuleRegistration::Register(
                pal4::StaticModuleSeries::Seventh, index, runtime));
        } catch (const std::out_of_range&) {
            threw = true;
        }
        Check(threw && runtime.calls.empty(),
              "invalid seventh registration has no side effects");
    }
    for (const auto index : {232u, 0xffffffffu}) {
        bool threw = false;
        try {
            static_cast<void>(pal4::StaticModuleRegistration::Describe(
                pal4::StaticModuleSeries::Eighth, index));
        } catch (const std::out_of_range&) {
            threw = true;
        }
        Check(threw, "eighth series rejects invalid index");
        threw = false;
        try {
            static_cast<void>(pal4::StaticModuleRegistration::Register(
                pal4::StaticModuleSeries::Eighth, index, runtime));
        } catch (const std::out_of_range&) {
            threw = true;
        }
        Check(threw && runtime.calls.empty(),
              "invalid eighth registration has no side effects");
    }
}

}  // namespace

int main() {
    try {
        EveryRegistrationMatchesIda();
        EverySecondRegistrationMatchesIda();
        EveryThirdRegistrationMatchesIda();
        EveryFourthRegistrationMatchesIda();
        EveryFifthRegistrationMatchesIda();
        EverySixthRegistrationMatchesIda();
        EverySeventhRegistrationMatchesIda();
        EveryEighthRegistrationMatchesIda();
        RejectsOutOfRangeIndex();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
