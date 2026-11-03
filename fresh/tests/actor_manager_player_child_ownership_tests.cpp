#include "pal4/actor_manager_player_child_ownership.h"

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

constexpr std::uint32_t kActor = 0x1000u;
constexpr std::uint32_t kSlot = kActor + 1312u;

struct Runtime final : pal4::IActorManagerOwnedSlotRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> trace;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == kSlot + 4u, "child pointer offset");
        trace.emplace_back("read_word");
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        Check(address == kSlot + 4u, "child pointer write offset");
        trace.emplace_back("write_word");
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == kSlot, "ownership byte offset");
        trace.emplace_back("read_owner");
        return bytes[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == kSlot, "ownership byte write offset");
        trace.emplace_back("write_owner");
        bytes[address] = value;
    }
    void DeleteActor(std::uint32_t child, std::uint32_t flags) override {
        Check(flags == 1u, "virtual deleting destructor flag");
        trace.emplace_back("delete:" + std::to_string(child));
    }
    void Free(std::uint32_t) override {
        throw std::runtime_error("embedded slot must not be freed");
    }
};

}  // namespace

int main() {
    try {
        Runtime release;
        release.words[kSlot + 4u] = 0x2200u;
        pal4::ActorManagerOwnedSlot::Release(kSlot, release);
        Check(release.trace == std::vector<std::string>{"read_owner"} &&
                  release.words[kSlot + 4u] == 0x2200u,
              "unowned slot is retained");
        release.trace.clear();
        release.bytes[kSlot] = 1u;
        release.words[kSlot + 4u] = 0u;
        pal4::ActorManagerOwnedSlot::Release(kSlot, release);
        Check(release.trace == std::vector<std::string>{
                  "read_owner", "read_word"},
              "owned null slot has no deletion target");
        release.trace.clear();
        release.words[kSlot + 4u] = 0x2200u;
        pal4::ActorManagerOwnedSlot::Release(kSlot, release);
        Check(release.trace == std::vector<std::string>{
                  "read_owner", "read_word", "delete:8704"} &&
                  release.bytes[kSlot] == 1u &&
                  release.words[kSlot + 4u] == 0x2200u,
              "owned slot deletes object without clearing fields");

        Runtime replace;
        replace.words[kSlot + 4u] = 0x2200u;
        replace.bytes[kSlot] = 1u;
        Check(pal4::ActorManagerPlayerChildOwnership::Assign(
                  kActor, 0x3300u, replace) == kSlot &&
                  replace.words[kSlot + 4u] == 0x3300u &&
                  replace.bytes[kSlot] == 1u &&
                  replace.trace == std::vector<std::string>{
                      "read_word", "read_owner", "delete:8704",
                      "write_owner", "write_word"},
              "owned child is deleted before replacement");

        Runtime borrowed;
        borrowed.words[kSlot + 4u] = 0x2200u;
        Check(pal4::ActorManagerPlayerChildOwnership::Assign(
                  kActor, 0x3300u, borrowed) == kSlot &&
                  borrowed.trace == std::vector<std::string>{
                      "read_word", "read_owner", "write_owner", "write_word"},
              "borrowed child is not deleted");

        Runtime same;
        same.words[kSlot + 4u] = 0x2200u;
        Check(pal4::ActorManagerPlayerChildOwnership::Assign(
                  kActor, 0x2200u, same) == kSlot && same.bytes[kSlot] == 1u &&
                  same.trace == std::vector<std::string>{
                      "read_word", "write_owner", "write_word"},
              "same child acquires ownership without deletion");

        Runtime clear;
        clear.words[kSlot + 4u] = 0x2200u;
        clear.bytes[kSlot] = 1u;
        Check(pal4::ActorManagerPlayerChildOwnership::Assign(
                  kActor, 0u, clear) == kSlot &&
                  clear.words[kSlot + 4u] == 0u && clear.bytes[kSlot] == 0u &&
                  clear.trace == std::vector<std::string>{
                      "read_word", "read_owner", "delete:8704",
                      "write_owner", "write_word"},
              "null assignment releases prior owned child");

        Runtime empty;
        Check(pal4::ActorManagerPlayerChildOwnership::Assign(
                  kActor, 0u, empty) == kSlot &&
                  empty.trace ==
                      std::vector<std::string>{"read_word", "write_word"},
              "null-to-null assignment leaves owner flag alone");
        std::cout << "actor_manager_player_child_ownership_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
