#include "pal4/pal_game_iv.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IPalGameIVRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t registrations = 0u;
    bool fail_allocation = false;
    bool throw_on_bone = false;
    bool throw_on_dictionary = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return static_cast<std::uint8_t>(
            words[address & ~3u] >> ((address & 3u) * 8u));
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        const auto slot = address & ~3u;
        const auto shift = (address & 3u) * 8u;
        words[slot] = (words[slot] & ~(0xffu << shift)) |
                      (std::uint32_t{value} << shift);
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == 444u, "game allocation size");
        calls.push_back("alloc");
        return fail_allocation ? 0u : 0x1000u;
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
    std::uint32_t SingletonManager() override {
        calls.push_back("singleton");
        return 0x4400u;
    }
    std::uint32_t RegisterSingleton(
        std::uint32_t manager, std::string_view name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t destructor) override {
        Check(manager == 0x4400u && name == "PALGameIV" && id == 1046u &&
                  factory == 0x5b5b30u && destructor == 0x5b5b90u,
              "singleton arguments");
        ++registrations;
        calls.push_back("register");
        return 0x1000u;
    }
    void InitializeVariables(std::uint32_t object) override {
        Check(object == 0x1000u, "variable initializer address");
        calls.push_back("variables");
    }
    void ConstructBone(std::uint32_t bone) override {
        Check(bone == 0x1178u, "bone address");
        calls.push_back("bone");
        if (throw_on_bone) throw std::runtime_error("bone");
    }
    void InitializeString(std::uint32_t string) override {
        Check(string == 0x1190u, "string address");
        calls.push_back("string-init");
    }
    void ConstructDictionary(std::uint32_t dictionary) override {
        Check(dictionary == 0x11acu, "dictionary address");
        calls.push_back("dictionary-init");
        if (throw_on_dictionary) throw std::runtime_error("dictionary");
    }
    void DestroyDictionary(std::uint32_t dictionary) override {
        Check(dictionary == 0x11acu, "dictionary destructor address");
        calls.push_back("dictionary-destroy");
    }
    void CleanupString(std::uint32_t string) override {
        calls.push_back("string-clean:" + std::to_string(string));
    }
    std::uint8_t ConstructorStackByte() override {
        calls.push_back("stack-byte");
        return 0x5au;
    }
};

void SingletonAndConstruct() {
    Runtime runtime;
    pal4::PalGameIVRegistration registration{0xaau, 0u};
    Check(pal4::PalGameIV::GetInstance(registration, runtime) == 0x1000u &&
              registration.guard == 0xabu && runtime.registrations == 1u,
          "singleton first registration");
    Check(pal4::PalGameIV::GetInstance(registration, runtime) == 0x1000u &&
              runtime.registrations == 1u, "singleton cached");
    runtime.calls.clear();
    Check(pal4::PalGameIV::Create(runtime) == 0x1000u, "game factory");
    Check(runtime.calls == std::vector<std::string>({
              "alloc", "stack-byte", "variables", "bone", "string-init",
              "dictionary-init"}), "construction order");
    Check(runtime.ReadByte(0x119cu) == 0x5au &&
              runtime.ReadWord(0x11a0u) == 0u &&
              runtime.ReadWord(0x11a4u) == 0u &&
              runtime.ReadWord(0x11a8u) == 0u &&
              runtime.ReadWord(0x1174u) == 0u &&
              runtime.ReadWord(0x1188u) == 0u &&
              runtime.ReadWord(0x118cu) == 0u, "construction fields");
}

void CleanupStrings() {
    Runtime runtime;
    constexpr auto object = 0x1000u;
    runtime.WriteWord(object + 344u, 0x2000u);
    runtime.WriteWord(object + 348u, 0x2010u);
    runtime.WriteWord(object + 352u, 0x2020u);
    runtime.WriteWord(object + 328u, 0x3010u);
    runtime.WriteByte(0x300fu, 0u);
    runtime.WriteWord(object + 312u, 0x4010u);
    runtime.WriteByte(0x400fu, 2u);
    runtime.WriteWord(object + 296u, 0x5010u);
    runtime.WriteByte(0x500fu, 0xffu);
    for (auto offset : {328u, 332u, 336u, 312u, 316u, 320u,
                        296u, 300u, 304u}) {
        if (runtime.ReadWord(object + offset) == 0u)
            runtime.WriteWord(object + offset, 0xdeadu);
    }
    pal4::PalGameIV::Cleanup(object, runtime);
    Check(runtime.calls == std::vector<std::string>({
              "free:8192", "free:12303", "free:20495"}),
          "cleanup free order");
    Check(runtime.ReadByte(0x400fu) == 1u, "retained string decrement");
    for (auto offset : {344u, 348u, 352u, 328u, 332u, 336u,
                        312u, 316u, 320u, 296u, 300u, 304u})
        Check(runtime.ReadWord(object + offset) == 0u, "cleanup reset fields");
    runtime.calls.clear();
    pal4::PalGameIV::Cleanup(object, runtime);
    Check(runtime.calls == std::vector<std::string>({"free:0"}),
          "empty cleanup");
}

void DestructOrderAndFactoryFailure() {
    Runtime runtime;
    const auto object = pal4::PalGameIV::Create(runtime);
    runtime.calls.clear();
    runtime.WriteWord(object + 0x1a0u, 0x6000u);
    runtime.WriteWord(object + 0x1a4u, 0x6024u);
    runtime.WriteWord(object + 0x1a8u, 0x6040u);
    pal4::PalGameIV::Destroy(object, runtime);
    Check(runtime.calls == std::vector<std::string>({
              "dictionary-destroy", "string-clean:24576",
              "string-clean:24588", "string-clean:24600", "free:24576",
              "string-clean:4496", "free:0", "free:4096"}),
          "destructor order");
    Check(runtime.ReadWord(object + 0x1a0u) == 0u &&
              runtime.ReadWord(object + 0x1a4u) == 0u &&
              runtime.ReadWord(object + 0x1a8u) == 0u,
          "destructor resets vector");
    runtime.calls.clear();
    pal4::PalGameIV::Destroy(0u, runtime);
    Check(runtime.calls.empty(), "null destruction");

    Runtime fail;
    fail.fail_allocation = true;
    Check(pal4::PalGameIV::Create(fail) == 0u &&
              fail.calls == std::vector<std::string>({"alloc"}),
          "factory allocation failure");
    Runtime throw_runtime;
    throw_runtime.throw_on_bone = true;
    try {
        (void)pal4::PalGameIV::Create(throw_runtime);
        Check(false, "constructor failure must throw");
    } catch (const std::runtime_error&) {
        Check(throw_runtime.calls == std::vector<std::string>({
                  "alloc", "stack-byte", "variables", "bone", "free:0",
                  "free:4096"}), "early constructor rollback");
    }
    Runtime dictionary_failure;
    dictionary_failure.throw_on_dictionary = true;
    try {
        (void)pal4::PalGameIV::Create(dictionary_failure);
        Check(false, "dictionary failure must throw");
    } catch (const std::runtime_error&) {
        Check(dictionary_failure.calls == std::vector<std::string>({
                  "alloc", "stack-byte", "variables", "bone", "string-init",
                  "dictionary-init", "free:0", "string-clean:4496",
                  "free:0", "free:4096"}),
              "dictionary constructor rollback");
    }
}

}  // namespace

int main() {
    try {
        SingletonAndConstruct();
        CleanupStrings();
        DestructOrderAndFactoryFailure();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
