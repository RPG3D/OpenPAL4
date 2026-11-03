#include "pal4/script_skill_vector8.h"

#include <array>
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

struct Runtime final : pal4::IScriptSkillVector8Runtime {
    std::map<std::uint32_t, std::uint32_t> memory;
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x5000u;
    std::uint32_t allocation_size = 0u;
    std::vector<std::uint32_t> freed;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return memory[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write " + std::to_string(address) + " " +
                        std::to_string(value));
        memory[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        calls.push_back("allocate " + std::to_string(size));
        allocation_size = size;
        return allocation;
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free " + std::to_string(address));
        freed.push_back(address);
    }
};

void SetVector(Runtime& runtime, std::uint32_t begin,
               std::uint32_t size, std::uint32_t capacity) {
    runtime.memory[0x7004u] = begin;
    runtime.memory[0x7008u] = begin + size * 8u;
    runtime.memory[0x700cu] = begin + capacity * 8u;
    for (std::uint32_t i = 0u; i < size; ++i) {
        runtime.memory[begin + i * 8u] = i + 1u;
        runtime.memory[begin + i * 8u + 4u] = 100u + i;
    }
}

void CheckElements(const Runtime& runtime,
                   const std::vector<std::uint32_t>& keys,
                   std::uint32_t begin) {
    for (std::size_t i = 0u; i < keys.size(); ++i) {
        const auto found = runtime.memory.find(begin +
                          static_cast<std::uint32_t>(i) * 8u);
        Check(found != runtime.memory.end() && found->second == keys[i],
              "inserted vector has expected key order");
    }
}

void EmptyAndCapacityPaths() {
    Runtime empty;
    Check(pal4::ScriptSkillVector8::Insert(
              0x7000u, 0u, 1u, {9u, 99u}, empty) == 0x5008u &&
              empty.allocation_size == 8u &&
              empty.freed == std::vector<std::uint32_t>{0u} &&
              empty.memory[0x7004u] == 0x5000u &&
              empty.memory[0x7008u] == 0x5008u &&
              empty.memory[0x700cu] == 0x5008u &&
              empty.memory[0x5000u] == 9u &&
              empty.memory[0x5004u] == 99u,
          "empty vector allocates one element and updates all three pointers");

    Runtime short_insert;
    SetVector(short_insert, 0x3000u, 4u, 8u);
    Check(pal4::ScriptSkillVector8::Insert(
              0x7000u, 0x3008u, 1u, {9u, 99u}, short_insert) ==
              0x3028u &&
              short_insert.memory[0x7008u] == 0x3028u &&
              short_insert.allocation_size == 0u &&
              short_insert.freed.empty(),
          "short insert uses existing capacity");
    CheckElements(short_insert, {1u, 9u, 2u, 3u, 4u}, 0x3000u);
    Check(short_insert.memory[0x300cu] == 99u &&
              short_insert.memory[0x3024u] == 103u,
          "short insert preserves second words while shifting tail");

    Runtime long_insert;
    SetVector(long_insert, 0x3000u, 2u, 8u);
    Check(pal4::ScriptSkillVector8::Insert(
              0x7000u, 0x3008u, 3u, {9u, 99u}, long_insert) ==
              0x3028u &&
              long_insert.allocation_size == 0u,
          "insert longer than tail also uses existing capacity");
    CheckElements(long_insert, {1u, 9u, 9u, 9u, 2u}, 0x3000u);
    Check(long_insert.memory[0x3024u] == 101u,
          "long insert preserves moved tail value");
}

void GrowthAndFailure() {
    Runtime growing;
    SetVector(growing, 0x3000u, 2u, 2u);
    Check(pal4::ScriptSkillVector8::Insert(
              0x7000u, 0x3008u, 1u, {9u, 99u}, growing) ==
              0x5018u &&
              growing.allocation_size == 32u &&
              growing.freed == std::vector<std::uint32_t>{0x3000u} &&
              growing.memory[0x7004u] == 0x5000u &&
              growing.memory[0x7008u] == 0x5018u &&
              growing.memory[0x700cu] == 0x5020u,
          "full vector doubles capacity and updates header after old free");
    CheckElements(growing, {1u, 9u, 2u}, 0x5000u);
    Check(growing.memory[0x5014u] == 101u,
          "growth preserves trailing second word");

    Runtime failed;
    SetVector(failed, 0x3000u, 1u, 1u);
    failed.allocation = 0u;
    Check(pal4::ScriptSkillVector8::Insert(
              0x7000u, 0x3008u, 1u, {9u, 99u}, failed) == 0u &&
              failed.memory[0x7004u] == 0x3000u &&
              failed.memory[0x7008u] == 0x3008u &&
              failed.memory[0x700cu] == 0x3008u &&
              failed.freed.empty(),
          "allocation failure leaves old vector intact");
}

void ZeroAndInvalidInputs() {
    Runtime runtime;
    SetVector(runtime, 0x3000u, 2u, 2u);
    runtime.calls.clear();
    Check(pal4::ScriptSkillVector8::Insert(
              0x7000u, 0x3008u, 0u, {9u, 99u}, runtime) ==
              0x3010u &&
              runtime.memory[0x7008u] == 0x3010u &&
              runtime.allocation_size == 0u && runtime.freed.empty(),
          "zero-count insertion preserves vector state");
    for (auto position : {0x2ff8u, 0x3001u, 0x3018u}) {
        bool rejected = false;
        try {
            static_cast<void>(pal4::ScriptSkillVector8::Insert(
                0x7000u, position, 1u, {9u, 99u}, runtime));
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        Check(rejected && runtime.memory[0x7008u] == 0x3010u,
              "invalid position has no vector mutation");
    }
}

}  // namespace

int main() {
    try {
        EmptyAndCapacityPaths();
        GrowthAndFailure();
        ZeroAndInvalidInputs();
        std::cout << "script skill vector8 tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
