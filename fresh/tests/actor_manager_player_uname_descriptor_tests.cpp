#include "pal4/actor_manager_player_uname_descriptor.h"

#include <algorithm>
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

constexpr std::uint32_t kDescriptor = 0x1000u;
constexpr std::uint32_t kTextBuffer = 0x5001u;

struct Runtime final : pal4::IActorManagerStringMethodRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> trace;
    std::vector<std::uint32_t> deleted;
    std::vector<std::uint32_t> freed;
    bool refill_during_text_free = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        if (address == kDescriptor)
            trace.emplace_back("vtable:" + std::to_string(value));
        else if (address == kDescriptor + 8u ||
                 address == kDescriptor + 16u)
            trace.emplace_back("clear_object:" + std::to_string(address));
        else if (address == kDescriptor + 24u)
            trace.emplace_back("clear_text");
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    void DeleteObject(std::uint32_t object,
                      std::uint32_t flags) override {
        Check(flags == 1u, "virtual deleting destructor flag");
        deleted.push_back(object);
        trace.emplace_back("delete:" + std::to_string(object));
    }
    void Free(std::uint32_t header) override {
        freed.push_back(header);
        trace.emplace_back("free_text");
        if (refill_during_text_free) {
            bytes[kDescriptor + 4u] = 1u;
            words[kDescriptor + 8u] = 0x700u;
            bytes[kDescriptor + 12u] = 1u;
            words[kDescriptor + 16u] = 0x800u;
        }
    }
};

void Prepare(Runtime& runtime) {
    runtime.bytes[kDescriptor + 4u] = 1u;
    runtime.words[kDescriptor + 8u] = 0x300u;
    runtime.bytes[kDescriptor + 12u] = 1u;
    runtime.words[kDescriptor + 16u] = 0x400u;
    runtime.words[kDescriptor + 24u] = kTextBuffer;
    runtime.words[kDescriptor + 28u] = 5u;
    runtime.words[kDescriptor + 32u] = 8u;
}

}  // namespace

int main() {
    try {
        Runtime owned;
        Prepare(owned);
        pal4::ActorManagerPlayerUNameDescriptor::Destroy(kDescriptor, owned);
        Check(owned.deleted == std::vector<std::uint32_t>{0x300u, 0x400u} &&
                  owned.freed ==
                      std::vector<std::uint32_t>{kTextBuffer - 1u} &&
                  owned.words[kDescriptor] ==
                      pal4::ActorManagerPlayerUNameDescriptor::kBaseVtable &&
                  owned.words[kDescriptor + 8u] == 0u &&
                  owned.words[kDescriptor + 16u] == 0u &&
                  owned.words[kDescriptor + 24u] == 0u &&
                  owned.words[kDescriptor + 28u] == 0u &&
                  owned.words[kDescriptor + 32u] == 0u &&
                  owned.trace.front() == "vtable:8671848" &&
                  owned.trace.back() == "vtable:8661972",
              "owned slots and text release in destructor order");

        Runtime borrowed;
        Prepare(borrowed);
        borrowed.bytes[kDescriptor + 4u] = 0u;
        borrowed.bytes[kDescriptor + 12u] = 0u;
        borrowed.bytes[kTextBuffer - 1u] = 2u;
        pal4::ActorManagerPlayerUNameDescriptor::Destroy(kDescriptor,
                                                           borrowed);
        Check(borrowed.deleted.empty() && borrowed.freed.empty() &&
                  borrowed.bytes[kTextBuffer - 1u] == 1u,
              "borrowed objects remain and shared text decrements");

        Runtime sentinel;
        Prepare(sentinel);
        sentinel.bytes[kTextBuffer - 1u] = 0xffu;
        pal4::ActorManagerPlayerUNameDescriptor::Destroy(kDescriptor,
                                                           sentinel);
        Check(sentinel.freed ==
                  std::vector<std::uint32_t>{kTextBuffer - 1u},
              "text sentinel is freed");

        Runtime refilled;
        Prepare(refilled);
        refilled.refill_during_text_free = true;
        pal4::ActorManagerPlayerUNameDescriptor::Destroy(kDescriptor,
                                                           refilled);
        Check(refilled.deleted == std::vector<std::uint32_t>{
                  0x300u, 0x400u, 0x700u, 0x800u} &&
                  refilled.words[kDescriptor + 8u] == 0u &&
                  refilled.words[kDescriptor + 16u] == 0u,
              "second clear handles objects added during text release");

        Runtime empty;
        pal4::ActorManagerPlayerUNameDescriptor::Destroy(kDescriptor, empty);
        Check(empty.deleted.empty() && empty.freed.empty() &&
                  empty.words[kDescriptor] ==
                      pal4::ActorManagerPlayerUNameDescriptor::kBaseVtable,
              "empty descriptor completes both vtable transitions");
        std::cout << "actor_manager_player_uname_descriptor_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
