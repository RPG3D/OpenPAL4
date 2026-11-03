#include "pal4/game_db_effect_object.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool okay, const char* message) {
    if (!okay) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IGameDbEffectObjectRuntime {
    std::vector<std::string> events;
    bool allocate_success = true;
    std::uint32_t free_result = 0x1234u;

    bool AllocateText(pal4::GameDbStringObjectState& text,
                      std::uint32_t length) override {
        events.push_back("allocate-text:" + std::to_string(length));
        if (!allocate_success)
            return false;
        text.data = 0xa000u;
        text.capacity = length;
        return true;
    }
    void WriteText(std::uint32_t destination, const char* source,
                   std::uint32_t length) override {
        events.push_back("write-text:" + std::to_string(destination) +
                         ":" + std::string(source, length));
    }
    void ClearEffectData(pal4::GameDbEffectObjectState&) override {
        events.push_back("clear-effect-data");
    }
    void ReleaseText(std::uint32_t data) override {
        events.push_back("release-text:" + std::to_string(data));
    }
    void CleanupTrack(std::uint32_t address) override {
        events.push_back("cleanup-track:" + std::to_string(address));
    }
    void CleanupRecord(std::uint32_t address) override {
        events.push_back("cleanup-record:" + std::to_string(address));
    }
    std::uint32_t Free(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
        return free_result;
    }
};

struct AddRuntime final : pal4::IGameDbEffectAddRuntime {
    std::vector<std::string> events;
    std::map<std::uint32_t, std::int32_t> keys;
    std::map<std::uint32_t, pal4::GameDbEffectIndexPair> pairs;
    std::uint32_t next_index = 0x8000u;

    std::int32_t GetRowKey(std::uint32_t row,
                           std::uint32_t property_index) override {
        events.push_back("key:" + std::to_string(row) + ":" +
                         std::to_string(property_index));
        return keys.at(row);
    }
    void AppendTrack(pal4::GameDbValueArrayState& tracks,
                     std::uint32_t row) override {
        events.push_back("append:" + std::to_string(row));
        if (tracks.begin == 0u)
            tracks.begin = tracks.end = 0x7000u;
        tracks.end += 16u;
    }
    void AssignTrack(std::uint32_t address, std::uint32_t row) override {
        events.push_back("assign:" + std::to_string(address) + ":" +
                         std::to_string(row));
    }
    pal4::GameDbEffectIndexPair ReadPair(std::uint32_t address) override {
        return pairs.at(address);
    }
    void WritePair(std::uint32_t address,
                   pal4::GameDbEffectIndexPair pair) override {
        pairs[address] = pair;
    }
    std::uint32_t AllocateIndex(std::uint32_t bytes) override {
        events.push_back("allocate:" + std::to_string(bytes));
        return next_index;
    }
    void FreeIndex(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
    }
};

struct AssignRuntime final : pal4::IGameDbEffectAssignRuntime {
    std::vector<std::string> events;
    std::uint32_t next_memory = 0x3000u;
    void ClearEffectData(pal4::GameDbEffectObjectState&) override {
        events.push_back("clear");
    }
    void AssignRecord(std::uint32_t destination,
                      std::uint32_t source) override {
        events.push_back("assign:" + std::to_string(destination) + ":" +
                         std::to_string(source));
    }
    void ConstructRecord(std::uint32_t destination,
                         std::uint32_t source) override {
        events.push_back("construct:" + std::to_string(destination) + ":" +
                         std::to_string(source));
    }
    void DestroyRecord(std::uint32_t address) override {
        events.push_back("destroy:" + std::to_string(address));
    }
    std::uint32_t AllocateRecords(std::uint32_t bytes) override {
        events.push_back("allocate:" + std::to_string(bytes));
        return next_memory;
    }
    void FreeRecords(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
    }
    void Finalize(pal4::GameDbEffectObjectState& object,
                  std::uint32_t value) override {
        events.push_back("finalize:" + std::to_string(value) + ":" +
                         std::to_string(object.key_index));
    }
};

struct ClearRuntime final : pal4::IGameDbEffectClearRuntime {
    std::vector<std::string> events;
    void ClearTrack(std::uint32_t address) override {
        events.push_back("track:" + std::to_string(address));
    }
    std::uint32_t ClearRecord(std::uint32_t address) override {
        events.push_back("record:" + std::to_string(address));
        return address + 1u;
    }
};

struct TrackRuntime final : pal4::IGameDbEffectTrackRuntime {
    std::vector<std::string> events;
    std::uint32_t next_memory = 0x8100u;
    void ConstructTrack(std::uint32_t destination,
                        std::uint32_t source) override {
        events.push_back("construct:" + std::to_string(destination) + ":" +
                         std::to_string(source));
    }
    void FillTrack(std::uint32_t destination,
                   std::uint32_t event) override {
        events.push_back("fill:" + std::to_string(destination) + ":" +
                         std::to_string(event));
    }
    void AssignEvents(std::uint32_t destination,
                      std::uint32_t event) override {
        events.push_back("events:" + std::to_string(destination) + ":" +
                         std::to_string(event));
    }
    void CleanupTrack(std::uint32_t address) override {
        events.push_back("cleanup:" + std::to_string(address));
    }
    std::uint32_t AllocateTracks(std::uint32_t bytes) override {
        events.push_back("allocate:" + std::to_string(bytes));
        return next_memory;
    }
    void FreeTracks(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
    }
};

struct EnableRuntime final : pal4::IGameDbEffectEnableRuntime {
    std::uint32_t observed_flags = 0u;
    int calls = 0;
    std::int32_t result = -19;
    std::int32_t CallVirtual24(
        pal4::GameDbEffectObjectState& object) override {
        observed_flags = object.lock_flags;
        ++calls;
        return result;
    }
};

struct SortRuntime final : pal4::IGameDbEffectSortRuntime {
    std::map<std::uint32_t, std::int32_t> keys;
    std::map<std::uint32_t, pal4::GameDbEffectIndexPair> pairs;
    std::vector<std::string> events;
    std::uint32_t next_memory = 0x9000u;

    std::int32_t GetTrackKey(std::uint32_t track,
                             std::uint32_t property_index) override {
        Check(property_index == 7u, "sort uses effect key index");
        return keys.at(track);
    }
    void AssignTrack(std::uint32_t destination,
                     std::uint32_t source) override {
        events.push_back("assign:" + std::to_string(destination) + ":" +
                         std::to_string(source));
        keys[destination] = keys.at(source);
    }
    void CopyTrackProperties(std::uint32_t destination,
                             std::uint32_t source) override {
        events.push_back("copy:" + std::to_string(destination) + ":" +
                         std::to_string(source));
        keys[destination] = keys.at(source);
    }
    void CleanupTrack(std::uint32_t address) override {
        events.push_back("cleanup:" + std::to_string(address));
        keys.erase(address);
    }
    pal4::GameDbEffectIndexPair ReadPair(std::uint32_t address) override {
        return pairs.at(address);
    }
    void WritePair(std::uint32_t address,
                   pal4::GameDbEffectIndexPair pair) override {
        pairs[address] = pair;
    }
    std::uint32_t AllocateIndex(std::uint32_t bytes) override {
        events.push_back("allocate:" + std::to_string(bytes));
        const auto address = next_memory;
        next_memory += bytes + 0x100u;
        return address;
    }
    void FreeIndex(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
    }
};

void ProcessAndSortEvents() {
    SortRuntime runtime;
    pal4::GameDbEffectObjectState object{};
    object.key_index = 7u;
    object.index = {0u, 0x8000u, 0x8010u, 0x8020u};
    object.tracks = {0u, 0x7000u, 0x7000u, 0x7100u};
    pal4::GameDbEffectObject::ProcessAndSortEvents(object, runtime);
    Check(object.index.end == 0x8000u && runtime.events.empty(),
          "empty tracks clear the index without touching capacity");

    object.tracks.end = 0x7030u;
    runtime.keys = {{0x7000u, 3}, {0x7010u, -2}, {0x7020u, 8}};
    pal4::GameDbEffectObject::ProcessAndSortEvents(object, runtime);
    Check(object.tracks.end == 0x7030u && object.index.end == 0x8018u &&
              runtime.events.empty() &&
              runtime.pairs.at(0x8000u).key == -2 &&
              runtime.pairs.at(0x8000u).track_index == 1u &&
              runtime.pairs.at(0x8008u).key == 3 &&
              runtime.pairs.at(0x8010u).key == 8,
          "distinct tracks retain order while index sorts signed keys");

    runtime.events.clear();
    runtime.keys = {{0x7000u, 4}, {0x7010u, 4},
                    {0x7020u, -3}, {0x7030u, -3},
                    {0x7040u, 4}};
    object.tracks.end = 0x7050u;
    pal4::GameDbEffectObject::ProcessAndSortEvents(object, runtime);
    Check(object.tracks.end == 0x7030u &&
              runtime.events == std::vector<std::string>{
                  "assign:28672:28672", "copy:28688:28704",
                  "copy:28704:28736", "cleanup:28720",
                  "cleanup:28736"} &&
              runtime.pairs.at(0x8000u).key == -3 &&
              runtime.pairs.at(0x8000u).track_index == 1u &&
              runtime.pairs.at(0x8008u).key == 4 &&
              runtime.pairs.at(0x8008u).track_index == 0u &&
              runtime.pairs.at(0x8010u).key == 4 &&
              runtime.pairs.at(0x8010u).track_index == 2u,
          "only consecutive equal keys collapse and later equal keys survive");

    runtime.events.clear();
    runtime.keys.clear();
    object.tracks.end = object.tracks.begin + 18u * 16u;
    object.index.end = object.index.begin;
    object.index.capacity = object.index.begin + 16u * 8u;
    for (auto i = 0u; i < 18u; ++i)
        runtime.keys[object.tracks.begin + i * 16u] =
            static_cast<std::int32_t>(18u - i);
    pal4::GameDbEffectObject::ProcessAndSortEvents(object, runtime);
    Check(object.index.begin == 0x9000u &&
              object.index.end == 0x9090u &&
              object.index.capacity == 0x9100u &&
              runtime.events == std::vector<std::string>{
                  "allocate:256", "free:32768"} &&
              runtime.pairs.at(0x9000u).key == 1 &&
              runtime.pairs.at(0x9000u).track_index == 17u &&
              runtime.pairs.at(0x9088u).key == 18,
          "large index grows geometrically and sorts all 18 pairs");

    runtime.events.clear();
    runtime.keys = {{0x7000u, -11}};
    runtime.next_memory = 0xd000u;
    object.tracks.end = 0x7010u;
    object.index = {};
    pal4::GameDbEffectObject::ProcessAndSortEvents(object, runtime);
    Check(object.index.begin == 0xd000u &&
              object.index.end == 0xd008u &&
              object.index.capacity == 0xd008u &&
              runtime.pairs.at(0xd000u).key == -11 &&
              runtime.pairs.at(0xd000u).track_index == 0u &&
              runtime.events == std::vector<std::string>{
                  "allocate:8", "free:0"},
          "first track allocates the initial eight-byte index slot");
}

struct ResizeRuntime final : pal4::IGameDbEffectResizeRuntime {
    std::map<std::uint32_t, pal4::GameDbEffectIndexPair> pairs;
    std::vector<std::string> events;
    std::uint32_t next_memory = 0x9000u;

    pal4::GameDbEffectIndexPair ReadPair(std::uint32_t address) override {
        return pairs.at(address);
    }
    void WritePair(std::uint32_t address,
                   pal4::GameDbEffectIndexPair pair) override {
        pairs[address] = pair;
        events.push_back("pair:" + std::to_string(address));
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        const auto address = next_memory;
        next_memory += 0x1000u;
        events.push_back("allocate:" + std::to_string(bytes));
        return address;
    }
    void Free(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
    }
    void CopyConstructTrack(std::uint32_t destination,
                            std::uint32_t source) override {
        events.push_back("construct:" + std::to_string(destination) + ":" +
                         std::to_string(source));
    }
    void CleanupTrack(std::uint32_t address) override {
        events.push_back("cleanup:" + std::to_string(address));
    }
};

void ResizeArrayBranches() {
    ResizeRuntime runtime;
    pal4::GameDbEffectObjectState object{};
    object.index = {0u, 0x1000u, 0x1010u, 0x1010u};
    object.tracks = {0u, 0x2000u, 0x2010u, 0x2010u};
    runtime.pairs[0x1000u] = {-3, 0u};
    runtime.pairs[0x1008u] = {9, 1u};
    Check(pal4::GameDbEffectObject::ResizeArrays(object, 4u, runtime) ==
                  0xa010u &&
              object.index.begin == 0x9000u &&
              object.index.end == 0x9010u &&
              object.index.capacity == 0x9020u &&
              runtime.pairs.at(0x9000u).key == -3 &&
              runtime.pairs.at(0x9008u).track_index == 1u &&
              object.tracks.begin == 0xa000u &&
              object.tracks.end == 0xa010u &&
              object.tracks.capacity == 0xa040u &&
              runtime.events == std::vector<std::string>{
                  "allocate:32", "pair:36864", "pair:36872",
                  "free:4096", "allocate:64",
                  "construct:40960:8192", "cleanup:8192", "free:8192"},
          "both arrays grow with raw pair copy and owned track reconstruction");
    runtime.events.clear();
    Check(pal4::GameDbEffectObject::ResizeArrays(object, 2u, runtime) == 4u &&
              runtime.events.empty(),
          "no growth returns existing track capacity");

    object.index = {0u, 0x1000u, 0x1008u, 0x1008u};
    object.tracks = {0u, 0x2000u, 0x2010u, 0x2040u};
    runtime.next_memory = 0xb000u;
    Check(pal4::GameDbEffectObject::ResizeArrays(object, 3u, runtime) == 4u &&
              object.index.begin == 0xb000u &&
              object.index.end == 0xb008u &&
              object.tracks.begin == 0x2000u &&
              runtime.events == std::vector<std::string>{
                  "allocate:24", "pair:45056", "free:4096"},
          "index-only growth retains track buffer and returns track capacity");

    runtime.events.clear();
    object.index = {0u, 0x3000u, 0x3008u, 0x3040u};
    object.tracks = {0u, 0x4000u, 0x4020u, 0x4020u};
    runtime.next_memory = 0xc000u;
    Check(pal4::GameDbEffectObject::ResizeArrays(object, 3u, runtime) ==
                  0xc020u &&
              object.tracks.end == 0xc020u &&
              runtime.events == std::vector<std::string>{
                  "allocate:48", "construct:49152:16384",
                  "construct:49168:16400", "cleanup:16384",
                  "cleanup:16400", "free:16384"},
          "track-only growth constructs all tracks before old cleanup");
}

void ConstructAndDestroyPopulated() {
    Runtime runtime;
    constexpr char name[] = "magic";
    const auto low_byte = static_cast<std::uint8_t>(
        reinterpret_cast<std::uintptr_t>(name));
    pal4::GameDbEffectObjectState object{};
    object.index = {0x11223344u, 1u, 2u, 3u};
    object.records = {0x22334455u, 4u, 5u, 6u};
    object.tracks = {0x33445566u, 7u, 8u, 9u};
    object.name = {0x44556677u, 10u, 11u, 12u};
    object.metadata = {0x55667788u, 13u, 14u, 15u};
    object.lock_flags = 0xabcdef80u;
    Check(&pal4::GameDbEffectObject::Construct(object, name, runtime) ==
              &object &&
              object.vtable == pal4::GameDbEffectObject::kVtable &&
              object.index.flag == (0x11223300u | low_byte) &&
              object.records.flag == (0x22334400u | low_byte) &&
              object.tracks.flag == (0x33445500u | low_byte) &&
              object.name.flag == (0x44556600u | low_byte) &&
              object.metadata.flag == (0x55667700u | low_byte) &&
              object.index.begin == 0u && object.records.begin == 0u &&
              object.tracks.begin == 0u &&
              object.name.data == 0xa000u && object.name.length == 5u &&
              object.name.capacity == 5u &&
              object.metadata.data == 0u && object.key_index == 0u &&
              object.lock_flags == 0xabcdef00u &&
              runtime.events == std::vector<std::string>{
                  "allocate-text:5", "write-text:40960:magic"},
          "constructor initializes native fields and copies name");
    runtime.events.clear();
    object.metadata.data = 0xb000u;
    object.metadata.length = 3u;
    object.metadata.capacity = 4u;
    object.tracks.begin = 0xc000u;
    object.tracks.end = 0xc020u;
    object.tracks.capacity = 0xc030u;
    object.records.begin = 0xd000u;
    object.records.end = 0xd078u;
    object.records.capacity = 0xd0b4u;
    object.index.begin = 0xe000u;
    object.index.end = 0xe008u;
    object.index.capacity = 0xe010u;
    Check(pal4::GameDbEffectObject::Destroy(object, runtime) ==
              runtime.free_result &&
              runtime.events == std::vector<std::string>{
                  "clear-effect-data", "release-text:45056",
                  "release-text:40960", "cleanup-track:49152",
                  "cleanup-track:49168", "free:49152",
                  "cleanup-record:53248", "cleanup-record:53308",
                  "free:53248", "free:57344"},
          "destructor releases every owned field in native order");
    Check(object.vtable == pal4::GameDbEffectObject::kVtable &&
              object.name.data == 0u && object.name.length == 0u &&
              object.name.capacity == 0u &&
              object.metadata.data == 0u && object.metadata.length == 0u &&
              object.metadata.capacity == 0u &&
              object.index.begin == 0u && object.index.end == 0u &&
              object.index.capacity == 0u &&
              object.records.begin == 0u && object.records.end == 0u &&
              object.records.capacity == 0u &&
              object.tracks.begin == 0u && object.tracks.end == 0u &&
              object.tracks.capacity == 0u,
          "destructor resets owned pointers while retaining object identity");
}

void FailedAndEmptyName() {
    Runtime runtime;
    runtime.allocate_success = false;
    pal4::GameDbEffectObjectState object{};
    (void)pal4::GameDbEffectObject::Construct(object, "unavailable", runtime);
    Check(object.name.data == 0u && object.name.length == 0u &&
              object.name.capacity == 0u &&
              runtime.events ==
                  std::vector<std::string>{"allocate-text:11"},
          "failed name allocation still completes object construction");
    runtime.events.clear();
    runtime.allocate_success = true;
    (void)pal4::GameDbEffectObject::Construct(object, "", runtime);
    Check(object.name.length == 0u &&
              runtime.events == std::vector<std::string>{
                  "allocate-text:0", "write-text:40960:"},
          "empty name follows native allocator and terminator path");
    runtime.events.clear();
    (void)pal4::GameDbEffectObject::Destroy(object, runtime);
    Check(runtime.events == std::vector<std::string>{
              "clear-effect-data", "release-text:40960", "free:0",
              "free:0", "free:0"},
          "empty arrays still pass null buffers to native free");
}

void AddRowsInKeyOrder() {
    AddRuntime runtime;
    runtime.keys = {{1u, 10}, {2u, 30}, {3u, 20},
                    {4u, 5}, {5u, 20}, {6u, -2}};
    pal4::GameDbEffectObjectState object{};
    object.key_index = 7u;
    object.lock_flags = 1u;
    Check(!pal4::GameDbEffectObject::AddRow(object, 1u, runtime) &&
              runtime.events.empty(),
          "locked effect rejects a row before key lookup");
    object.lock_flags = 0u;
    Check(pal4::GameDbEffectObject::AddRow(object, 1u, runtime) &&
              object.index.begin == 0x8000u &&
              object.index.end == 0x8008u &&
              object.index.capacity == 0x8008u &&
              runtime.pairs[0x8000u].key == 10 &&
              runtime.pairs[0x8000u].track_index == 0u &&
              runtime.events == std::vector<std::string>{
                  "key:1:7", "append:1", "allocate:8", "free:0"},
          "first row appends track and allocates one sorted index slot");
    runtime.events.clear();
    runtime.next_index = 0x9000u;
    Check(pal4::GameDbEffectObject::AddRow(object, 2u, runtime) &&
              object.index.begin == 0x9000u &&
              object.index.end == 0x9010u &&
              object.index.capacity == 0x9010u &&
              runtime.pairs[0x9000u].key == 10 &&
              runtime.pairs[0x9008u].key == 30 &&
              runtime.pairs[0x9008u].track_index == 1u &&
              runtime.events == std::vector<std::string>{
                  "key:2:7", "append:2", "allocate:16", "free:32768"},
          "full index grows and preserves the earlier pair");
    runtime.events.clear();
    runtime.next_index = 0xa000u;
    Check(pal4::GameDbEffectObject::AddRow(object, 3u, runtime) &&
              object.index.begin == 0xa000u &&
              object.index.end == 0xa018u &&
              object.index.capacity == 0xa020u &&
              runtime.pairs[0xa000u].key == 10 &&
              runtime.pairs[0xa008u].key == 20 &&
              runtime.pairs[0xa008u].track_index == 2u &&
              runtime.pairs[0xa010u].key == 30 &&
              runtime.pairs[0xa010u].track_index == 1u &&
              runtime.events == std::vector<std::string>{
                  "key:3:7", "append:3", "allocate:32", "free:36864"},
          "middle insertion grows by old count and retains track indices");
    runtime.events.clear();
    Check(pal4::GameDbEffectObject::AddRow(object, 4u, runtime) &&
              object.index.end == 0xa020u &&
              runtime.pairs[0xa000u].key == 5 &&
              runtime.pairs[0xa008u].key == 10 &&
              runtime.pairs[0xa010u].key == 20 &&
              runtime.pairs[0xa018u].key == 30 &&
              runtime.pairs[0xa000u].track_index == 3u &&
              runtime.events ==
                  std::vector<std::string>{"key:4:7", "append:4"},
          "spare capacity shifts the index tail in place");
    runtime.events.clear();
    Check(pal4::GameDbEffectObject::AddRow(object, 5u, runtime) &&
              object.index.end == 0xa020u && object.tracks.end == 0x7040u &&
              runtime.events == std::vector<std::string>{
                  "key:5:7", "assign:28704:5"},
          "duplicate key replaces its existing track without appending");
    runtime.events.clear();
    runtime.next_index = 0xb000u;
    Check(pal4::GameDbEffectObject::AddRow(object, 6u, runtime) &&
              object.index.begin == 0xb000u &&
              object.index.end == 0xb028u &&
              object.index.capacity == 0xb040u &&
              runtime.pairs[0xb000u].key == -2 &&
              runtime.pairs[0xb008u].key == 5 &&
              runtime.pairs[0xb000u].track_index == 4u,
          "signed negative key sorts before nonnegative keys on growth");
    Check(pal4::GameDbEffectObject::TrackAt(object, 0u, runtime) == 0x7040u &&
              pal4::GameDbEffectObject::TrackAt(object, 2u, runtime) == 0x7000u &&
              pal4::GameDbEffectObject::TrackAt(object, 5u, runtime) == 0u,
          "sorted index maps to original track positions without track bounds");
    object.lock_flags = 1u;
    Check(pal4::GameDbEffectObject::TrackAt(object, 0u, runtime) == 0u,
          "locked effect hides otherwise valid track");
    object.lock_flags = 0u;
    object.index.begin = 0u;
    Check(pal4::GameDbEffectObject::TrackAt(object, 0u, runtime) == 0u,
          "null index buffer returns no track");
}

void AssignSchemaBranches() {
    AssignRuntime runtime;
    pal4::GameDbEffectObjectState object{};
    object.records = {0u, 0x1000u, 0x10b4u, 0x10b4u};
    Check(pal4::GameDbEffectObject::AssignSchema(
              object, object.records, 7u, 99u, runtime) &&
              object.records.begin == 0x1000u &&
              object.records.end == 0x10b4u &&
              runtime.events == std::vector<std::string>{
                  "clear", "finalize:99:7"},
          "self assignment skips array copy but clears and finalizes");
    runtime.events.clear();
    const pal4::GameDbValueArrayState one{0u, 0x2000u, 0x203cu, 0x203cu};
    Check(pal4::GameDbEffectObject::AssignSchema(
              object, one, 8u, 20u, runtime) &&
              object.records.begin == 0x1000u &&
              object.records.end == 0x103cu &&
              object.records.capacity == 0x10b4u &&
              runtime.events == std::vector<std::string>{
                  "clear", "assign:4096:8192", "destroy:4156",
                  "destroy:4216", "finalize:20:8"},
          "shrinking assigns prefix then destroys excess records");
    runtime.events.clear();
    const pal4::GameDbValueArrayState three{0u, 0x2000u, 0x20b4u, 0x20b4u};
    Check(pal4::GameDbEffectObject::AssignSchema(
              object, three, 9u, 21u, runtime) &&
              object.records.begin == 0x1000u &&
              object.records.end == 0x10b4u &&
              runtime.events == std::vector<std::string>{
                  "clear", "assign:4096:8192", "construct:4156:8252",
                  "construct:4216:8312", "finalize:21:9"},
          "growth within capacity assigns old and constructs new records");
    runtime.events.clear();
    object.records.end = object.records.capacity = 0x1078u;
    Check(pal4::GameDbEffectObject::AssignSchema(
              object, three, 10u, 22u, runtime) &&
              object.records.begin == 0x3000u &&
              object.records.end == 0x30b4u &&
              object.records.capacity == 0x30b4u &&
              runtime.events == std::vector<std::string>{
                  "clear", "destroy:4096", "destroy:4156", "free:4096",
                  "allocate:180", "construct:12288:8192",
                  "construct:12348:8252", "construct:12408:8312",
                  "finalize:22:10"},
          "insufficient capacity destroys old records and copy-constructs new");
    runtime.events.clear();
    const pal4::GameDbValueArrayState empty{};
    Check(pal4::GameDbEffectObject::AssignSchema(
              object, empty, 11u, 23u, runtime) &&
              object.records.begin == 0x3000u &&
              object.records.end == 0x3000u &&
              runtime.events == std::vector<std::string>{
                  "clear", "destroy:12288", "destroy:12348",
                  "destroy:12408", "finalize:23:11"},
          "empty source destroys every record without releasing capacity");
}

void ClearDataPreservesCapacity() {
    ClearRuntime runtime;
    pal4::GameDbEffectObjectState object{};
    object.index = {0x91u, 0x1000u, 0x1010u, 0x1020u};
    object.records = {0x92u, 0x2000u, 0x2078u, 0x20b4u};
    object.tracks = {0x93u, 0x3000u, 0x3020u, 0x3040u};
    Check(pal4::GameDbEffectObject::ClearData(object, runtime) == 0x203du &&
              runtime.events == std::vector<std::string>{
                  "track:12288", "track:12304",
                  "record:8192", "record:8252"},
          "clear processes tracks then records and returns last record result");
    Check(object.index.begin == 0x1000u && object.index.end == 0x1000u &&
              object.index.capacity == 0x1020u &&
              object.records.begin == 0x2000u &&
              object.records.end == 0x2000u &&
              object.records.capacity == 0x20b4u &&
              object.tracks.begin == 0x3000u &&
              object.tracks.end == 0x3000u &&
              object.tracks.capacity == 0x3040u,
          "clear resets live ends while retaining all allocated buffers");
    runtime.events.clear();
    object.index.end = 0x1008u;
    Check(pal4::GameDbEffectObject::ClearData(object, runtime) == 0x1008u &&
              runtime.events.empty(),
          "empty track and record arrays return the old index end");
}

void LockedTrackAppendBranches() {
    TrackRuntime runtime;
    pal4::GameDbEffectObjectState object{};
    object.tracks = {0u, 0x7000u, 0x7000u, 0x7020u};
    Check(pal4::GameDbEffectObject::AddLockedTrack(
              object, 5u, runtime) == 0u && runtime.events.empty() &&
              object.tracks.end == 0x7000u,
          "unlocked effect ignores track append");
    object.lock_flags = 1u;
    Check(pal4::GameDbEffectObject::AddLockedTrack(
              object, 5u, runtime) == 0x10u &&
              object.tracks.end == 0x7010u &&
              runtime.events == std::vector<std::string>{
                  "fill:28672:5", "events:28672:5"},
          "spare capacity fills and assigns events before advancing end");
    runtime.events.clear();
    object.tracks.capacity = object.tracks.end;
    Check(pal4::GameDbEffectObject::AddLockedTrack(
              object, 6u, runtime) == 0u &&
              object.tracks.begin == 0x8100u &&
              object.tracks.end == 0x8120u &&
              object.tracks.capacity == 0x8120u &&
              runtime.events == std::vector<std::string>{
                  "allocate:32", "construct:33024:28672",
                  "fill:33040:6", "cleanup:28672", "free:28672"},
          "full one-track array grows and releases its old storage");
    runtime.events.clear();
    runtime.next_memory = 0x9200u;
    Check(pal4::GameDbEffectObject::AddLockedTrack(
              object, 7u, runtime) == 0u &&
              object.tracks.begin == 0x9200u &&
              object.tracks.end == 0x9230u &&
              object.tracks.capacity == 0x9240u &&
              runtime.events == std::vector<std::string>{
                  "allocate:64", "construct:37376:33024",
                  "construct:37392:33040", "fill:37408:7",
                  "cleanup:33024", "cleanup:33040", "free:33024"},
          "full two-track array doubles capacity and preserves copy order");
    runtime.events.clear();
    object.tracks = {0u, 0u, 0u, 0u};
    runtime.next_memory = 0xa020u;
    Check(pal4::GameDbEffectObject::AddLockedTrack(
              object, 8u, runtime) == 0u &&
              object.tracks.begin == 0xa020u &&
              object.tracks.end == 0xa030u &&
              object.tracks.capacity == 0xa030u &&
              runtime.events == std::vector<std::string>{
                  "allocate:16", "fill:40992:8", "free:0"},
          "empty locked array allocates and fills one track");
}

void EnableClearsOnlyLockByte() {
    EnableRuntime runtime;
    pal4::GameDbEffectObjectState object{};
    object.lock_flags = 0xaabbcc7fu;
    Check(pal4::GameDbEffectObject::Enable(object, runtime) == -19 &&
              object.lock_flags == 0xaabbcc00u &&
              runtime.observed_flags == 0xaabbcc00u &&
              runtime.calls == 1,
          "enable clears lock byte before dispatching virtual slot 24");
}

}  // namespace

int main() {
    ConstructAndDestroyPopulated();
    FailedAndEmptyName();
    AddRowsInKeyOrder();
    AssignSchemaBranches();
    ClearDataPreservesCapacity();
    LockedTrackAppendBranches();
    EnableClearsOnlyLockByte();
    ProcessAndSortEvents();
    ResizeArrayBranches();
    std::cout << "GameDB effect object tests passed\n";
}
