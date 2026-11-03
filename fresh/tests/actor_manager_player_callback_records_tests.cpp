#include "pal4/actor_manager_player_callback_records.h"

#include <array>
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

using Records = pal4::ActorManagerPlayerCallbackRecords;
constexpr std::uint32_t kObject = 0x1000u;

struct Runtime final : pal4::IActorManagerPlayerCallbackRecordRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;
    std::vector<std::uint32_t> frees;

    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        writes.emplace_back(address, value);
    }
    void Free(std::uint32_t address) override {
        frees.push_back(address);
    }
};

using DestroyFunction = std::uint32_t (*)(
    std::uint32_t, std::uint32_t,
    pal4::IActorManagerPlayerCallbackRecordRuntime&);

void CheckDestroy(DestroyFunction destroy, std::uint32_t expected_vtable) {
    for (const auto flags : {0u, 1u, 2u, 3u}) {
        Runtime runtime;
        runtime.words[kObject + 4u] = 0x88u;
        Check(destroy(kObject, flags, runtime) == kObject &&
                  runtime.writes ==
                      std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                          {kObject, expected_vtable}} &&
                  runtime.words[kObject + 4u] == 0x88u &&
                  runtime.frees ==
                      ((flags & 1u) != 0u
                           ? std::vector<std::uint32_t>{kObject}
                           : std::vector<std::uint32_t>{}),
              "deleting flag controls free after the correct vtable write");
    }
}

}  // namespace

int main() {
    try {
        Runtime construct;
        construct.words[kObject + 4u] = 0x99u;
        Check(Records::ConstructSetter(kObject, 0x2200u, 0x3300u,
                                        construct) == kObject &&
                  construct.writes ==
                      std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                          {kObject, Records::kSetterVtable},
                          {kObject + 8u, 0x2200u},
                          {kObject + 12u, 0x3300u}} &&
                  construct.words[kObject + 4u] == 0x99u &&
                  construct.frees.empty(),
              "setter constructor writes only observed fields");

        Runtime first;
        Records::MarkFirst(kObject, first);
        Check(first.writes ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {kObject, Records::kFirstVtable}},
              "first marker writes one vtable");
        Runtime second;
        Records::MarkSecond(kObject, second);
        Check(second.writes ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {kObject, Records::kSecondVtable}},
              "second marker writes one vtable");

        CheckDestroy(&Records::DestroyFirst, Records::kFirstVtable);
        CheckDestroy(&Records::DestroySecond, Records::kSecondVtable);
        CheckDestroy(&Records::DestroyFirstInline, Records::kFirstVtable);
        CheckDestroy(&Records::DestroySecondInline, Records::kSecondVtable);
        std::cout << "actor_manager_player_callback_records_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
