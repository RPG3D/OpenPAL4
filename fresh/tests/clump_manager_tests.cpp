#include "pal4/clump_manager.h"

#include <algorithm>
#include <cstdlib>
#include <cctype>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IClumpManagerRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::uint32_t allocated = object;
    bool fail_name = false;
    bool populate_on_container_init = false;
    bool populate_on_container_destroy = false;
    std::uint32_t next_record_allocation = 0xa000u;
    std::vector<std::uint32_t> load_elements;
    std::map<std::uint32_t, std::string> strings;
    bool fail_copy = false;
    int copies_until_failure = -1;

    [[nodiscard]] std::uint8_t ReadByte(
        const std::uint32_t address) override { return bytes[address]; }
    [[nodiscard]] std::uint32_t ReadWord(
        const std::uint32_t address) override { return words[address]; }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        bytes[address] = value;
        events.emplace_back("byte:" + std::to_string(address));
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
        events.emplace_back("word:" + std::to_string(address));
    }
    [[nodiscard]] std::uint32_t SingletonManager() override {
        events.emplace_back("get-singleton");
        return 0x7000u;
    }
    [[nodiscard]] std::uint32_t RegisterSingleton(
        const std::uint32_t manager, const std::string_view name,
        const std::uint32_t id,
        const pal4::ClumpManagerCallback factory,
        const pal4::ClumpManagerCallback destroy) override {
        Check(manager == 0x7000u && name == "RpHookClumpMgrArena" &&
                  id == 1010u &&
                  factory == pal4::ClumpManagerCallback::create &&
                  destroy == pal4::ClumpManagerCallback::destroy,
              "registers exact singleton identity and lifecycle callbacks");
        events.emplace_back("register");
        return 0x7777u;
    }
    [[nodiscard]] std::uint32_t Allocate(const std::uint32_t size) override {
        if (size == 48u && next_record_allocation == 0xa000u &&
            load_elements.empty()) {
            events.emplace_back("allocate");
            return allocated;
        }
        events.emplace_back("allocate:" + std::to_string(size));
        const auto result = next_record_allocation;
        next_record_allocation += 0x100u;
        return result;
    }
    [[nodiscard]] std::uint32_t Free(const std::uint32_t address) override {
        events.emplace_back("free:" + std::to_string(address));
        return address == object ? 0x66u : 0x55u;
    }
    void InitializeContainer(const std::uint32_t container) override {
        Check(container == object + 16u, "container subobject is at +16");
        events.emplace_back("init-container");
        if (populate_on_container_init) {
            words[object + 4u] = 0x5000u;
            words[object + 8u] = 0x5018u;
        }
    }
    void DestroyContainer(const std::uint32_t container) override {
        Check(container == object + 16u, "destroys container subobject");
        events.emplace_back("destroy-container");
        if (populate_on_container_destroy) {
            words[object + 4u] = 0x6000u;
            words[object + 8u] = 0x600cu;
        }
    }
    void CleanupString(const std::uint32_t record) override {
        events.emplace_back("cleanup:" + std::to_string(record));
    }
    void SetContainerName(const std::uint32_t container,
                          const std::string_view name) override {
        Check(container == object + 16u &&
                  name == "hook-clump-mgr-container",
              "sets native container name");
        events.emplace_back("name");
        if (fail_name) throw std::runtime_error("test name failure");
    }
    void SetContainerFlag(const std::uint32_t container,
                          const std::uint32_t value) override {
        Check(container == object + 16u && value == 1u,
              "sets native container property one");
        events.emplace_back("flag");
    }
    void SetContainerCallback(
        const std::uint32_t container,
        const pal4::ClumpManagerCallback callback) override {
        Check(container == object + 16u,
              "all callbacks belong to embedded container");
        events.emplace_back("callback:" +
                            std::to_string(static_cast<int>(callback)));
    }
    [[nodiscard]] std::uint32_t CreateLoadCollection() override {
        events.emplace_back("create-load");
        return 0x8000u;
    }
    void ScanClumpFile(const std::uint32_t collection,
                       const std::string_view filename) override {
        Check(collection == 0x8000u && filename == "clumps.txt",
              "load scanner receives collection and filename");
        events.emplace_back("scan");
    }
    [[nodiscard]] int LoadCollectionCount(
        const std::uint32_t collection) override {
        Check(collection == 0x8000u, "count reads temporary collection");
        events.emplace_back("count");
        return static_cast<int>(load_elements.size());
    }
    [[nodiscard]] std::uint32_t LoadCollectionElement(
        const std::uint32_t collection, const int index) override {
        Check(collection == 0x8000u && index >= 0 &&
                  static_cast<std::size_t>(index) < load_elements.size(),
              "loads element by collection index");
        events.emplace_back("element:" + std::to_string(index));
        return load_elements[static_cast<std::size_t>(index)];
    }
    [[nodiscard]] std::uint32_t LowercaseString(
        const std::uint32_t record) override {
        for (char& ch : strings[record])
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        events.emplace_back("lower:" + std::to_string(record));
        return record;
    }
    void CopyStringRecord(const std::uint32_t destination,
                          const std::uint32_t source) override {
        if (fail_copy || copies_until_failure == 0)
            throw std::runtime_error("copy failed");
        if (copies_until_failure > 0) --copies_until_failure;
        strings[destination] = strings[source];
        events.emplace_back("copy:" + std::to_string(destination));
    }
    void ClearLoadCollection(const std::uint32_t collection) override {
        Check(collection == 0x8000u, "clears temporary load collection");
        events.emplace_back("clear-load");
    }
    void DestroyLoadCollection(const std::uint32_t collection) override {
        Check(collection == 0x8000u, "destroys temporary load collection");
        events.emplace_back("destroy-load");
    }
    [[nodiscard]] std::uint32_t CreateLookupString(
        const std::string_view text) override {
        strings[0x9000u] = text;
        events.emplace_back("create-lookup");
        return 0x9000u;
    }
    [[nodiscard]] int FindStringSubstring(
        const std::uint32_t query, const std::uint32_t candidate,
        const std::uint32_t start_offset) override {
        Check(query == 0x9000u && start_offset == 0u,
              "lookup searches normalized query from its start");
        events.emplace_back("find:" + std::to_string(candidate));
        const auto position = strings[query].find(strings[candidate]);
        return position == std::string::npos ? -1 :
               static_cast<int>(position);
    }
    void CleanupLookupString(const std::uint32_t record) override {
        Check(record == 0x9000u, "cleans temporary lookup string");
        events.emplace_back("cleanup-lookup");
    }
    [[nodiscard]] std::uint32_t CleanupEmbeddedContainer(
        const std::uint32_t container) override {
        Check(container == object + 16u, "cleans manager embedded container");
        events.emplace_back("cleanup-container");
        return 0x77u;
    }
};

}  // namespace

int main() {
    using Manager = pal4::ClumpManager;
    Runtime runtime;
    runtime.bytes[0x92f570u] = 0x80u;
    Check(Manager::Register(runtime) == 0x7777u &&
              runtime.bytes[0x92f570u] == 0x81u &&
              runtime.words[0x92f56cu] == 0x7777u &&
              runtime.events == std::vector<std::string>({
                  "byte:" + std::to_string(0x92f570u),
                  "get-singleton", "register",
                  "word:" + std::to_string(0x92f56cu)}),
          "first registration sets guard before singleton registration");
    runtime.events.clear();
    Check(Manager::Register(runtime) == 0x7777u && runtime.events.empty(),
          "guarded registration returns cached instance");
    Check(Manager::Compare(1u, 2u) == 1 &&
              Manager::Compare(2u, 2u) == 0 &&
              Manager::Compare(3u, 2u) == -1 &&
              Manager::Compare(0xffffffffu, 0u) == -1,
          "callback compares unsigned keys with signed ordering result");
    runtime.events.clear();
    runtime.words[0x3000u] = 0x1234u;
    Check(Manager::FreeRecord(0x3000u, runtime) == 0x55u &&
              runtime.words[0x3000u] == 0u &&
              runtime.events == std::vector<std::string>({
                  "word:" + std::to_string(0x3000u),
                  "free:" + std::to_string(0x3000u)}),
          "record callback zeros first word before freeing");
    runtime.events.clear();
    Check(Manager::SetCurrentClump(0x3000u, 0x9999u, runtime) ==
              0x9999u && runtime.words[0x3000u] == 0x9999u,
          "current-clump callback stores and returns supplied value");
    runtime.events.clear();
    runtime.allocated = 0u;
    Check(Manager::Create(runtime) == 0u &&
              runtime.events == std::vector<std::string>({"allocate"}),
          "allocation failure skips construction");
    runtime.events.clear();
    runtime.allocated = Runtime::object;
    runtime.bytes[Runtime::object] = 0x5au;
    Check(Manager::Create(runtime) == Runtime::object &&
              runtime.bytes[Runtime::object] == 0x5au &&
              runtime.words[Runtime::object + 4u] == 0u &&
              runtime.words[Runtime::object + 8u] == 0u &&
              runtime.words[Runtime::object + 12u] == 0u &&
              runtime.events.front() == "allocate" &&
              runtime.events[4] == "init-container" &&
              runtime.events[6] == "name" &&
              runtime.events[7] == "flag" &&
              runtime.events.back() == "callback:5",
          "constructor initializes fields and container callbacks in order");
    runtime.events.clear();
    runtime.populate_on_container_init = true;
    Check(Manager::Initialize(Runtime::object, runtime) == Runtime::object &&
              runtime.words[Runtime::object + 8u] == 0x5000u &&
              runtime.events[4] == "cleanup:20480" &&
              runtime.events[5] == "cleanup:20492",
          "constructor releases populated string records in twelve-byte steps");
    runtime.populate_on_container_init = false;
    runtime.events.clear();
    runtime.words[Runtime::object + 4u] = 0x5000u;
    runtime.words[Runtime::object + 8u] = 0x5018u;
    runtime.words[Runtime::object + 12u] = 0x5040u;
    Check(Manager::Cleanup(Runtime::object, runtime) == 0x55u &&
              runtime.words[Runtime::object + 4u] == 0u &&
              runtime.words[Runtime::object + 8u] == 0u &&
              runtime.words[Runtime::object + 12u] == 0u &&
              runtime.events[0] == "cleanup:20480" &&
              runtime.events[1] == "cleanup:20492" &&
              runtime.events[3] == "destroy-container" &&
              runtime.events[4] == "free:20480",
          "cleanup releases string array, destroys container and clears fields");
    runtime.events.clear();
    runtime.populate_on_container_destroy = true;
    Check(Manager::Cleanup(Runtime::object, runtime) == 0x55u &&
              runtime.events[1] == "destroy-container" &&
              runtime.events[2] == "cleanup:24576" &&
              runtime.events[3] == "free:24576",
          "cleanup observes records exposed by container destructor");
    runtime.populate_on_container_destroy = false;
    runtime.events.clear();
    Check(Manager::Destroy(0u, runtime) == 0u && runtime.events.empty(),
          "null destructor is a defined no-op");
    Check(Manager::Destroy(Runtime::object, runtime) == 0x66u &&
              runtime.events[2] == "free:0" &&
              runtime.events.back() == "free:4096",
          "destructor cleans embedded state before freeing manager");
    runtime.events.clear();
    runtime.fail_name = true;
    bool threw = false;
    try {
        (void)Manager::Create(runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.events.back() == "free:4096" &&
              runtime.events[runtime.events.size() - 6u] ==
                  "destroy-container" &&
              runtime.events[runtime.events.size() - 5u] == "free:0",
          "constructor failure destroys container before freeing manager");
    Runtime load;
    Check(!Manager::LoadClump(Runtime::object, "clumps.txt", load) &&
              load.events == std::vector<std::string>({
                  "create-load", "scan", "count", "destroy-load"}),
          "empty scan destroys collection without clearing or allocating");
    load.events.clear();
    load.load_elements = {0x8000u, 0x8100u, 0x8200u, 0x8300u};
    load.strings[0x800cu] = "FIRST";
    load.strings[0x810cu] = "Second";
    load.strings[0x820cu] = "THIRD";
    load.strings[0x830cu] = "Fourth";
    Check(Manager::LoadClump(Runtime::object, "clumps.txt", load) &&
              load.words[Runtime::object + 4u] == 0xa200u &&
              load.words[Runtime::object + 8u] == 0xa230u &&
              load.words[Runtime::object + 12u] == 0xa230u &&
              load.strings[0xa200u] == "first" &&
              load.strings[0xa20cu] == "second" &&
              load.strings[0xa218u] == "third" &&
              load.strings[0xa224u] == "fourth" &&
              load.events.back() == "destroy-load" &&
              load.events[load.events.size() - 2u] == "clear-load",
          "load lowercases and appends records across growth and spare capacity");
    Check(load.next_record_allocation == 0xa300u,
          "fourth record reuses remaining capacity");
    load.events.clear();
    load.fail_copy = true;
    const auto old_begin = load.words[Runtime::object + 4u];
    const auto old_end = load.words[Runtime::object + 8u];
    threw = false;
    try {
        (void)Manager::LoadClump(Runtime::object, "clumps.txt", load);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && load.events.back() == "destroy-load" &&
              load.events[load.events.size() - 2u] == "free:41728" &&
              load.words[Runtime::object + 4u] == old_begin &&
              load.words[Runtime::object + 8u] == old_end &&
              std::find(load.events.begin(), load.events.end(), "clear-load") ==
                  load.events.end(),
          "failed reallocation frees new storage and destroys collection");
    load.events.clear();
    load.fail_copy = false;
    load.copies_until_failure = 1;
    threw = false;
    try {
        (void)Manager::LoadClump(Runtime::object, "clumps.txt", load);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && load.events.back() == "destroy-load" &&
              load.events[load.events.size() - 2u] == "free:41984" &&
              load.events[load.events.size() - 3u] == "cleanup:41984" &&
              load.words[Runtime::object + 4u] == old_begin &&
              load.words[Runtime::object + 8u] == old_end,
          "partial reallocation cleans constructed records and preserves array");
    load.words[0xa200u] = 0xb000u;
    load.words[0xa20cu] = 0xb100u;
    load.words[0xa218u] = 0xb200u;
    load.words[0xa224u] = 0xb300u;
    load.strings[0xb000u] = "unrelated";
    load.strings[0xb100u] = "second";
    load.strings[0xb200u] = "third";
    load.strings[0xb300u] = "fourth";
    load.events.clear();
    Check(Manager::FindClump(Runtime::object, "SECOND-extra", load) ==
              0xb100u &&
              load.events == std::vector<std::string>({
                  "create-lookup", "lower:36864", "find:45056",
                  "find:45312", "cleanup-lookup"}),
          "lookup returns first candidate matching query prefix");
    load.events.clear();
    Check(Manager::FindClump(Runtime::object, "missing", load) == 0u &&
              load.events.back() == "cleanup-lookup" &&
              std::count_if(load.events.begin(), load.events.end(),
                            [](const std::string& event) {
                                return event.starts_with("find:");
                            }) == 4,
          "lookup visits all records and cleans temporary on miss");
    load.events.clear();
    Check(Manager::CleanupContainer(Runtime::object, load) == 0x77u &&
              load.events == std::vector<std::string>({"cleanup-container"}),
          "container cleanup forwards embedded offset and result");
    pal4::ClumpManagerRuntimeStub stub;
    Check(Manager::Register(stub) == 0u && Manager::Create(stub) == 0u,
          "singleton and allocator boundary stub links");
    return EXIT_SUCCESS;
}
