#include "pal4/world_path_planner.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using Planner = pal4::WorldPathPlanner;
using Record = pal4::PathwayRecord;

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IWorldPathRuntime {
    struct Invocation {
        pal4::WorldPathCall operation;
        std::vector<std::uint32_t> arguments;
        bool operator==(const Invocation&) const = default;
    };
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> allocation_sizes;
    std::vector<std::uint32_t> allocation_results;
    std::vector<std::uint32_t> frees;
    std::vector<std::uint32_t> stream_words;
    std::vector<std::uint8_t> stream_bytes;
    std::vector<std::array<std::uint32_t, 3>> stream_records;
    std::vector<std::uint32_t> record_destinations;
    std::vector<Invocation> calls;
    std::vector<std::uint32_t> closed_streams;
    std::string registered_name;
    std::vector<std::uint32_t> registration;
    std::string opened_directory;
    std::string opened_name;
    std::uint32_t stream_address = 0u;
    std::optional<pal4::WorldPathCall> fail_call;
    bool fail_record_read = false;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        return bytes[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t Allocate(const std::uint32_t size) override {
        allocation_sizes.push_back(size);
        if (allocation_results.empty()) return 0u;
        const auto value = allocation_results.front();
        allocation_results.erase(allocation_results.begin());
        return value;
    }
    std::uint32_t Free(const std::uint32_t pointer) override {
        frees.push_back(pointer);
        return pointer + 1u;
    }
    std::uint32_t ReadStreamWord(std::uint32_t) override {
        if (stream_words.empty()) return 0u;
        const auto value = stream_words.front();
        stream_words.erase(stream_words.begin());
        return value;
    }
    std::uint8_t ReadStreamByte(std::uint32_t) override {
        if (stream_bytes.empty()) return 0u;
        const auto value = stream_bytes.front();
        stream_bytes.erase(stream_bytes.begin());
        return value;
    }
    void ReadStreamRecord(std::uint32_t,
                          const std::uint32_t destination) override {
        if (fail_record_read) throw std::runtime_error("injected point read");
        record_destinations.push_back(destination);
        if (stream_records.empty()) return;
        const auto value = stream_records.front();
        stream_records.erase(stream_records.begin());
        for (auto index = 0u; index < value.size(); ++index)
            words[destination + index * 4u] = value[index];
    }
    std::uint32_t OpenPathwayStream(
        const std::string_view directory,
        const std::string_view name) override {
        opened_directory = directory;
        opened_name = name;
        return stream_address;
    }
    void ClosePathwayStream(const std::uint32_t stream) override {
        closed_streams.push_back(stream);
    }
    std::uint32_t RegisterSingleton(
        const std::string_view name, const std::uint32_t id,
        const std::uint32_t factory,
        const std::uint32_t deleter) override {
        registered_name = name;
        registration = {id, factory, deleter};
        return 0x8500u;
    }
    std::uint32_t Call(const pal4::WorldPathCall operation,
                       const std::initializer_list<std::uint32_t> arguments)
        override {
        calls.push_back({operation, arguments});
        if (fail_call == operation) throw std::runtime_error("injected operation");
        if (operation == pal4::WorldPathCall::ConstructGrid)
            return *arguments.begin();
        if (operation == pal4::WorldPathCall::DestroyGrid)
            return 0xbeefu;
        return 0u;
    }
};

void StreamRecordLifecycle() {
    Runtime runtime;
    constexpr std::uint32_t record = 0x7000u;
    constexpr std::uint32_t stream = 0x6000u;
    runtime.stream_words = {0u};
    Check(Record::Load(record, stream, runtime) == 0u &&
              runtime.calls == (std::vector<Runtime::Invocation>{
                  {pal4::WorldPathCall::ReadString, {stream, record}}}) &&
              runtime.allocation_sizes.empty(),
          "record without grid stops after string and presence flag");

    runtime = Runtime{};
    runtime.stream_words = {1u, 0x3344u, 2u};
    runtime.stream_bytes = {0x6cu};
    runtime.stream_records = {{1u, 2u, 3u}, {4u, 5u, 6u}};
    runtime.allocation_results = {0x8000u, 0x9000u};
    Check(Record::Load(record, stream, runtime) == 0x8001u &&
              runtime.allocation_sizes ==
                  (std::vector<std::uint32_t>{24u, 0x58u}) &&
              runtime.record_destinations ==
                  (std::vector<std::uint32_t>{0x8000u, 0x800cu}) &&
              runtime.words[0x8000u] == 1u &&
              runtime.words[0x8004u] == 2u &&
              runtime.words[0x8008u] == 3u &&
              runtime.words[0x800cu] == 4u &&
              runtime.words[0x8010u] == 5u &&
              runtime.words[0x8014u] == 6u &&
              runtime.words[record + 12u] == 0x9000u &&
              runtime.frees == (std::vector<std::uint32_t>{0x8000u}) &&
              runtime.calls == (std::vector<Runtime::Invocation>{
                  {pal4::WorldPathCall::ReadString, {stream, record}},
                  {pal4::WorldPathCall::ConstructGrid, {0x9000u, 1u}},
                  {pal4::WorldPathCall::InitializeGrid,
                   {0x9000u, 2u, 0x8000u, 0x3344u, 0x6cu}}}),
          "pathway stream creates grid from two 12-byte records then frees data");
    runtime.calls.clear();
    Check(Record::Release(record, runtime) == 0xbeefu &&
              runtime.words[record + 12u] == 0u &&
              runtime.calls == (std::vector<Runtime::Invocation>{
                  {pal4::WorldPathCall::DestroyGrid, {0x9000u, 1u}}}),
          "resource release destroys grid and clears owning pointer");
    runtime.calls.clear();
    Check(Record::Release(record, runtime) == 0u && runtime.calls.empty(),
          "repeated resource release is harmless");

    runtime = Runtime{};
    runtime.stream_words = {1u, 2u, 1u};
    runtime.allocation_results = {0u};
    Check(Record::Load(record, stream, runtime) == 0u &&
              runtime.words[record + 12u] == 0u && runtime.frees.empty(),
          "temporary record allocation failure does not create grid");
    runtime = Runtime{};
    runtime.stream_words = {1u, 2u, 1u};
    runtime.allocation_results = {0x8000u, 0u};
    Check(Record::Load(record, stream, runtime) == 0u &&
              runtime.words[record + 12u] == 0u &&
              runtime.frees == (std::vector<std::uint32_t>{0x8000u}),
          "grid allocation failure frees temporary path points");
}

void PlannerLifecycle() {
    Runtime runtime;
    std::uint8_t guard = 0x80u;
    std::uint32_t registered = 0u;
    Check(Planner::Register(guard, registered, runtime) == 0x8500u &&
              guard == 0x81u && registered == 0x8500u &&
              runtime.registered_name == "worldPathPlanner" &&
              runtime.registration ==
                  (std::vector<std::uint32_t>{1026u, 0x522e30u,
                                               0x522e90u}),
          "world path singleton has confirmed ID and callbacks");
    runtime.registered_name.clear();
    Check(Planner::Register(guard, registered, runtime) == 0x8500u &&
              runtime.registered_name.empty(),
          "world path registration reuses cached singleton");
    runtime.allocation_results = {0x12340000u};
    Check(Planner::Create(runtime) == 0x12340000u &&
              runtime.allocation_sizes ==
                  (std::vector<std::uint32_t>{16u}) &&
              runtime.bytes[0x12340000u] == 0x12u &&
              runtime.words[0x12340004u] == 0u &&
              runtime.words[0x12340008u] == 0u &&
              runtime.words[0x1234000cu] == 0u,
          "factory initializes 16-byte planner and three array pointers");
    runtime.words[0x12340004u] = 0x5000u;
    Check(Planner::Destroy(0x12340000u, runtime) == 0x12340001u &&
              runtime.frees == (std::vector<std::uint32_t>{
                  0x5000u, 0x12340000u}) &&
              runtime.words[0x12340004u] == 0u &&
              runtime.words[0x12340008u] == 0u &&
              runtime.words[0x1234000cu] == 0u,
          "destructor frees pointer array before planner object");
    Check(Planner::Destroy(0u, runtime) == 0u &&
              runtime.frees.size() == 2u,
          "null planner destruction does nothing");
}

void PathListCleanup() {
    Runtime runtime;
    constexpr std::uint32_t planner = 0x6000u;
    runtime.words[planner + 4u] = 0x5000u;
    runtime.words[planner + 8u] = 0x5008u;
    runtime.words[0x5000u] = 0x7000u;
    runtime.words[0x5004u] = 0x7100u;
    runtime.words[0x7000u + 12u] = 0x9000u;
    Check(Planner::CleanupPathList(planner, runtime) == 0x5008u &&
              runtime.words[planner + 8u] == 0x5000u &&
              runtime.words[0x7000u + 12u] == 0u &&
              runtime.frees ==
                  (std::vector<std::uint32_t>{0x7000u, 0x7100u}) &&
              runtime.calls == (std::vector<Runtime::Invocation>{
                  {pal4::WorldPathCall::DestroyGrid, {0x9000u, 1u}},
                  {pal4::WorldPathCall::CleanupText, {0x7000u}},
                  {pal4::WorldPathCall::CleanupText, {0x7100u}}}),
          "path list releases grid, text and record for each pointer");
    runtime.calls.clear();
    Check(Planner::CleanupPathList(planner, runtime) == 0x5000u &&
              runtime.calls.empty(),
          "already empty path list stays empty");
    runtime.words[planner + 8u] = 0x5004u;
    runtime.words[0x5000u] = 0u;
    Check(Planner::CleanupPathList(planner, runtime) == 0x5004u &&
              runtime.calls.empty() && runtime.frees.size() == 2u,
          "null path entry is skipped without dereferencing it");
}

void FindAndLoadPathwayFile() {
    Runtime runtime;
    constexpr std::uint32_t planner = 0x6000u;
    runtime.words[planner + 4u] = 0x5000u;
    runtime.words[planner + 8u] = 0x5008u;
    runtime.words[0x5000u] = 0x7000u;
    runtime.words[0x5004u] = 0x7100u;
    runtime.words[0x7000u] = 0x8000u;
    runtime.words[0x7004u] = 5u;
    runtime.words[0x7100u] = 0x8100u;
    runtime.words[0x7104u] = 4u;
    for (auto i = 0u; i < 6u; ++i)
        runtime.bytes[0x8000u + i] = "alpha"[i];
    for (auto i = 0u; i < 5u; ++i) {
        runtime.bytes[0x8100u + i] = "beta"[i];
        runtime.bytes[0x8200u + i] = "beta"[i];
    }
    Check(Planner::Find(planner, 0x8200u, runtime) == 0x7100u &&
              Planner::Find(planner, 0x8000u, runtime) == 0x7000u,
          "lookup compares each name including its null terminator");
    runtime.bytes[0x8200u] = 'c';
    Check(Planner::Find(planner, 0x8200u, runtime) == 0u,
          "missing pathway name returns null");
    runtime.words[0x5000u] = 0u;
    Check(Planner::Find(planner, 0x8100u, runtime) == 0x7100u,
          "name lookup skips a null record pointer");

    runtime = Runtime{};
    Check(!Planner::LoadPathwayFile(planner, "Scene\\", "A01", runtime) &&
              runtime.opened_directory == "Scene\\" &&
              runtime.opened_name == "A01" &&
              runtime.closed_streams.empty(),
          "missing Pathway.pwy stream leaves planner untouched");

    runtime = Runtime{};
    runtime.stream_address = 0x9000u;
    runtime.stream_words = {2u, 0u, 0u};
    runtime.allocation_results = {0x7000u, 0x5000u,
                                  0x7100u, 0x5100u};
    Check(Planner::LoadPathwayFile(planner, "Scene\\", "A01", runtime) &&
              runtime.allocation_sizes ==
                  (std::vector<std::uint32_t>{20u, 4u, 20u, 8u}) &&
              runtime.words[planner + 4u] == 0x5100u &&
              runtime.words[planner + 8u] == 0x5108u &&
              runtime.words[planner + 12u] == 0x5108u &&
              runtime.words[0x5100u] == 0x7000u &&
              runtime.words[0x5104u] == 0x7100u &&
              runtime.frees ==
                  (std::vector<std::uint32_t>{0u, 0x5000u}) &&
              runtime.closed_streams ==
                  (std::vector<std::uint32_t>{0x9000u}) &&
              runtime.calls == (std::vector<Runtime::Invocation>{
                  {pal4::WorldPathCall::InitializeRecordText, {0x7000u}},
                  {pal4::WorldPathCall::ReadString, {0x9000u, 0x7000u}},
                  {pal4::WorldPathCall::InitializeRecordText, {0x7100u}},
                  {pal4::WorldPathCall::ReadString, {0x9000u, 0x7100u}}}),
          "pathway file loads two records and grows pointer array");

    runtime = Runtime{};
    runtime.stream_address = 0x9000u;
    runtime.stream_words = {1u};
    runtime.allocation_results = {0u};
    Check(!Planner::LoadPathwayFile(planner, "", "A01", runtime) &&
              runtime.closed_streams ==
                  (std::vector<std::uint32_t>{0x9000u}) &&
              runtime.frees.empty(),
          "record allocation failure still closes the pathway stream");

    runtime = Runtime{};
    runtime.stream_address = 0x9000u;
    runtime.stream_words = {1u, 0u};
    runtime.allocation_results = {0x7000u, 0u};
    Check(!Planner::LoadPathwayFile(planner, "", "A01", runtime) &&
              runtime.closed_streams ==
                  (std::vector<std::uint32_t>{0x9000u}) &&
              runtime.frees ==
                  (std::vector<std::uint32_t>{0x7000u}) &&
              runtime.calls.back() == Runtime::Invocation{
                  pal4::WorldPathCall::CleanupText, {0x7000u}},
          "array allocation failure releases new record and closes stream");
}

void ThrowingLoadCleanup() {
    for (const auto fail : {pal4::WorldPathCall::InitializeRecordText,
                           pal4::WorldPathCall::ReadString,
                           pal4::WorldPathCall::ConstructGrid,
                           pal4::WorldPathCall::InitializeGrid}) {
        Runtime runtime;
        runtime.stream_address=0x9000u;
        runtime.stream_words={1u,1u,0x3f800000u,2u};
        runtime.allocation_results={0x7000u,0x8000u,0xa000u};
        runtime.fail_call=fail;
        bool caught=false;
        try { (void)Planner::LoadPathwayFile(0x6000u,"Q02","Q02",runtime); }
        catch(const std::runtime_error&) { caught=true; }
        Check(caught && runtime.words[0x6008u]==0u &&
                  runtime.closed_streams==std::vector<std::uint32_t>{0x9000u} &&
                  runtime.frees.back()==0x7000u,
              "exception releases unattached record and closes stream");
        if(fail==pal4::WorldPathCall::ConstructGrid)
            Check(runtime.frees==std::vector<std::uint32_t>{0xa000u,0x8000u,0x7000u},
                  "throwing grid constructor frees raw storage and points");
        if(fail==pal4::WorldPathCall::InitializeGrid)
            Check(runtime.words[0x700cu]==0u &&
                      runtime.frees==std::vector<std::uint32_t>{0x8000u,0x7000u} &&
                      runtime.calls[runtime.calls.size()-2u].operation ==
                          pal4::WorldPathCall::DestroyGrid,
                  "throwing grid initializer destroys grid and frees temporary points");
    }
    Runtime runtime;
    runtime.stream_words={1u,0u,2u};
    runtime.allocation_results={0x8000u};
    runtime.fail_record_read=true;
    bool caught=false;
    try { (void)Record::Load(0x7000u,0x9000u,runtime); }
    catch(const std::runtime_error&) { caught=true; }
    Check(caught && runtime.frees==std::vector<std::uint32_t>{0x8000u},
          "truncated point read releases temporary coordinates");
}

}  // namespace

int main() {
    try {
        ThrowingLoadCleanup();
        StreamRecordLifecycle();
        PlannerLifecycle();
        PathListCleanup();
        FindAndLoadPathwayFile();
        pal4::WorldPathRuntimeStub stub;
        Check(Planner::Create(stub) == 0u,
              "native world path boundary remains linkable");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
