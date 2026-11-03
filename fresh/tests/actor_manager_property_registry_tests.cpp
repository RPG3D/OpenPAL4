#include "pal4/actor_manager_property_registry.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IActorManagerPlayerPropertyRegistryRuntime {
    bool player = false;
    std::vector<std::string> trace;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::string> texts;
    std::map<std::uint32_t, std::uint32_t> slots;
    std::vector<std::uint32_t> entries;
    std::uint32_t fail_size = 0u;
    bool no_getter = false;
    bool no_setter = false;
    bool fail_name_assignment = false;

    std::uint32_t Address() const {
        return player ? pal4::ActorManagerPlayerPropertyRegistry::kAddress
                      : pal4::ActorManagerPropertyRegistry::kAddress;
    }
    void RegisterCleanup(const std::uint32_t callback) override {
        Check(callback == (player
                  ? pal4::ActorManagerPlayerPropertyRegistry::kCleanup
                  : pal4::ActorManagerPropertyRegistry::kCleanup),
              "matching atexit cleanup callback");
        trace.emplace_back("register");
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        trace.emplace_back("allocate:" + std::to_string(bytes));
        if (bytes == fail_size) return 0u;
        if (bytes == 24u) return 0x1000u;
        if (bytes == 36u) return 0x2000u;
        if (bytes == 16u) {
            const auto count = ++callback_allocations;
            return (count == 1u ? no_getter : no_setter)
                ? 0u : 0x3000u + 0x1000u * (count - 1u);
        }
        throw std::runtime_error("unexpected allocation");
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        trace.emplace_back("write:" + std::to_string(address));
    }
    void InitializeText(std::uint32_t address) override {
        trace.emplace_back("init-text:" + std::to_string(address));
    }
    void InitializeOwnedSlot(std::uint32_t address) override {
        trace.emplace_back("init-slot:" + std::to_string(address));
    }
    void SetText(pal4::ActorManagerRegistryText& text,
                 std::string_view value) override {
        text.value = value;
        trace.emplace_back("text:" + text.value);
    }
    void AppendEmptyText(pal4::ActorManagerRegistryText& text) override {
        Check(text.value == "DBKey", "empty suffix follows DBKey");
        trace.emplace_back("append-empty");
    }
    void AssignText(std::uint32_t address,
                    const pal4::ActorManagerRegistryText& text) override {
        if (address == 0x2014u && fail_name_assignment)
            throw std::runtime_error("name assignment failed");
        texts[address] = text.value;
        trace.emplace_back("assign-text:" + std::to_string(address));
    }
    void ReleaseText(pal4::ActorManagerRegistryText& text) override {
        trace.emplace_back("release-text:" + text.value);
        text.value.clear();
    }
    void AssignOwnedSlot(std::uint32_t address,
                         std::uint32_t object) override {
        slots[address] = object;
        trace.emplace_back("assign-slot:" + std::to_string(address));
    }
    void ReleaseOwnedSlot(std::uint32_t object) override {
        trace.emplace_back("release-slot:" + std::to_string(object));
    }
    void ConstructSetter(std::uint32_t object,
                         std::uint32_t callback) override {
        Check(callback == 0x530e00u, "setter callback address");
        words[object + 8u] = callback;
        trace.emplace_back("setter");
    }
    void PushBack(std::uint32_t collection,
                  std::uint32_t entry) override {
        Check(collection == Address(), "push collection address");
        entries.push_back(entry);
        trace.emplace_back("push:" + std::to_string(entry));
    }
    std::uint32_t GameData() override {
        trace.emplace_back("game-data");
        return 0x5000u;
    }
    std::uint32_t GameObject() override {
        trace.emplace_back("game-object");
        return 0x6000u;
    }
    std::uint32_t ScriptEventSystem() override {
        Check(player, "script event source belongs to player registry");
        trace.emplace_back("script-events");
        return 0x7000u;
    }
    void ProcessAll(std::uint32_t collection, std::uint32_t source,
                    std::uint32_t offset) override {
        Check(collection == Address(), "process collection address");
        trace.emplace_back("process:" + std::to_string(source) + ":" +
                           std::to_string(offset));
    }
    void Sort(std::uint32_t collection) override {
        Check(collection == Address(), "sort collection address");
        trace.emplace_back("sort");
    }
    void CleanupCollection(std::uint32_t collection) override {
        Check(collection == Address(), "cleanup collection address");
        trace.emplace_back("cleanup");
    }

private:
    std::uint32_t callback_allocations = 0u;
};

void FullInitialization() {
    pal4::ActorManagerPropertyRegistry registry;
    Runtime runtime;
    Check(registry.ForwardInitialize(runtime) == 0x8f610cu &&
              registry.CollectionConstructed() && registry.EntriesInitialized(),
          "thunk returns initialized collection");
    for (const auto offset : {0u, 4u, 8u, 12u, 16u, 20u, 24u, 28u,
                              32u, 36u, 40u, 44u})
        Check(runtime.words.at(0x8f610cu + offset) == 0u,
              "collection array starts empty");
    Check(runtime.entries == std::vector<std::uint32_t>{0x1000u, 0x2000u} &&
              runtime.words.at(0x1000u) == 0x843368u &&
              runtime.words.at(0x1004u) == 960u &&
              runtime.words.at(0x2000u) == 0x844f64u &&
              runtime.words.at(0x3000u) == 0x844f5cu &&
              runtime.words.at(0x3008u) == 0x530f40u &&
              runtime.words.at(0x300cu) == 0u &&
              runtime.words.at(0x4008u) == 0x530e00u &&
              runtime.texts.at(0x1008u) == "DBKey" &&
              runtime.texts.at(0x2014u) == "UName" &&
              runtime.slots.at(0x2004u) == 0x3000u &&
              runtime.slots.at(0x200cu) == 0x4000u,
          "registry entries retain IDA layout and callbacks");
    const std::vector<std::string> ordered{
        "text:DBKey", "append-empty", "assign-text:4104",
        "write:4100", "push:4096", "allocate:36", "write:8192",
        "init-slot:8196", "init-text:8212", "write:8192",
        "text:UName", "assign-text:8212", "release-text:UName",
        "allocate:16", "write:12288", "write:12296", "write:12300",
        "assign-slot:8196", "release-slot:12288", "allocate:16",
        "setter", "assign-slot:8204", "release-slot:16384",
        "push:8192", "game-data", "process:20480:896",
        "game-object", "process:24576:0", "sort", "release-text:DBKey"};
    const auto begin = runtime.trace.begin() + 17;
    Check(std::vector<std::string>(begin, runtime.trace.end()) == ordered,
          "entry setup and postprocessing order");
    const auto count = runtime.trace.size();
    Check(registry.Initialize(runtime) == 0x8f610cu &&
              registry.ForwardInitialize(runtime) == 0x8f610cu &&
              runtime.trace.size() == count,
          "repeated calls have no side effects");
}

void MissingCallbacks() {
    pal4::ActorManagerPropertyRegistry registry;
    Runtime runtime;
    runtime.no_getter = true;
    runtime.no_setter = true;
    Check(registry.Initialize(runtime) == 0x8f610cu &&
              runtime.slots.at(0x2004u) == 0u &&
              runtime.slots.at(0x200cu) == 0u,
          "null callback allocations create empty owned slots");
}

void EntryAllocationFailure() {
    for (const auto size : {24u, 36u}) {
        pal4::ActorManagerPropertyRegistry registry;
        Runtime runtime;
        runtime.fail_size = size;
        bool failed = false;
        try {
            (void)registry.Initialize(runtime);
        } catch (const std::bad_alloc&) {
            failed = true;
        }
        Check(failed && registry.CollectionConstructed() &&
                  registry.EntriesInitialized(),
              "entry allocation failure preserves original guard timing");
        Check(runtime.trace.back() == "release-text:" +
                  std::string(size == 24u ? "" : "DBKey"),
              "allocation failure releases the live temporary text");
    }
}

void TextAssignmentFailure() {
    pal4::ActorManagerPropertyRegistry registry;
    Runtime runtime;
    runtime.fail_name_assignment = true;
    bool failed = false;
    try {
        (void)registry.Initialize(runtime);
    } catch (const std::runtime_error&) {
        failed = true;
    }
    Check(failed && runtime.trace.size() >= 2u &&
              runtime.trace[runtime.trace.size() - 2u] ==
                  "release-text:UName" &&
              runtime.trace.back() == "release-text:DBKey",
          "failed UName assignment releases both temporary strings");
}

void ExitCleanup() {
    Runtime runtime;
    pal4::ActorManagerPropertyRegistry::Cleanup(runtime);
    Check(runtime.trace == std::vector<std::string>{"cleanup"},
          "CRT exit callback cleans the global collection");
}

void PlayerRegistry() {
    pal4::ActorManagerPlayerPropertyRegistry registry;
    Runtime runtime;
    runtime.player = true;
    const auto address = pal4::ActorManagerPlayerPropertyRegistry::kAddress;
    Check(registry.ForwardInitialize(runtime) == address &&
              registry.CollectionConstructed() &&
              registry.EntriesInitialized() &&
              runtime.entries ==
                  std::vector<std::uint32_t>{0x1000u, 0x2000u},
          "player registry thunk initializes its own collection");
    for (std::uint32_t offset = 0u; offset < 48u; offset += 4u)
        Check(runtime.words.at(address + offset) == 0u,
              "player registry starts with three empty arrays");
    Check(runtime.words.at(0x1000u) == 0x843368u &&
              runtime.words.at(0x1004u) == 1196u &&
              runtime.words.at(0x2000u) == 0x845268u &&
              runtime.words.at(0x3000u) == 0x845260u &&
              runtime.words.at(0x3008u) == 0x530f40u &&
              runtime.words.at(0x4008u) == 0x530e00u &&
              runtime.texts.at(0x1008u) == "DBKey" &&
              runtime.texts.at(0x2014u) == "UName" &&
              runtime.slots.at(0x2004u) == 0x3000u &&
              runtime.slots.at(0x200cu) == 0x4000u,
          "player registry descriptors and callbacks match IDA layout");
    const std::vector<std::string> tail{
        "game-data", "process:20480:1112",
        "game-object", "process:24576:0",
        "script-events", "process:28672:896",
        "sort", "release-text:DBKey"};
    Check(runtime.trace.size() >= tail.size() &&
              std::vector<std::string>(runtime.trace.end() - tail.size(),
                                       runtime.trace.end()) == tail,
          "player registry processes game, object and script data in order");
    const auto count = runtime.trace.size();
    Check(registry.Initialize(runtime) == address &&
              registry.ForwardInitialize(runtime) == address &&
              runtime.trace.size() == count,
          "player registry repeats without side effects");
    runtime.trace.clear();
    pal4::ActorManagerPlayerPropertyRegistry::Cleanup(runtime);
    Check(runtime.trace == std::vector<std::string>{"cleanup"},
          "player registry exit callback cleans its collection");

    pal4::ActorManagerPlayerPropertyRegistry absent;
    Runtime no_callbacks;
    no_callbacks.player = true;
    no_callbacks.no_getter = true;
    no_callbacks.no_setter = true;
    Check(absent.Initialize(no_callbacks) == address &&
              no_callbacks.slots.at(0x2004u) == 0u &&
              no_callbacks.slots.at(0x200cu) == 0u,
          "player registry keeps empty slots on callback allocation failure");
}

}  // namespace

int main() {
    try {
        FullInitialization();
        MissingCallbacks();
        EntryAllocationFailure();
        TextAssignmentFailure();
        ExitCleanup();
        PlayerRegistry();
        std::cout << "actor_manager_property_registry_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
