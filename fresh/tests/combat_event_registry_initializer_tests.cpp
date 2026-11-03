#include "pal4/combat_event_data_registry.h"
#include "pal4/combat_event_registry_callbacks.h"
#include "pal4/combat_event_registry_initializer.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct ObjectCall {
    std::uint32_t object;
    std::uint32_t kind;
    std::uint32_t game_element;
    std::uint32_t data_element;
};

struct Runtime final : pal4::ICombatEventRegistryInitRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> guards;
    std::vector<std::string> calls;
    std::vector<std::string> names;
    std::vector<ObjectCall> objects;
    std::vector<std::uint32_t> callbacks;

    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("word " + std::to_string(address) + " " +
                        std::to_string(value));
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("guard-read " + std::to_string(address));
        return guards[address];
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.push_back("guard-write " + std::to_string(address) + " " +
                        std::to_string(value));
        guards[address] = value;
    }
    std::uint32_t GameCollection() override {
        calls.push_back("game");
        return 11u;
    }
    std::uint32_t DataCollection() override {
        calls.push_back("data");
        return 22u;
    }
    std::uint32_t FindElementByName(
        std::uint32_t collection, std::string_view name) override {
        const std::string key(name);
        calls.push_back("find " + std::to_string(collection) + " " + key);
        names.push_back(key);
        return collection == 11u ? 111u : 222u;
    }
    void InitializeObject(std::uint32_t object,
                          std::uint32_t game_element,
                          std::uint32_t data_element,
                          std::uint32_t kind) override {
        calls.push_back("init " + std::to_string(object) + " " +
                        std::to_string(kind));
        objects.push_back({object, kind, game_element, data_element});
    }
    std::int32_t RegisterAtExit(std::uint32_t callback) override {
        if (pal4::CombatEventRegistryCallbacks::Resolve(callback) == nullptr)
            throw std::runtime_error("unmapped IDA callback");
        calls.push_back("atexit " + std::to_string(callback));
        callbacks.push_back(callback);
        return -17;
    }
};

constexpr auto kBase = pal4::CombatEventDataRegistry::kBase;

void FirstInitialization() {
    Runtime runtime;
    runtime.words[kBase + 78u * 4u] = 0xdeadbeefu;
    Check(pal4::CombatEventRegistryInitializer::Initialize(runtime) == -17 &&
              runtime.objects.size() == 77u &&
              runtime.callbacks.size() == 77u &&
              runtime.names.size() == 154u,
          "77 guarded objects initialize after two name lookups each");
    Check(runtime.calls[0] == "word 9414864 0" &&
              runtime.calls[77] == "word 9415172 0" &&
              runtime.calls[78] == "game" &&
              runtime.calls[79] == "data" &&
              runtime.calls[80] == "find 11 AI_TYPE" &&
              runtime.calls[81] == "find 22 AI_TYPE" &&
              runtime.words[kBase] == 0u &&
              runtime.words[kBase + 78u * 4u] == 0xdeadbeefu,
          "exactly 78 table words are cleared before collection acquisition");
    Check(runtime.callbacks.front() == 0x583540u &&
              runtime.callbacks.back() == 0x583080u &&
              runtime.objects.front().object == 0x8fa8bcu &&
              runtime.objects.front().kind == 1u &&
              runtime.objects.front().game_element == 111u &&
              runtime.objects.front().data_element == 222u &&
              runtime.objects.back().object == 0x8fa3d8u,
          "callback order and object arguments match IDA registration blocks");
    for (std::size_t i = 0; i < runtime.callbacks.size(); ++i)
        Check(runtime.callbacks[i] == 0x583540u - i * 16u,
              "all callback addresses follow the IDA sequence");

    Check(runtime.words[kBase + 1u * 4u] == 0x8fa8acu &&
              runtime.words[kBase + 20u * 4u] == 0x8fa6acu &&
              runtime.words[kBase + 21u * 4u] == 0x8fa774u &&
              runtime.words[kBase + 22u * 4u] == 0x8fa710u &&
              runtime.words[kBase + 38u * 4u] == 0x8fa5b8u &&
              runtime.words[kBase + 39u * 4u] == 0x8fa64cu &&
              runtime.words[kBase + 57u * 4u] == 0x8fa8bcu &&
              runtime.words[kBase + 77u * 4u] == 0x8fa3d8u,
          "out-of-order table slots retain their native object addresses");
    for (std::uint32_t index = 1u; index < 78u; ++index)
        Check(runtime.words[kBase + index * 4u] != 0u,
              "every table slot one through 77 receives an object");

    std::size_t zero_kind = 0u;
    for (const auto& object : runtime.objects)
        if (object.kind == 0u) ++zero_kind;
    Check(zero_kind == 9u &&
              runtime.names[76] == "EAST_A" &&
              runtime.names[92] == "EAST_I" &&
              std::count(runtime.names.begin(), runtime.names.end(),
                         "ESI_SIPHON") == 16 &&
              runtime.guards[0x8fa8ccu] == 0xffu &&
              runtime.guards[0x8fa428u] == 0x1fu,
          "nine EAST modes, repeated siphon keys and guard groups are preserved");
}

void RepeatedInitialization() {
    Runtime runtime;
    Check(pal4::CombatEventRegistryInitializer::Initialize(runtime) == -17,
          "first initialization returns final CRT result");
    runtime.calls.clear();
    runtime.names.clear();
    Check(pal4::CombatEventRegistryInitializer::Initialize(runtime) == 222 &&
              runtime.objects.size() == 77u &&
              runtime.callbacks.size() == 77u &&
              runtime.names.size() == 154u &&
              runtime.calls.back() == "word 9415172 9413592",
          "second call refreshes table without rebuilding guarded objects");
}

}  // namespace

int main() {
    try {
        FirstInitialization();
        RepeatedInitialization();
        std::cout << "combat event registry initializer tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
