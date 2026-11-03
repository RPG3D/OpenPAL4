#include "pal4/global_system.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::GlobalSystemRuntimeStub {
    std::vector<std::string> events;
    std::vector<std::uint32_t> inserted;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::uint32_t next_entry{0x9000u};
    std::string fail_at;

    void Event(std::string event) {
        events.push_back(event);
        if (event == fail_at)
            throw std::runtime_error(event);
    }
    void RegisterCleanup() override { Event("register-cleanup"); }
    void SetErrorFlag(pal4::GlobalSystemText& text,
                      std::uint8_t flag) override {
        Check(flag == 0u && text.marker == 0x6bu &&
                  text.buffer.data == 0u && text.buffer.length == 0u &&
                  text.buffer.capacity == 0u,
              "temporary text preserves its incoming stack byte");
        Event("text-init");
    }
    std::uint32_t AllocateEntry(std::uint32_t bytes) override {
        Check(bytes == 0x18u, "entry size");
        const auto entry = next_entry;
        next_entry += bytes;
        Event("allocate:" + std::to_string(entry));
        return entry;
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        words[address] = value;
        Event("write:" + std::to_string(address));
    }
    void InitializeEntryText(std::uint32_t address,
                             std::uint8_t incoming_byte) override {
        Check(incoming_byte == 0x6bu &&
                  words[address - 8u] == pal4::GlobalSystem::kBaseVtable,
              "entry text follows base vtable");
        Event("entry-text:" + std::to_string(address));
    }
    void SetText(pal4::GlobalSystemText&, std::string_view value) override {
        Check(value == "InTeam", "first entry name");
        Event("set-in-team");
    }
    void AssignSourceText(pal4::GlobalSystemText&,
                          std::uint32_t address) override {
        Event("source:" + std::to_string(address));
    }
    void AppendEmptyErrorString(pal4::GlobalSystemText&) override {
        Event("append-empty");
    }
    void AssignEntryText(std::uint32_t address,
                         const pal4::GlobalSystemText&) override {
        Check(words[address - 8u] == pal4::GlobalSystem::kEntryVtable,
              "entry vtable is installed before text assignment");
        Event("assign-entry-text");
    }
    void PushBack(pal4::GlobalSystemCollection& collection,
                  std::uint32_t entry) override {
        Check(words[entry] == pal4::GlobalSystem::kEntryVtable,
              "pushed entry has its final vtable");
        inserted.push_back(entry);
        if (collection.entries.begin == 0u)
            collection.entries.begin = 0x8000u;
        collection.entries.end = collection.entries.begin +
                                 static_cast<std::uint32_t>(inserted.size()) * 4u;
        Event("push:" + std::to_string(words[entry + 4u]));
    }
    std::uint32_t GameDataCollection(std::uint32_t offset) override {
        Check(offset == 0x9f8u, "game data member offset");
        Event("get-game-data");
        return 0xa000u;
    }
    std::uint32_t GameDataObjectCollection(std::uint32_t offset) override {
        Check(offset == 0x654u, "game data object member offset");
        Event("get-data-object");
        return 0xb000u;
    }
    std::uint32_t ScriptEventCollection(std::uint32_t offset) override {
        Check(offset == 0x920u, "script event member offset");
        Event("get-script-events");
        return 0xc000u;
    }
    void ProcessAll(pal4::GlobalSystemCollection& collection,
                    std::uint32_t source) override {
        Check(collection.entries.end - collection.entries.begin == 32u,
              "all eight entries exist before system processing");
        Event("process:" + std::to_string(source));
    }
    void Sort(pal4::GlobalSystemCollection&) override { Event("sort"); }
    void ReleaseText(pal4::GlobalSystemText&) override {
        Event("release-text");
    }
    void DestroyUnlinkedEntry(std::uint32_t entry) override {
        Event("destroy:" + std::to_string(entry));
    }
    void CleanupCollection(pal4::GlobalSystemCollection& collection) override {
        Check(collection.entries.end - collection.entries.begin == 32u,
              "cleanup receives initialized collection");
        Event("cleanup-collection");
    }
};

std::size_t Position(const std::vector<std::string>& events,
                     const std::string& event) {
    const auto it = std::find(events.begin(), events.end(), event);
    Check(it != events.end(), "expected event exists");
    return static_cast<std::size_t>(it - events.begin());
}

void TestInitializeAndCleanup() {
    pal4::GlobalSystem system;
    Runtime runtime;
    auto& collection = system.Initialize(0x5au, 0x6bu, runtime);
    Check(&collection == &system.Collection(), "returns global collection");
    Check((collection.entries.marker & 0xffu) == 0x5au &&
              (collection.processed.marker & 0xffu) == 0x5au &&
              (collection.sorted.marker & 0xffu) == 0x6bu,
          "incoming stack bytes seed the three records");
    Check(runtime.inserted.size() == 8u, "eight entries created");
    Check(runtime.words[runtime.inserted[0] + 4u] == 2824u,
          "InTeam identifier");
    for (std::uint32_t i = 0u; i < 7u; ++i) {
        Check(runtime.words[runtime.inserted[i + 1u] + 4u] ==
                  2796u + i * 4u,
              "numbered identifiers advance by four");
        Check(Position(runtime.events,
                       "source:" + std::to_string(
                           pal4::GlobalSystem::kSourceTextArray + i * 16u)) <
                  Position(runtime.events,
                           "push:" + std::to_string(2796u + i * 4u)),
              "source text precedes insertion");
    }
    Check(Position(runtime.events, "allocate:36864") <
              Position(runtime.events, "set-in-team"),
          "first entry is allocated before text preparation");
    Check(Position(runtime.events, "push:2820") <
              Position(runtime.events, "get-game-data") &&
              Position(runtime.events, "process:40960") <
                  Position(runtime.events, "get-data-object") &&
              Position(runtime.events, "process:45056") <
                  Position(runtime.events, "get-script-events") &&
              Position(runtime.events, "process:49152") <
                  Position(runtime.events, "sort") &&
              Position(runtime.events, "sort") <
                  Position(runtime.events, "release-text"),
          "system processing, sort and temporary release order");

    const auto count = runtime.events.size();
    Check(&system.Initialize(1u, 2u, runtime) == &collection &&
              runtime.events.size() == count,
          "second initialization is guarded");
    system.Cleanup(runtime);
    Check(runtime.events.back() == "cleanup-collection",
          "cleanup entry delegates to collection cleanup");
}

void TestFailureReleasesUnlinkedEntryAndText() {
    pal4::GlobalSystem system;
    Runtime runtime;
    runtime.fail_at = "source:" + std::to_string(
        pal4::GlobalSystem::kSourceTextArray + 16u);
    bool threw = false;
    try {
        static_cast<void>(system.Initialize(0x5au, 0x6bu, runtime));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw, "source assignment failure propagates");
    Check(runtime.inserted.size() == 2u,
          "two completed entries remain in the guarded collection");
    Check(Position(runtime.events, "destroy:36912") <
              Position(runtime.events, "release-text"),
          "unlinked entry is destroyed before temporary text");
    const auto count = runtime.events.size();
    static_cast<void>(system.Initialize(1u, 2u, runtime));
    Check(runtime.events.size() == count,
          "entry guard remains set after failure");
}

}  // namespace

int main() {
    TestInitializeAndCleanup();
    TestFailureReleasesUnlinkedEntryAndText();
}
