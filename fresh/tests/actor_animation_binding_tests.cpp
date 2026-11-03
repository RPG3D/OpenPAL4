#include "pal4/actor_animation_binding.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr auto actor = 0x1000u;
constexpr auto clump = 0x2000u;
constexpr auto hook = 0x3000u;

struct Runtime final : pal4::IActorAnimationBindingRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> trace;
    std::uint32_t found_hook = hook;
    std::uint32_t identity_callback = 0x9000u;
    std::uint32_t replacement_frame = 0;
    std::uint32_t hide_result = 0x55u;
    std::uint32_t processed_context = 0;
    std::uint32_t processed_frame = 0;
    std::string processed_name;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
        trace.push_back("callback:" + std::to_string(value));
    }
    bool EqualIgnoringCase(const std::string_view left,
                           const std::string_view right) override {
        trace.push_back("compare");
        return left.size() == right.size() &&
               std::equal(left.begin(), left.end(), right.begin(),
                   [](const char a, const char b) {
                       return std::tolower(static_cast<unsigned char>(a)) ==
                              std::tolower(static_cast<unsigned char>(b));
                   });
    }
    std::uint32_t FindHookHand(const std::uint32_t target) override {
        Check(target == clump, "hook search uses actor clump");
        trace.push_back("find-hook");
        return found_hook;
    }
    std::uint32_t IdentityRenderCallback() override {
        trace.push_back("identity-callback");
        return identity_callback;
    }
    std::uint32_t DefaultRenderCallback() override {
        trace.push_back("default-callback");
        return 0x9100u;
    }
    void InvertMatrix(const std::uint32_t destination,
                    const std::uint32_t source) override {
        Check(destination == 0x4010u && source == 0x5010u,
              "invert hook source matrix into clump frame matrix");
        trace.push_back("invert-matrix");
    }
    void UpdateFrame(const std::uint32_t frame) override {
        trace.push_back("update-frame:" + std::to_string(frame));
        if (replacement_frame && frame == 0x4000u)
            words[clump + 4u] = replacement_frame;
    }
    void RotateMatrix(const std::uint32_t matrix,
                      const std::uint32_t axis,
                      const float degrees,
                      const std::uint32_t mode) override {
        Check(mode == 2u, "RenderWare rotation uses combine mode two");
        trace.push_back("rotate:" + std::to_string(matrix) + ":" +
                        std::to_string(axis) + ":" +
                        std::to_string(static_cast<int>(degrees)));
    }
    void ProcessUserData(const std::uint32_t context,
                         const std::string_view name,
                         const std::uint32_t frame) override {
        processed_context = context;
        processed_name = name;
        processed_frame = frame;
        trace.push_back("process-userdata");
    }
    std::uint32_t HideHookAtomics(const std::uint32_t target) override {
        Check(target == clump, "hide hook atomics uses current clump");
        trace.push_back("hide-hook-atomics");
        return hide_result;
    }
};

void InitializeHook(Runtime& runtime) {
    runtime.words[actor + 20u] = clump;
    runtime.words[hook + 60u] = clump;
    runtime.words[hook + 4u] = 0x5000u;
    runtime.words[clump + 4u] = 0x4000u;
}

void CallbackGates() {
    Runtime runtime;
    Check(pal4::ActorAnimationBinding::BindCallback(
              actor, false, 0u, runtime) == 0u &&
          runtime.trace.empty(), "null clump skips hook search");
    runtime.words[actor + 20u] = clump;
    runtime.found_hook = 0;
    Check(pal4::ActorAnimationBinding::BindCallback(
              actor, false, 0u, runtime) == 0u &&
          runtime.trace == std::vector<std::string>{"find-hook"},
          "missing hook leaves rotation untouched");
    runtime.found_hook = hook;
    runtime.trace.clear();
    Check(pal4::ActorAnimationBinding::RotateOrdinary(
              hook, false, runtime) == hook && runtime.trace.empty(),
          "hook without clump returns its own address");
}

void OrdinaryAndAlternateRotation() {
    Runtime ordinary;
    InitializeHook(ordinary);
    Check(pal4::ActorAnimationBinding::BindCallback(
              actor, false, 0u, ordinary) == 0u &&
          ordinary.words[hook + 72u] == 0x9000u &&
          ordinary.trace == std::vector<std::string>{
              "find-hook", "identity-callback", "callback:36864",
              "invert-matrix", "update-frame:16384",
              "rotate:16400:8669500:90", "rotate:16400:8669512:180",
              "update-frame:16384"},
          "ordinary non-left hook uses 90 then 180 degree rotations");
    Runtime alternate;
    InitializeHook(alternate);
    alternate.identity_callback = 0;
    alternate.replacement_frame = 0x4100u;
    Check(pal4::ActorAnimationBinding::BindCallback(
              actor, true, 1u, alternate) == 0u &&
          alternate.words[hook + 72u] == 0x9100u &&
          alternate.trace == std::vector<std::string>{
              "find-hook", "identity-callback", "default-callback",
              "callback:37120", "invert-matrix", "update-frame:16384",
              "rotate:16656:8669500:-90", "update-frame:16640"},
          "alternate left hook uses fallback callback and refreshed frame");
}

void UpdateHidesHookAtomics() {
    Runtime runtime;
    InitializeHook(runtime);
    Check(pal4::ActorAnimationBinding::Update(
              actor, 0x7000u, "HOOK_lhweapon", 1u, runtime) ==
              runtime.hide_result &&
          runtime.processed_context == 0x7000u &&
          runtime.processed_name == "HOOK_lhweapon" &&
          runtime.processed_frame == 0x4000u &&
          runtime.trace.front() == "compare" &&
          runtime.trace[runtime.trace.size() - 2] == "process-userdata" &&
          runtime.trace.back() == "hide-hook-atomics",
          "animation update binds callback, processes userdata and hides helpers");
}

}  // namespace

int main() {
    try {
        CallbackGates();
        OrdinaryAndAlternateRotation();
        UpdateHidesHookAtomics();
        std::cout << "actor_animation_binding_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
