#include "pal4/game_arena_script_scene_callbacks.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using Op = pal4::GameArenaScriptSceneCall;
using Callbacks = pal4::GameArenaScriptSceneCallbacks;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::string R(std::uint32_t address) {
    return "R" + std::to_string(address);
}
std::string W(std::uint32_t address) {
    return "W" + std::to_string(address);
}
std::string C(Op operation) {
    return "C" + std::to_string(static_cast<unsigned>(operation));
}

struct Invocation {
    Op operation;
    std::uint32_t first;
    std::uint32_t second;
    std::uint32_t third;
};

struct Runtime final : pal4::IGameArenaScriptSceneRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x5030u, 1u}, {0x5010u, 0x8500u}, {0x5194u, 0x9000u},
        {0x61f0u, 1u}, {0x6018u, 0u}, {0x6014u, 0x8000u},
        {0x6b10u, 1u}, {0x71f0u, 1u}, {0x7018u, 0u},
        {0x7014u, 0x8100u}, {0xa000u, 0xb000u}};
    std::vector<std::string> events;
    std::vector<Invocation> calls;
    std::int32_t property = 2;
    bool mutateClump = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        events.push_back(R(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        events.push_back(W(address));
        words[address] = value;
    }
    std::uint32_t Call(Op operation, std::uint32_t first,
                       std::uint32_t second,
                       std::uint32_t third) override {
        events.push_back(C(operation));
        calls.push_back({operation, first, second, third});
        if (operation == Op::GetPetInstance) return 0x7000u;
        if (operation == Op::GetMovementAdapter) return 0xa000u;
        if (operation == Op::GetCharacterProperty) {
            words[0x5194u] = 0x9010u;
            return static_cast<std::uint32_t>(property);
        }
        if (mutateClump && operation == Op::AddClump) {
            words[0x6014u] = 0x8010u;
            words[0x7014u] = 0x8110u;
        }
        if (mutateClump && operation == Op::RemoveClump) {
            words[0x6014u] = 0x8020u;
            words[0x7014u] = 0x8120u;
        }
        if (mutateClump && operation == Op::SetClumpRenderCallback)
            words[0x6014u] = 0x8030u;
        return 0u;
    }
};

void Gates() {
    Runtime runtime;
    runtime.words[0x5030u] = 0u;
    Check(Callbacks::AddObject(0x5000u, 0x6000u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{R(0x5030u)},
          "object add stops at missing world");
    runtime = Runtime{};
    Check(Callbacks::RemoveObject(0x5000u, 0u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{R(0x5030u)},
          "object remove checks world before null object");
    runtime = Runtime{};
    runtime.words[0x5030u] = 0u;
    Check(Callbacks::AddPet(0x5000u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{R(0x5030u)},
          "pet add stops before singleton getter without world");
    runtime = Runtime{};
    runtime.words[0x7014u] = 0u;
    Check(Callbacks::RemovePet(0x5000u, runtime) == 0u &&
              runtime.events == (std::vector<std::string>{
                  R(0x5030u), C(Op::GetPetInstance), R(0x7014u)}),
          "pet remove stops when singleton clump is missing");
}

void ObjectCallbacks() {
    Runtime runtime;
    runtime.mutateClump = true;
    Check(Callbacks::AddObject(0x5000u, 0x6000u, runtime) == 1u &&
              runtime.events == (std::vector<std::string>{
                  R(0x5030u), R(0x61f0u), R(0x6018u),
                  R(0x6014u), C(Op::AddClump),
                  C(Op::SetAnimationFlag), R(0x5010u),
                  C(Op::RenderCamera), R(0x6014u),
                  C(Op::ProcessClumpAtomics), R(0x5194u),
                  C(Op::GetCharacterProperty), R(0x5194u),
                  C(Op::InitializeProperties)}),
          "object add follows IDA calls and rereads clump/property pointers");
    Check(runtime.calls[0].first == 0x5000u &&
              runtime.calls[0].second == 0x8000u &&
              runtime.calls[1].first == 0x6170u &&
              runtime.calls[2].third == 0x8500u &&
              runtime.calls[3].first == 0x8010u &&
              runtime.calls[3].second == 0x512cu &&
              runtime.calls.back().second == 0x9010u,
          "object add arguments and changed pointer values");

    runtime = Runtime{};
    runtime.words[0x61f0u] = 0u;
    runtime.property = -1;
    Check(Callbacks::AddObject(0x5000u, 0x6000u, runtime) == 1u &&
              runtime.calls.size() == 1u &&
              runtime.calls[0].operation == Op::GetCharacterProperty,
          "inactive object and nonpositive property skip optional add work");

    runtime = Runtime{};
    runtime.mutateClump = true;
    Check(Callbacks::RemoveObject(0x5000u, 0x6000u, runtime) == 1u &&
              runtime.events == (std::vector<std::string>{
                  R(0x5030u), C(Op::CleanupCamera),
                  C(Op::SetStateFlag), C(Op::CleanupObject),
                  R(0x6018u), R(0x6014u), C(Op::RemoveClump),
                  C(Op::GetObjectProperty), R(0x6b10u),
                  R(0x6014u), C(Op::GetMovementAdapter),
                  R(0xa000u), C(Op::SetClumpRenderCallback),
                  W(0x6b10u), R(0x6014u),
                  C(Op::ProcessClumpAtomicsWrapper)}),
          "object remove preserves cleanup and callback restoration order");
    Check(runtime.calls[3].second == 0x8000u &&
              runtime.calls[6].first == 0xb000u &&
              runtime.calls[6].second == 0x8020u &&
              runtime.calls.back().first == 0x8030u &&
              runtime.words[0x6b10u] == 0u,
          "remove callback uses pointer rereads and clears restoration flag");

    runtime = Runtime{};
    runtime.words[0x6018u] = 1u;
    runtime.words[0x6b10u] = 0u;
    Check(Callbacks::RemoveObject(0x5000u, 0x6000u, runtime) == 1u &&
              runtime.calls.size() == 5u &&
              runtime.calls.back().operation ==
                  Op::ProcessClumpAtomicsWrapper,
          "attached clump and unset callback flag skip optional remove work");
}

void PetCallbacks() {
    Runtime runtime;
    runtime.mutateClump = true;
    Check(Callbacks::AddPet(0x5000u, runtime) == 1u &&
              runtime.events == (std::vector<std::string>{
                  R(0x5030u), C(Op::GetPetInstance), R(0x71f0u),
                  C(Op::GetPetInstance), R(0x7018u),
                  C(Op::GetPetInstance), R(0x7014u),
                  C(Op::AddClump), C(Op::GetPetInstance),
                  C(Op::SetAnimationFlag), R(0x5010u),
                  C(Op::GetPetInstance), C(Op::RenderCamera),
                  C(Op::GetPetInstance), R(0x7014u),
                  C(Op::ProcessClumpAtomics), R(0x5194u),
                  C(Op::GetCharacterProperty),
                  C(Op::GetPetInstance), R(0x5194u),
                  C(Op::InitializeProperties)}),
          "pet add reacquires singleton for every observed operation");
    Check(runtime.calls[3].second == 0x8100u &&
              runtime.calls[5].first == 0x7170u &&
              runtime.calls[7].first == 0x7000u &&
              runtime.calls[7].third == 0x8500u &&
              runtime.calls[9].first == 0x8110u &&
              runtime.calls.back().second == 0x9010u,
          "pet add uses changed clump and reread property pointer");

    runtime = Runtime{};
    runtime.words[0x71f0u] = 0u;
    runtime.words[0x5194u] = 0u;
    Check(Callbacks::AddPet(0x5000u, runtime) == 1u &&
              runtime.calls.size() == 1u &&
              runtime.calls[0].operation == Op::GetPetInstance,
          "inactive pet and absent property skip scene setup");

    runtime = Runtime{};
    runtime.mutateClump = true;
    Check(Callbacks::RemovePet(0x5000u, runtime) == 1u &&
              runtime.events == (std::vector<std::string>{
                  R(0x5030u), C(Op::GetPetInstance), R(0x7014u),
                  C(Op::GetPetInstance), C(Op::CleanupCamera),
                  C(Op::GetPetInstance), C(Op::SetStateFlag),
                  C(Op::GetPetInstance), C(Op::CleanupObject),
                  C(Op::GetPetInstance), R(0x7018u),
                  C(Op::GetPetInstance), R(0x7014u),
                  C(Op::RemoveClump), C(Op::GetPetInstance),
                  C(Op::GetObjectProperty), C(Op::GetPetInstance),
                  R(0x7014u), C(Op::ProcessClumpAtomicsWrapper)}),
          "pet remove follows singleton, clump and cleanup order");
    Check(runtime.calls[2].first == 0x7000u &&
              runtime.calls[4].second == 0u &&
              runtime.calls[9].second == 0x8100u &&
              runtime.calls.back().first == 0x8120u,
          "pet remove arguments and clump reread after removal");

    runtime = Runtime{};
    runtime.words[0x7018u] = 1u;
    Check(Callbacks::RemovePet(0x5000u, runtime) == 1u &&
              runtime.calls.back().operation ==
                  Op::ProcessClumpAtomicsWrapper,
          "attached pet clump is not removed from world");

    pal4::GameArenaScriptSceneRuntimeStub stub;
    Check(Callbacks::AddObject(0u, 0u, stub) == 0u &&
              Callbacks::RemoveObject(0u, 0u, stub) == 0u &&
              Callbacks::AddPet(0u, stub) == 0u &&
              Callbacks::RemovePet(0u, stub) == 0u,
          "native scene callback stub links");
}

}  // namespace

int main() {
    try {
        Gates();
        ObjectCallbacks();
        PetCallbacks();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
