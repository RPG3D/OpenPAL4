#include "pal4/actor_manager_monster_iterator.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using pal4::ActorManagerMonsterIterator;
constexpr std::uint32_t source = 0x1000u;
constexpr std::uint32_t iterator = 0x2000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IActorManagerMonsterIteratorRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> trace;
    std::uint32_t allocation = iterator;

    std::uint32_t ReadWord(std::uint32_t address) override {
        trace.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        trace.emplace_back("write:" + std::to_string(address) + ":" +
                           std::to_string(value));
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 12u, "iterator allocation size");
        trace.emplace_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t address) override {
        Check(address == iterator, "iterator free address");
        trace.emplace_back("free");
    }
};

void CloneAndDelete() {
    Runtime runtime;
    runtime.words[source + 4u] = 0x3000u;
    runtime.words[source + 8u] = 0x4000u;
    Check(ActorManagerMonsterIterator::Clone(source, runtime) == iterator &&
              runtime.words[iterator] ==
                  ActorManagerMonsterIterator::kDerivedVtable &&
              runtime.words[iterator + 4u] == 0x3000u &&
              runtime.words[iterator + 8u] == 0x4000u &&
              runtime.trace == std::vector<std::string>{
                  "allocate", "write:8192:8671172", "read:4100",
                  "write:8196:12288", "read:4104", "write:8200:16384",
                  "write:8192:8671128"},
          "clone uses base vtable during field copy then derived vtable");

    for (const auto flags : std::array<std::uint8_t, 4>{0u, 1u, 2u, 3u}) {
        Runtime deleting;
        Check(ActorManagerMonsterIterator::Delete(
                  iterator, flags, deleting) == iterator &&
                  deleting.words[iterator] ==
                      ActorManagerMonsterIterator::kBaseVtable &&
                  deleting.trace.size() == (flags & 1u ? 2u : 1u) &&
                  (!(flags & 1u) || deleting.trace.back() == "free"),
              "delete restores base vtable and honors low flag bit");
    }

    Runtime failed;
    failed.allocation = 0u;
    Check(ActorManagerMonsterIterator::Clone(source, failed) == 0u &&
              failed.trace == std::vector<std::string>{"allocate"},
          "allocation failure returns null without writes");
}

}  // namespace

int main() {
    try {
        CloneAndDelete();
        std::cout << "actor_manager_monster_iterator_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
