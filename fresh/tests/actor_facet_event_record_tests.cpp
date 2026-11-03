#include "pal4/actor_facet_event_record.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kRecord = 0x4000u;
constexpr std::uint32_t kActor = 0x5000u;
using Write = std::pair<std::uint32_t, std::uint32_t>;

struct Runtime final : pal4::IActorFacetEventRecordRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<Write> writes;
    std::vector<Write> resets;
    bool throw_on_reset = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        writes.emplace_back(address, value);
    }
    void ResetTemporaryHandle(std::uint32_t& first,
                              std::uint32_t& second) override {
        resets.emplace_back(first, second);
        first = 0xffffffffu;
        second = 0u;
        if (throw_on_reset) throw std::runtime_error("reset failed");
    }
};

void BaseAndDecode() {
    Runtime runtime;
    Check(pal4::ActorFacetEventRecord::ConstructRaw(
              kRecord, 0x1234u, runtime) == kRecord &&
          runtime.writes == std::vector<Write>{{kRecord, 0x1234u}},
          "raw constructor writes only first word and returns this");
    runtime.writes.clear();
    Check(pal4::ActorFacetEventRecord::ConstructFacet(
              kRecord, kActor, runtime) == kRecord &&
          runtime.writes == std::vector<Write>{{kRecord, kActor + 716u}} &&
          pal4::ActorFacetEventRecord::ActorFromFacet(
              kRecord, runtime) == kActor,
          "actor constructor stores +716 facet and decoder reverses it");
    runtime.writes.clear();
    Check(pal4::ActorFacetEventRecord::ConstructFacet(
              kRecord, 0u, runtime) == kRecord &&
          runtime.writes == std::vector<Write>{{kRecord, 0u}} &&
          pal4::ActorFacetEventRecord::ActorFromFacet(
              kRecord, runtime) == 0u,
          "null actor remains a null facet and decodes to null");
    runtime.words[kRecord] = 1u;
    Check(pal4::ActorFacetEventRecord::ActorFromFacet(
              kRecord, runtime) == 1u - 716u,
          "facet subtraction preserves 32-bit wraparound");
}

void ConstructorShapes() {
    Runtime runtime;
    Check(pal4::ActorFacetEventRecord::ConstructFourWords(
              kRecord, kActor, 11u, 12u, 13u, runtime) == kRecord &&
          runtime.writes == std::vector<Write>{
              {kRecord, kActor + 716u}, {kRecord + 12u, 13u},
              {kRecord + 4u, 11u}, {kRecord + 8u, 12u}},
          "four-word constructor writes mode before payload pair");

    runtime = Runtime{};
    Check(pal4::ActorFacetEventRecord::ConstructFixedMode(
              kRecord, kActor, runtime) == kRecord &&
          runtime.writes == std::vector<Write>{
              {kRecord, kActor + 716u}, {kRecord + 12u, 6u},
              {kRecord + 4u, 0u}, {kRecord + 8u, 0u}},
          "fixed-mode constructor writes 6 and zeros the pair");

    runtime = Runtime{};
    runtime.words[kRecord + 12u] = 99u;
    Check(pal4::ActorFacetEventRecord::ConstructPair(
              kRecord, kActor, 21u, 22u, runtime) == kRecord &&
          runtime.writes == std::vector<Write>{
              {kRecord, kActor + 716u},
              {kRecord + 4u, 21u}, {kRecord + 8u, 22u}} &&
          runtime.words[kRecord + 12u] == 99u,
          "pair constructor does not touch fourth word");

    runtime = Runtime{};
    runtime.words[kRecord + 8u] = 77u;
    Check(pal4::ActorFacetEventRecord::ConstructOneExtra(
              kRecord, kActor, 31u, runtime) == kRecord &&
          runtime.writes == std::vector<Write>{
              {kRecord, kActor + 716u}, {kRecord + 4u, 31u}} &&
          runtime.words[kRecord + 8u] == 77u,
          "one-extra constructor preserves the remaining words");
}

void EffectHandleCopyAndReset() {
    Runtime runtime;
    Check(pal4::ActorFacetEventRecord::ConstructWithEffectHandle(
              kRecord, kActor, 41u, 42u, runtime) == kRecord &&
          runtime.writes == std::vector<Write>{
              {kRecord, kActor + 716u},
              {kRecord + 4u, 41u}, {kRecord + 8u, 42u}} &&
          runtime.resets == std::vector<Write>{{41u, 42u}} &&
          runtime.words[kRecord + 4u] == 41u &&
          runtime.words[kRecord + 8u] == 42u,
          "effect pair is copied before resetting by-value temporary");

    runtime = Runtime{};
    runtime.throw_on_reset = true;
    bool threw = false;
    try {
        (void)pal4::ActorFacetEventRecord::ConstructWithEffectHandle(
            kRecord, 0u, 51u, 52u, runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.writes == std::vector<Write>{
              {kRecord, 0u}, {kRecord + 4u, 51u},
              {kRecord + 8u, 52u}} &&
          runtime.resets == std::vector<Write>{{51u, 52u}},
          "reset failure happens after pair copy with null actor");
}

}  // namespace

int main() {
    try {
        BaseAndDecode();
        ConstructorShapes();
        EffectHandleCopyAndReset();
        std::cout << "actor_facet_event_record_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
