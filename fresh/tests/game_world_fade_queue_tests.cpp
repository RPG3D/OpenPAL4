#include "pal4/game_world_fade_queue.h"

#include <bit>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Event {
    std::string name;
    std::uint32_t first;
    std::uint32_t second;
    bool operator==(const Event&) const = default;
};

struct Runtime final : pal4::IGameWorldFadeRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<Event> effects;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t SetMaterialFlags(std::uint32_t clump,
                                   std::uint32_t rgba) override {
        effects.push_back({"material", clump, rgba});
        return 0xdeadu;
    }
    void ScriptOnRemove(
        std::uint32_t world, std::uint32_t object,
        pal4::GameWorldFadeRemoval callback) override {
        effects.push_back({callback ==
                                   pal4::GameWorldFadeRemoval::ScriptOnRemove3
                               ? "remove3"
                               : "remove5",
                           world, object});
    }
};

void SetQueue(Runtime& runtime, std::uint32_t count) {
    runtime.words[0x526cu] = 0x1000u;
    runtime.words[0x5270u] = 0x1000u + count * 12u;
}

void SetInQueue(Runtime& runtime, std::uint32_t count) {
    runtime.words[0x527cu] = 0x1000u;
    runtime.words[0x5280u] = 0x1000u + count * 12u;
}

void SetSecondaryQueue(Runtime& runtime, std::uint32_t count) {
    runtime.words[0x528cu] = 0x1000u;
    runtime.words[0x5290u] = 0x1000u + count * 12u;
}

void SetEntry(Runtime& runtime, std::uint32_t index,
              std::uint32_t object, float duration, float elapsed,
              bool active) {
    const auto record = 0x1000u + 12u * index;
    runtime.words[record] = object;
    runtime.words[record + 4u] = std::bit_cast<std::uint32_t>(duration);
    runtime.words[record + 8u] = std::bit_cast<std::uint32_t>(elapsed);
    runtime.words[object + 388u] = active ? 1u : 0u;
    runtime.words[object + 20u] = object + 0x80u;
}

void EmptyAndInactive() {
    Runtime empty;
    SetQueue(empty, 0u);
    Check(pal4::GameWorldFadeQueue::Advance(0x5000u, 0.25f, empty) ==
              0x1000u && empty.effects.empty(),
          "empty fade queue returns original end pointer");

    Runtime inactive;
    SetQueue(inactive, 1u);
    SetEntry(inactive, 0u, 0x2000u, 1.0f, 0.0f, false);
    Check(pal4::GameWorldFadeQueue::Advance(0x5000u, 0.25f, inactive) ==
              0x100cu && inactive.words[0x5270u] == 0x1000u &&
              inactive.effects.empty(),
          "inactive object is erased without material or script callbacks");
}

void InProgressAndExactBoundary() {
    Runtime ongoing;
    SetQueue(ongoing, 1u);
    SetEntry(ongoing, 0u, 0x2000u, 2.0f, 0.5f, true);
    Check(pal4::GameWorldFadeQueue::Advance(0x5000u, 0.5f, ongoing) ==
              0xdeadu && ongoing.words[0x1008u] ==
                             std::bit_cast<std::uint32_t>(1.0f) &&
              ongoing.words[0x5270u] == 0x100cu &&
              ongoing.effects == std::vector<Event>{
                  {"material", 0x2080u, 0x80808080u}},
          "halfway fade stores elapsed and repeats the rounded alpha byte");

    Runtime boundary;
    SetQueue(boundary, 1u);
    SetEntry(boundary, 0u, 0x2000u, 1.0f, 0.5f, true);
    Check(pal4::GameWorldFadeQueue::Advance(0x5000u, 0.5f, boundary) ==
              0xdeadu && boundary.words[0x5270u] == 0x100cu &&
              boundary.effects == std::vector<Event>{
                  {"material", 0x2080u, 0u}},
          "elapsed equal to duration stays queued until it exceeds duration");

    Runtime negative;
    SetQueue(negative, 1u);
    SetEntry(negative, 0u, 0x2000u, 1.0f, -1.5f, true);
    Check(pal4::GameWorldFadeQueue::Advance(0x5000u, 0.5f, negative) ==
              0xdeadu && negative.effects == std::vector<Event>{
                  {"material", 0x2080u, 0xfefefefeu}},
          "64-bit truncation still uses only the low byte for color");
}

void CompletionAndCompaction() {
    Runtime completed;
    SetQueue(completed, 1u);
    SetEntry(completed, 0u, 0x2000u, 1.0f, 0.5f, true);
    Check(pal4::GameWorldFadeQueue::Advance(0x5000u, 0.75f,
                                              completed) == 0x100cu &&
              completed.words[0x1008u] ==
                  std::bit_cast<std::uint32_t>(1.0f) &&
              completed.words[0x5270u] == 0x1000u &&
              completed.effects == std::vector<Event>{
                  {"material", 0x2080u, 0u},
                  {"material", 0x2080u, 0u},
                  {"remove3", 0x5000u, 0x2000u}},
          "completed fade clamps elapsed, writes material twice and removes");

    Runtime mixed;
    SetQueue(mixed, 3u);
    SetEntry(mixed, 0u, 0x2000u, 1.0f, 0.0f, false);
    SetEntry(mixed, 1u, 0x3000u, 1.0f, 0.5f, true);
    SetEntry(mixed, 2u, 0x4000u, 2.0f, 0.0f, true);
    Check(pal4::GameWorldFadeQueue::Advance(0x5000u, 0.75f, mixed) ==
              0xdeadu && mixed.words[0x5270u] == 0x100cu &&
              mixed.words[0x1000u] == 0x4000u &&
              mixed.words[0x1004u] ==
                  std::bit_cast<std::uint32_t>(2.0f) &&
              mixed.words[0x1008u] ==
                  std::bit_cast<std::uint32_t>(0.75f) &&
              mixed.effects == std::vector<Event>{
                  {"material", 0x3080u, 0u},
                  {"material", 0x3080u, 0u},
                  {"remove3", 0x5000u, 0x3000u},
                  {"material", 0x4080u, 0x9f9f9f9fu}},
          "successive removals shift all twelve-byte records without skip");
}

void FadeInQueue() {
    Runtime empty;
    SetInQueue(empty, 0u);
    Check(pal4::GameWorldFadeQueue::AdvanceIn(0x5000u, 0.5f, empty) ==
              0x1000u && empty.effects.empty(),
          "empty fade-in queue returns its own end pointer");

    Runtime ongoing;
    SetInQueue(ongoing, 1u);
    SetEntry(ongoing, 0u, 0x2000u, 2.0f, 0.0f, false);
    Check(pal4::GameWorldFadeQueue::AdvanceIn(0x5000u, 0.5f, ongoing) ==
              0xdeadu && ongoing.words[0x1008u] ==
                             std::bit_cast<std::uint32_t>(0.5f) &&
              ongoing.words[0x5280u] == 0x100cu &&
              ongoing.effects == std::vector<Event>{
                  {"material", 0x2080u, 0x40404040u}},
          "fade-in ignores fade-out active flag and raises material color");

    Runtime boundary;
    SetInQueue(boundary, 1u);
    SetEntry(boundary, 0u, 0x2000u, 1.0f, 0.5f, true);
    Check(pal4::GameWorldFadeQueue::AdvanceIn(0x5000u, 0.5f, boundary) ==
              0xdeadu && boundary.words[0x5280u] == 0x100cu &&
              boundary.effects == std::vector<Event>{
                  {"material", 0x2080u, 0xffffffffu}},
          "exact duration does not remove fade-in record");

    Runtime complete;
    SetInQueue(complete, 1u);
    SetEntry(complete, 0u, 0x2000u, 1.0f, 0.5f, true);
    Check(pal4::GameWorldFadeQueue::AdvanceIn(0x5000u, 0.75f,
                                                complete) == 0x100cu &&
              complete.words[0x5280u] == 0x1000u &&
              complete.words[0x1008u] ==
                  std::bit_cast<std::uint32_t>(1.0f) &&
              complete.effects == std::vector<Event>{
                  {"material", 0x2080u, 0xffffffffu}},
          "completed fade-in clamps, writes material once and erases silently");

    Runtime mixed;
    SetInQueue(mixed, 2u);
    SetEntry(mixed, 0u, 0x2000u, 1.0f, 0.75f, true);
    SetEntry(mixed, 1u, 0x3000u, 2.0f, 0.0f, true);
    Check(pal4::GameWorldFadeQueue::AdvanceIn(0x5000u, 0.5f, mixed) ==
              0xdeadu && mixed.words[0x5280u] == 0x100cu &&
              mixed.words[0x1000u] == 0x3000u &&
              mixed.words[0x1008u] ==
                  std::bit_cast<std::uint32_t>(0.5f) &&
              mixed.effects == std::vector<Event>{
                  {"material", 0x2080u, 0xffffffffu},
                  {"material", 0x3080u, 0x40404040u}},
          "fade-in compaction reprocesses shifted successor without script");
}

void SecondaryFadeOutQueue() {
    Runtime empty;
    SetSecondaryQueue(empty, 0u);
    Check(pal4::GameWorldFadeQueue::AdvanceOutSecondary(
              0x5000u, 0.5f, empty) == 0x1000u && empty.effects.empty(),
          "empty secondary fade queue returns its own end pointer");

    Runtime ongoing;
    SetSecondaryQueue(ongoing, 1u);
    SetEntry(ongoing, 0u, 0x2000u, 2.0f, 0.5f, false);
    Check(pal4::GameWorldFadeQueue::AdvanceOutSecondary(
              0x5000u, 0.5f, ongoing) == 0xdeadu &&
              ongoing.words[0x1008u] ==
                  std::bit_cast<std::uint32_t>(1.0f) &&
              ongoing.words[0x5290u] == 0x100cu &&
              ongoing.effects == std::vector<Event>{
                  {"material", 0x2080u, 0x80808080u}},
          "secondary fade ignores object activity and advances material");

    Runtime boundary;
    SetSecondaryQueue(boundary, 1u);
    SetEntry(boundary, 0u, 0x2000u, 1.0f, 0.5f, true);
    Check(pal4::GameWorldFadeQueue::AdvanceOutSecondary(
              0x5000u, 0.5f, boundary) == 0xdeadu &&
              boundary.words[0x5290u] == 0x100cu &&
              boundary.effects == std::vector<Event>{
                  {"material", 0x2080u, 0u}},
          "secondary fade retains record at exact duration");

    Runtime mixed;
    SetSecondaryQueue(mixed, 2u);
    SetEntry(mixed, 0u, 0x2000u, 1.0f, 0.75f, true);
    SetEntry(mixed, 1u, 0x3000u, 2.0f, 0.0f, true);
    Check(pal4::GameWorldFadeQueue::AdvanceOutSecondary(
              0x5000u, 0.5f, mixed) == 0xdeadu &&
              mixed.words[0x5290u] == 0x100cu &&
              mixed.words[0x1000u] == 0x3000u &&
              mixed.words[0x1008u] ==
                  std::bit_cast<std::uint32_t>(0.5f) &&
              mixed.effects == std::vector<Event>{
                  {"material", 0x2080u, 0u},
                  {"material", 0x2080u, 0u},
                  {"remove5", 0x5000u, 0x2000u},
                  {"material", 0x3080u, 0xbfbfbfbfu}},
          "secondary completion uses remove5 and reprocesses shifted record");
}

void Pc24CompletionBoundary() {
    for (unsigned queue=0;queue<3u;++queue) {
        Runtime runtime;
        if(queue==0u) SetQueue(runtime,1u);
        else if(queue==1u) SetInQueue(runtime,1u);
        else SetSecondaryQueue(runtime,1u);
        SetEntry(runtime,0u,0x2000u,1.f,1.f,true);
        const auto advance=[&](float delta){
            if(queue==0u) (void)pal4::GameWorldFadeQueue::Advance(0x5000u,delta,runtime);
            else if(queue==1u) (void)pal4::GameWorldFadeQueue::AdvanceIn(0x5000u,delta,runtime);
            else (void)pal4::GameWorldFadeQueue::AdvanceOutSecondary(0x5000u,delta,runtime);
        };
        const auto end=0x5270u+16u*queue;
        advance(0x1p-25f);
        Check(runtime.words[end]==0x100cu && runtime.words[0x1008u]==std::bit_cast<unsigned>(1.f) &&
              runtime.effects.size()==1u,"PC24 sub-half-ULP increment rounds to duration and stays queued");
        advance(0x1p-22f);
        Check(runtime.words[end]==0x1000u,"PC24 representable increment beyond duration completes");
    }
}

void NativeStub() {
    pal4::GameWorldFadeRuntimeStub runtime;
    Check(pal4::GameWorldFadeQueue::Advance(0x5000u, 0.25f, runtime) == 0u,
          "empty native-service stub returns without touching clumps");
    Check(pal4::GameWorldFadeQueue::AdvanceIn(0x5000u, 0.25f, runtime) == 0u,
          "fade-in native-service stub remains linkable");
    Check(pal4::GameWorldFadeQueue::AdvanceOutSecondary(
              0x5000u, 0.25f, runtime) == 0u,
          "secondary fade native-service stub remains linkable");
}

}  // namespace

int main() {
    try {
        EmptyAndInactive();
        InProgressAndExactBoundary();
        CompletionAndCompaction();
        FadeInQueue();
        SecondaryFadeOutQueue();
        Pc24CompletionBoundary();
        NativeStub();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
