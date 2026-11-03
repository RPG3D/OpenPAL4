#include "pal4/animation_track.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IAnimationTrackRuntime {
    std::vector<std::string> events;
    std::uint32_t next_memory = 0x3000u;

    void SetProperty(std::uint32_t destination,
                     std::uint32_t source) override {
        events.push_back("set:" + std::to_string(destination) + ":" +
                         std::to_string(source));
    }
    void ConstructProperty(std::uint32_t destination,
                           std::uint32_t source) override {
        events.push_back("construct:" + std::to_string(destination) + ":" +
                         std::to_string(source));
    }
    void DestroyProperty(std::uint32_t address) override {
        events.push_back("destroy:" + std::to_string(address));
    }
    void CleanupProperty(std::uint32_t address) override {
        events.push_back("cleanup:" + std::to_string(address));
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        events.push_back("allocate:" + std::to_string(bytes));
        return next_memory;
    }
    void Free(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
    }
};

struct ClearRuntime final : pal4::IAnimationTrackClearRuntime {
    std::vector<std::string> events;
    void CleanupProperties(std::uint32_t address) override {
        events.push_back("cleanup:" + std::to_string(address));
    }
    void Free(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
    }
};

struct RangeRuntime final : pal4::IAnimationTrackRangeRuntime {
    std::vector<std::string> events;
    std::uint32_t CleanupProperties(std::uint32_t address) override {
        events.push_back("cleanup:" + std::to_string(address));
        return address + 1u;
    }
    void CopyConstruct(std::uint32_t destination,
                       std::uint32_t source) override {
        events.push_back("copy:" + std::to_string(destination) + ":" +
                         std::to_string(source));
    }
};

struct InsertRuntime final : pal4::IAnimationTrackInsertRuntime {
    std::map<std::uint32_t, int> values;
    std::vector<std::string> events;
    std::uint32_t next_memory = 0x5000u;

    void CopyConstruct(std::uint32_t destination,
                       std::uint32_t source) override {
        events.push_back("copy:" + std::to_string(destination) + ":" +
                         std::to_string(source));
        values[destination] = values.at(source);
    }
    void Assign(std::uint32_t destination,
                std::uint32_t source) override {
        events.push_back("assign:" + std::to_string(destination) + ":" +
                         std::to_string(source));
        values[destination] = values.at(source);
    }
    void CleanupProperties(std::uint32_t address) override {
        events.push_back("cleanup:" + std::to_string(address));
        values.erase(address);
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        events.push_back("allocate:" + std::to_string(bytes));
        return next_memory;
    }
    void Free(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
    }
};

struct KeyRuntime final : pal4::IAnimationTrackKeyRuntime {
    std::map<std::uint32_t, std::int32_t> keys;
    std::vector<std::string> events;
    std::int32_t GetPropertyValue(std::uint32_t track,
                                  std::uint32_t property_index) override {
        events.push_back("key:" + std::to_string(track) + ":" +
                         std::to_string(property_index));
        return keys.at(track);
    }
};

struct BatchRuntime final : pal4::IAnimationTrackBatchRuntime {
    std::map<std::uint32_t, pal4::AnimationTrackState> tracks;
    Runtime properties;
    pal4::AnimationTrackState& TrackAt(std::uint32_t address) override {
        return tracks.at(address);
    }
    pal4::IAnimationTrackRuntime& PropertyRuntime() override {
        return properties;
    }
};

struct PropertyDestroyRuntime final : pal4::IAnimationTrackPropertyDestroyRuntime {
    std::vector<std::string> events;
    void CleanupProperty(std::uint32_t address) override {
        events.push_back("cleanup:" + std::to_string(address));
    }
    void Free(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
    }
};

struct DistinctRuntime final : pal4::IAnimationTrackDistinctRuntime {
    std::map<std::uint32_t, std::int32_t> keys;
    std::vector<std::string> events;
    std::int32_t GetTrackKey(std::uint32_t track,
                             std::uint32_t property_index) override {
        Check(property_index == 5u, "distinct run uses selected property");
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
};

void CopyDistinctRuns() {
    DistinctRuntime runtime;
    Check(pal4::AnimationTrack::CopyDistinctRuns(
              0x1000u, 0x1000u, 0x2000u, 5u, runtime) == 0x2000u &&
              runtime.events.empty(),
          "empty source preserves destination cursor");
    runtime.keys = {{0x1000u, 1}, {0x1010u, 1}, {0x1020u, 2},
                    {0x1030u, 2}, {0x1040u, 1}};
    Check(pal4::AnimationTrack::CopyDistinctRuns(
              0x1000u, 0x1050u, 0x1000u, 5u, runtime) == 0x1030u &&
              runtime.keys.at(0x1000u) == 1 &&
              runtime.keys.at(0x1010u) == 2 &&
              runtime.keys.at(0x1020u) == 1 &&
              runtime.events == std::vector<std::string>{
                  "assign:4096:4096", "copy:4112:4128",
                  "copy:4128:4160"},
          "adjacent runs compact while separated equal keys remain");
    runtime.events.clear();
    runtime.keys = {{0x2000u, -2}, {0x2010u, 3}};
    Check(pal4::AnimationTrack::CopyDistinctRuns(
              0x2000u, 0x2020u, 0x2000u, 5u, runtime) == 0x2020u &&
              runtime.events == std::vector<std::string>{
                  "assign:8192:8192"},
          "distinct adjacent tracks need no shifted property copy");
}

void ConstructAndDestroy() {
    Runtime runtime;
    pal4::AnimationTrackState destination{0xaabbcc11u, 0u, 0u, 0u};
    const pal4::AnimationTrackState source{
        0x12345644u, 0x2000u, 0x2010u, 0x2020u};
    pal4::AnimationTrack::CopyConstruct(&destination, source, runtime);
    Check(destination.flag == 0xaabbcc44u &&
              destination.begin == 0x3000u &&
              destination.end == destination.capacity &&
              destination.end == 0x3010u &&
              runtime.events == std::vector<std::string>{
                  "allocate:16", "construct:12288:8192",
                  "construct:12296:8200"},
          "copy construction copies low flag byte and owned properties");
    runtime.events.clear();
    pal4::AnimationTrack::CopyConstruct(nullptr, source, runtime);
    Check(runtime.events.empty(), "null track destination has no side effects");
    const pal4::AnimationTrackState empty{};
    pal4::AnimationTrack::CopyConstruct(&destination, empty, runtime);
    Check(destination.flag == 0xaabbcc00u &&
              destination.begin == destination.end &&
              destination.end == destination.capacity &&
              runtime.events == std::vector<std::string>{"allocate:0"},
          "empty copy still allocates zero bytes and preserves upper flag bits");

    PropertyDestroyRuntime property;
    Check(pal4::AnimationTrackProperty::Destroy(
              0x4000u, 0u, property) == 0x4000u &&
              property.events == std::vector<std::string>{"cleanup:16384"},
          "property destructor without delete flag only cleans value");
    property.events.clear();
    Check(pal4::AnimationTrackProperty::Destroy(
              0x4000u, 3u, property) == 0x4000u &&
              property.events == std::vector<std::string>{
                  "cleanup:16384", "free:16384"},
          "property destructor frees object after cleanup when bit zero set");

    ClearRuntime track;
    Check(pal4::AnimationTrack::DestroyAt(
              0x5000u, 0u, track) == 0x5000u &&
              track.events == std::vector<std::string>{"cleanup:20480"},
          "track destructor without delete flag only cleans properties");
    track.events.clear();
    Check(pal4::AnimationTrack::DestroyAt(
              0x5000u, 1u, track) == 0x5000u &&
              track.events == std::vector<std::string>{
                  "cleanup:20480", "free:20480"},
          "track destructor frees object after property cleanup");
}

void AssignEventsRange() {
    BatchRuntime runtime;
    runtime.tracks[0x1000u] = {0u, 0x4000u, 0x4008u, 0x4010u};
    runtime.tracks[0x1010u] = {0u, 0x5000u, 0x5008u, 0x5008u};
    runtime.tracks[0x1020u] = {};
    Check(pal4::AnimationTrack::AssignEvents(
              0x1000u, 0x1030u, 0x1010u, runtime) == 0x1030u &&
              runtime.tracks.at(0x1000u).begin == 0x4000u &&
              runtime.tracks.at(0x1020u).begin == 0x3000u &&
              runtime.tracks.at(0x1020u).end == 0x3008u &&
              runtime.properties.events == std::vector<std::string>{
                  "set:16384:20480", "free:0", "allocate:8",
                  "construct:12288:20480"},
          "batch assignment skips source slot and assigns other tracks in order");
    runtime.properties.events.clear();
    Check(pal4::AnimationTrack::AssignEvents(
              0x1000u, 0x1000u, 0x1010u, runtime) == 0x1000u &&
              runtime.properties.events.empty(),
          "empty batch returns end without property operations");
}

void MoveEventsBackward() {
    BatchRuntime runtime;
    runtime.tracks[0x1000u] = {0u, 0x4000u, 0x4008u, 0x4008u};
    runtime.tracks[0x1010u] = {0u, 0x5000u, 0x5008u, 0x5008u};
    runtime.tracks[0x1020u] = {0u, 0x6000u, 0x6008u, 0x6008u};
    runtime.tracks[0x1030u] = {0u, 0x7000u, 0x7008u, 0x7008u};
    Check(pal4::AnimationTrack::MoveEvents(
              0x1000u, 0x1030u, 0x1040u, runtime) == 0x1010u &&
              runtime.properties.events == std::vector<std::string>{
                  "set:28672:24576", "set:24576:20480",
                  "set:20480:16384"},
          "overlapping right move assigns highest source first");
    runtime.properties.events.clear();
    Check(pal4::AnimationTrack::MoveEvents(
              0x1000u, 0x1020u, 0x1020u, runtime) == 0x1000u &&
              runtime.properties.events.empty(),
          "same-address move skips self assignment");
    Check(pal4::AnimationTrack::MoveEvents(
              0x1000u, 0x1000u, 0x1040u, runtime) == 0x1040u &&
              runtime.properties.events.empty(),
          "empty move returns original target end");
}

void CompareAndFill() {
    KeyRuntime keys;
    keys.keys = {{0x1000u, -7}, {0x1010u, -7}, {0x1020u, 2}};
    Check(pal4::AnimationTrack::CompareEvents(
              3u, 0x1000u, 0x1010u, keys) &&
              keys.events == std::vector<std::string>{
                  "key:4096:3", "key:4112:3"},
          "comparison queries first and second tracks using selected index");
    keys.events.clear();
    Check(!pal4::AnimationTrack::CompareEvents(
              3u, 0x1000u, 0x1020u, keys) &&
              keys.events == std::vector<std::string>{
                  "key:4096:3", "key:4128:3"},
          "comparison rejects different signed property values");

    RangeRuntime runtime;
    pal4::AnimationTrack::Fill(0x2000u, 3u, 0x3000u, runtime);
    Check(runtime.events == std::vector<std::string>{
              "copy:8192:12288", "copy:8208:12288",
              "copy:8224:12288"},
          "fill constructs exactly three consecutive 16-byte tracks");
    runtime.events.clear();
    pal4::AnimationTrack::Fill(0x2000u, 0u, 0x3000u, runtime);
    Check(runtime.events.empty(), "zero-count fill has no constructor call");
}

void InsertBranches() {
    InsertRuntime runtime;
    runtime.values = {{0x1000u, 1}, {0x1010u, 2},
                      {0x1020u, 3}, {0x1030u, 4}, {0x9000u, 9}};
    pal4::AnimationTrackState array{0u, 0x1000u, 0x1040u, 0x1080u};
    pal4::AnimationTrack::Insert(array, 0x1010u, 2u, 0x9000u, runtime);
    Check(array.end == 0x1060u && array.capacity == 0x1080u &&
              runtime.values.at(0x1000u) == 1 &&
              runtime.values.at(0x1010u) == 9 &&
              runtime.values.at(0x1020u) == 9 &&
              runtime.values.at(0x1030u) == 2 &&
              runtime.values.at(0x1040u) == 3 &&
              runtime.values.at(0x1050u) == 4 &&
              runtime.events == std::vector<std::string>{
                  "copy:4160:4128", "copy:4176:4144",
                  "assign:4144:4112", "assign:4112:36864",
                  "assign:4128:36864"},
          "long tail moves last tracks then assigns backward and fills gap");
    runtime.events.clear();
    pal4::AnimationTrack::Insert(array, 0x1020u, 0u, 0x9000u, runtime);
    Check(array.end == 0x1060u && runtime.events.empty(),
          "zero insertion with spare capacity does nothing");

    runtime.events.clear();
    runtime.values = {{0x2000u, 1}, {0x2010u, 2},
                      {0x2020u, 3}, {0x9000u, 9}};
    array = {0u, 0x2000u, 0x2030u, 0x2080u};
    pal4::AnimationTrack::Insert(array, 0x2020u, 3u, 0x9000u, runtime);
    Check(array.end == 0x2060u &&
              runtime.values.at(0x2020u) == 9 &&
              runtime.values.at(0x2030u) == 9 &&
              runtime.values.at(0x2040u) == 9 &&
              runtime.values.at(0x2050u) == 3 &&
              runtime.events == std::vector<std::string>{
                  "copy:8272:8224", "copy:8240:36864",
                  "copy:8256:36864", "assign:8224:36864"},
          "short tail copies displaced track and constructs remaining gap");

    runtime.events.clear();
    runtime.values = {{0x3000u, 1}, {0x3010u, 2}, {0x9000u, 9}};
    array = {0u, 0x3000u, 0x3020u, 0x3020u};
    pal4::AnimationTrack::Insert(array, 0x3010u, 2u, 0x9000u, runtime);
    Check(array.begin == 0x5000u && array.end == 0x5040u &&
              array.capacity == 0x5040u &&
              runtime.values.at(0x5000u) == 1 &&
              runtime.values.at(0x5010u) == 9 &&
              runtime.values.at(0x5020u) == 9 &&
              runtime.values.at(0x5030u) == 2 &&
              runtime.events == std::vector<std::string>{
                  "allocate:64", "copy:20480:12288",
                  "copy:20496:36864", "copy:20512:36864",
                  "copy:20528:12304", "cleanup:12288",
                  "cleanup:12304", "free:12288"},
          "full array allocates prefix, repeated values and suffix before cleanup");

    runtime.events.clear();
    runtime.next_memory = 0x6000u;
    array = {};
    pal4::AnimationTrack::Insert(array, 0u, 1u, 0x9000u, runtime);
    Check(array.begin == 0x6000u && array.end == 0x6010u &&
              array.capacity == 0x6010u &&
              runtime.values.at(0x6000u) == 9 &&
              runtime.events == std::vector<std::string>{
                  "allocate:16", "copy:24576:36864", "free:0"},
          "empty array allocates first track and forwards null free");
}

void RangeHelpers() {
    RangeRuntime runtime;
    Check(pal4::AnimationTrack::CleanupRange(0x1000u, 0x1030u, runtime) ==
                  0x1021u &&
              runtime.events == std::vector<std::string>{
                  "cleanup:4096", "cleanup:4112", "cleanup:4128"},
          "cleanup range forwards each 16-byte block and final result");
    runtime.events.clear();
    Check(pal4::AnimationTrack::CleanupRange(0x1000u, 0x1000u, runtime) ==
                  0u && runtime.events.empty(),
          "empty cleanup range has no native cleanup call");
    Check(pal4::AnimationTrack::CopyRange(
              0x2000u, 0x2030u, 0x3000u, runtime) == 0x3030u &&
              runtime.events == std::vector<std::string>{
                  "copy:12288:8192", "copy:12304:8208",
                  "copy:12320:8224"},
          "copy range constructs in source order and returns target end");
    runtime.events.clear();
    Check(pal4::AnimationTrack::CopyRange(
              0x2000u, 0x2000u, 0x3000u, runtime) == 0x3000u &&
              runtime.events.empty(),
          "empty copy range returns its original target");
}

void ClearArray() {
    ClearRuntime runtime;
    pal4::AnimationTrackState track{0x42u, 0x1000u, 0x1020u, 0x1040u};
    Check(pal4::AnimationTrack::Clear(track, runtime) == 0u &&
              track.flag == 0x42u && track.begin == 0u &&
              track.end == 0u && track.capacity == 0u &&
              runtime.events == std::vector<std::string>{
                  "cleanup:4096", "cleanup:4112", "free:4096"},
          "clear visits 16-byte blocks and zeros only array pointers");
    runtime.events.clear();
    Check(pal4::AnimationTrack::Clear(track, runtime) == 0u &&
              runtime.events == std::vector<std::string>{"free:0"},
          "empty clear still forwards null storage to free");
}

void AssignBranches() {
    Runtime runtime;
    pal4::AnimationTrackState destination{0x1234u, 0x1000u, 0x1018u, 0x1020u};
    const pal4::AnimationTrackState one{0u, 0x2000u, 0x2008u, 0x2008u};
    Check(&pal4::AnimationTrack::Assign(destination, destination, runtime) ==
                  &destination && runtime.events.empty(),
          "self-assignment returns without changing owned properties");
    Check(&pal4::AnimationTrack::Assign(destination, one, runtime) ==
                  &destination &&
              destination.end == 0x1008u &&
              destination.capacity == 0x1020u &&
              destination.flag == 0x1234u &&
              runtime.events == std::vector<std::string>{
                  "set:4096:8192", "destroy:4104", "destroy:4112"},
          "shrinking assigns prefix and destroys surplus values");

    runtime.events.clear();
    const pal4::AnimationTrackState three{0u, 0x2000u, 0x2018u, 0x2018u};
    Check(&pal4::AnimationTrack::Assign(destination, three, runtime) ==
                  &destination && destination.end == 0x1018u &&
              destination.capacity == 0x1020u &&
              runtime.events == std::vector<std::string>{
                  "set:4096:8192", "construct:4104:8200",
                  "construct:4112:8208"},
          "in-capacity growth sets existing values then constructs new ones");

    runtime.events.clear();
    destination.capacity = destination.end;
    const pal4::AnimationTrackState five{0u, 0x2000u, 0x2028u, 0x2028u};
    Check(&pal4::AnimationTrack::Assign(destination, five, runtime) ==
                  &destination &&
              destination.begin == 0x3000u &&
              destination.end == 0x3028u &&
              destination.capacity == 0x3028u &&
              runtime.events == std::vector<std::string>{
                  "cleanup:4096", "cleanup:4104", "cleanup:4112",
                  "free:4096", "allocate:40",
                  "construct:12288:8192", "construct:12296:8200",
                  "construct:12304:8208", "construct:12312:8216",
                  "construct:12320:8224"},
          "reallocation cleans old values before copying into new storage");

    runtime.events.clear();
    const pal4::AnimationTrackState empty{};
    Check(&pal4::AnimationTrack::Assign(destination, empty, runtime) ==
                  &destination &&
              destination.begin == destination.end &&
              destination.capacity == 0x3028u &&
              runtime.events == std::vector<std::string>{
                  "destroy:12288", "destroy:12296", "destroy:12304",
                  "destroy:12312", "destroy:12320"},
          "empty source destroys live properties and retains storage");
}

}  // namespace

int main() {
    AssignBranches();
    ClearArray();
    RangeHelpers();
    InsertBranches();
    CompareAndFill();
    AssignEventsRange();
    MoveEventsBackward();
    ConstructAndDestroy();
    CopyDistinctRuns();
    std::cout << "Animation track tests passed\n";
}
