#include "pal4/combat_translator.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kObject = 0x1000u;
constexpr std::uint32_t kStorage = 0x2000u;
constexpr std::uint32_t kBackend = 0x2100u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatTranslatorRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t allocated = kStorage;
    std::uint32_t constructed = kBackend;
    std::uint32_t found = 0x3000u;
    std::uint8_t loaded = 1u;
    bool clear_object_pointer = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write:" + std::to_string(address));
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 32u, "translator allocates 32-byte backend");
        calls.emplace_back("allocate");
        return allocated;
    }
    std::uint32_t ConstructBackend(std::uint32_t storage) override {
        Check(storage == kStorage, "backend constructor receives allocation");
        calls.emplace_back("construct-backend");
        return constructed;
    }
    std::uint8_t LoadBackend(std::uint32_t backend,
                             std::uint32_t input,
                             std::uint32_t adapter_vtable) override {
        Check(backend == kBackend && input == 0x4000u &&
                  adapter_vtable == 0x84488cu,
              "translator load forwards backend, input and adapter vtable");
        calls.emplace_back("load-backend");
        return loaded;
    }
    std::uint32_t FindNamedBackend(
        std::uint32_t backend, std::string_view name) override {
        Check(backend == kBackend && name == "CombatTranslation",
              "translator looks up exact IDA section name");
        calls.emplace_back("find-name");
        return found;
    }
    std::uint32_t ClearBackend(std::uint32_t backend) override {
        Check(backend == kBackend, "backend clear receives current pointer");
        calls.emplace_back("clear-backend");
        if (clear_object_pointer) words[kObject] = 0u;
        return 0x6000u;
    }
    void DestroyBackend(std::uint32_t backend) override {
        Check(backend == kBackend, "backend destroy receives current pointer");
        calls.emplace_back("destroy-backend");
    }
    std::uint32_t Free(std::uint32_t address) override {
        Check(address == kBackend, "backend allocation is freed");
        calls.emplace_back("free");
        return 0x7000u;
    }
};

void Construction() {
    Runtime runtime;
    Check(pal4::CombatTranslator::Construct(kObject, runtime) == kObject &&
              runtime.words[kObject] == kBackend &&
              runtime.words[kObject + 4u] == 0u &&
              runtime.words[kObject + 8u] == 0u &&
              runtime.words[kObject + 12u] == 0u &&
              runtime.calls[0] == "allocate" &&
              runtime.calls[1] == "construct-backend",
          "constructor stores backend then clears remaining fields");

    Runtime failure;
    failure.allocated = 0u;
    Check(pal4::CombatTranslator::Construct(kObject, failure) == kObject &&
              failure.words[kObject] == 0u &&
              failure.calls.size() == 5u &&
              failure.calls[0] == "allocate",
          "allocation failure skips backend constructor and clears object");
}

void Loading() {
    Runtime runtime;
    runtime.words[kObject] = kBackend;
    runtime.words[kObject + 4u] = 0x1111u;
    runtime.loaded = 0u;
    Check(pal4::CombatTranslator::Load(
              kObject, 0x4000u, runtime) == 0u &&
              runtime.words[kObject + 4u] == 0x1111u &&
              runtime.calls == std::vector<std::string>{"load-backend"},
          "failed backend load preserves prior translation pointer");

    runtime.calls.clear();
    runtime.loaded = 1u;
    Check(pal4::CombatTranslator::Load(
              kObject, 0x4000u, runtime) == 1u &&
              runtime.words[kObject + 4u] == 0x3000u &&
              runtime.calls == std::vector<std::string>{
                  "load-backend", "find-name", "write:4100"},
          "successful load selects CombatTranslation and stores pointer");

    runtime.calls.clear();
    runtime.found = 0u;
    Check(pal4::CombatTranslator::Load(
              kObject, 0x4000u, runtime) == 0u &&
              runtime.words[kObject + 4u] == 0u,
          "loaded backend without named translation reports failure");
}

void DestructionAndThunk() {
    Runtime runtime;
    runtime.words[kObject] = kBackend;
    runtime.words[kObject + 4u] = 0x3000u;
    Check(pal4::CombatTranslator::DestructThunk(
              kObject, runtime) == 0x7000u &&
              runtime.words[kObject] == 0u &&
              runtime.words[kObject + 4u] == 0u &&
              runtime.calls == std::vector<std::string>{
                  "write:4100", "clear-backend", "destroy-backend",
                  "free", "write:4096"},
          "thunk forwards destructor cleanup and free result");

    Runtime cleared;
    cleared.words[kObject] = kBackend;
    cleared.clear_object_pointer = true;
    Check(pal4::CombatTranslator::Destruct(
              kObject, cleared) == 0x6000u &&
              cleared.calls == std::vector<std::string>{
                  "write:4100", "clear-backend", "write:4096"},
          "cleared backend pointer skips final destroy/free");

    Runtime empty;
    Check(pal4::CombatTranslator::Destruct(
              kObject, empty) == 0u && empty.calls.empty(),
          "empty translator destructor has no side effects");
}

}  // namespace

int main() {
    try {
        Construction();
        Loading();
        DestructionAndThunk();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
