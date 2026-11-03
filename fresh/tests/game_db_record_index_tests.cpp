#include "pal4/game_db_record_index.h"
#include "pal4/utilities_text_record_copy.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Memory final : pal4::IGameDbRecordIndexMemory {
    std::uint32_t next = 0x1000u;
    std::vector<std::uint32_t> sizes;
    std::vector<std::uint32_t> freed;
    std::vector<std::tuple<std::uint32_t, std::uint32_t,
                           std::uint32_t>> writes;
    std::map<std::uint32_t, std::uint32_t> words;
    std::uint32_t Allocate(std::uint32_t bytes) override {
        sizes.push_back(bytes);
        const auto address = next;
        next += 0x100u;
        return address;
    }
    void WriteWord(std::uint32_t address, std::uint32_t offset,
                   std::uint32_t value) override {
        writes.emplace_back(address, offset, value);
        words[address + offset] = value;
    }
    void Free(std::uint32_t address) override { freed.push_back(address); }
};

struct CleanupRuntime final : pal4::IGameDbRecordIndexCleanupRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint32_t> successors;
    std::vector<std::string> events;
    bool reset_end = true;
    void ClearRecords(pal4::GameDbRecordIndexState& state) override {
        events.push_back("clear-records");
        if (reset_end) state.words[2u] = state.words[1u];
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        events.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    std::uint32_t FirstElement(std::uint32_t head) override {
        events.push_back("first");
        return words.at(head);
    }
    void DestroySubtree(std::uint32_t root) override {
        events.push_back("destroy:" + std::to_string(root));
    }
    std::uint32_t NextNode(std::uint32_t node) override {
        events.push_back("next:" + std::to_string(node));
        return successors.at(node);
    }
    void EraseNode(std::uint32_t node) override {
        events.push_back("erase:" + std::to_string(node));
    }
    void ResetRecord(std::uint32_t address,
                     std::uint32_t flag) override {
        Check(flag == 1u, "record reset flag");
        events.push_back("reset:" + std::to_string(address));
    }
    int Free(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
        return 47;
    }
};

struct CallbackRuntime final : pal4::IGameDbRecordIndexCallbackRuntime {
    std::uint32_t calls = 0u;
    std::uint32_t result = 0u;
    std::uint32_t Invoke(std::uint32_t callback, std::int32_t argument,
                         std::uint32_t index_address) override {
        Check(callback == 0x7000u && argument == -9 &&
                  index_address == 0x3000u,
              "callback receives object, argument and index owner");
        ++calls;
        return result;
    }
};

struct ClearRuntime final : pal4::IGameDbRecordIndexClearRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint32_t> objects;
    std::map<std::uint32_t, std::uint32_t> successors;
    std::vector<std::string> events;
    std::uint32_t RecordObjectAt(std::uint32_t record) override {
        events.push_back("object:" + std::to_string(record));
        return objects[record];
    }
    void ResetRecordObject(std::uint32_t object,
                           std::uint32_t flag) override {
        Check(flag == 1u, "record object reset flag");
        events.push_back("reset-object:" + std::to_string(object));
    }
    void ReleaseRecordText(std::uint32_t record) override {
        events.push_back("release-text:" + std::to_string(record));
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        events.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    void DestroyRightSubtree(std::uint32_t root) override {
        events.push_back("destroy-right:" + std::to_string(root));
    }
    void CleanupNodePayload(std::uint32_t payload,
                            std::uint32_t flag) override {
        Check(flag == 0u, "tree payload cleanup flag");
        events.push_back("cleanup-payload:" + std::to_string(payload));
    }
    void FreeNode(std::uint32_t node) override {
        events.push_back("free-node:" + std::to_string(node));
    }
    std::uint32_t NextNode(std::uint32_t node) override {
        events.push_back("next:" + std::to_string(node));
        return successors.at(node);
    }
    void EraseNode(std::uint32_t node) override {
        events.push_back("erase:" + std::to_string(node));
    }
};

struct RegisterRuntime final : pal4::IGameDbRecordIndexRegisterRuntime {
    std::map<std::uint32_t, std::string> names;
    std::vector<std::uint32_t> visited;
    std::vector<std::tuple<std::uint32_t, std::string,
                           std::uint32_t>> inserted;
    bool NameEquals(std::uint32_t record,
                    std::string_view name) override {
        visited.push_back(record);
        return names.at(record) == name;
    }
    void InsertCopy(pal4::GameDbRecordIndexState& state,
                    std::uint32_t position, std::string_view name,
                    std::uint32_t object) override {
        inserted.emplace_back(position, std::string(name), object);
        names[position] = name;
        state.words[2u] += 24u;
    }
};

struct LookupRuntime final : pal4::IGameDbRecordIndexLookupRuntime {
    std::map<std::uint32_t, std::string> names;
    std::map<std::uint32_t, std::uint32_t> objects;
    std::vector<std::uint32_t> visited;
    std::map<std::uint32_t, std::uint32_t> factories;
    std::vector<std::string> events;
    std::uint32_t created = 0u;
    std::string_view RecordName(std::uint32_t record) override {
        visited.push_back(record);
        return names.at(record);
    }
    std::uint32_t RecordObject(std::uint32_t record) override {
        return objects.at(record);
    }
    std::uint32_t RecordFactory(std::uint32_t record) override {
        return factories.at(record);
    }
    std::uint32_t InvokeFactory(std::uint32_t factory,
                                const char* name) override {
        events.push_back("factory:" + std::to_string(factory) + ":" + name);
        return created;
    }
    void StoreRecordObject(std::uint32_t record,
                           std::uint32_t object) override {
        events.push_back("store:" + std::to_string(record));
        objects[record] = object;
    }
};

struct EraseRuntime final : pal4::IGameDbRecordIndexEraseRuntime {
    struct Entry {
        std::string name;
        std::uint32_t object;
    };
    std::map<std::uint32_t, Entry> entries;
    std::vector<std::string> events;
    std::string_view RecordName(std::uint32_t record) override {
        events.push_back("name:" + std::to_string(record));
        return entries.at(record).name;
    }
    std::uint32_t RecordObject(std::uint32_t record) override {
        return entries.at(record).object;
    }
    void ResetRecordObject(std::uint32_t object,
                           std::uint32_t flag) override {
        Check(flag == 1u, "erase reset flag");
        events.push_back("reset:" + std::to_string(object));
    }
    void AssignRecord(std::uint32_t destination,
                      std::uint32_t source) override {
        events.push_back("assign:" + std::to_string(destination) + ":" +
                         std::to_string(source));
        entries[destination] = entries.at(source);
    }
    void ReleaseRecordText(std::uint32_t record) override {
        events.push_back("release:" + std::to_string(record));
    }
};

struct ArrayRuntime final : pal4::IGameDbRecordArrayRuntime {
    struct Record {
        std::string name;
        std::uint32_t factory;
        std::uint32_t object;
    };
    std::map<std::uint32_t, Record> records;
    std::vector<std::string> events;
    std::uint32_t next = 0xa000u;
    std::uint32_t Allocate(std::uint32_t bytes) override {
        events.push_back("allocate:" + std::to_string(bytes));
        const auto result = next;
        next += 0x1000u;
        return result;
    }
    void ConstructCopy(std::uint32_t destination,
                       std::uint32_t source) override {
        events.push_back("construct:" + std::to_string(destination) +
                         ":" + std::to_string(source));
        records[destination] = records.at(source);
    }
    void AssignCopy(std::uint32_t destination,
                    std::uint32_t source) override {
        events.push_back("assign:" + std::to_string(destination) + ":" +
                         std::to_string(source));
        records[destination] = records.at(source);
    }
    void ReleaseRecordText(std::uint32_t record) override {
        events.push_back("release:" + std::to_string(record));
    }
    void Free(std::uint32_t allocation) override {
        events.push_back("free:" + std::to_string(allocation));
    }
};

struct IntegratedRegisterRuntime final
    : pal4::IGameDbRecordIndexRegisterRuntime {
    ArrayRuntime& storage;
    explicit IntegratedRegisterRuntime(ArrayRuntime& runtime)
        : storage(runtime) {}
    bool NameEquals(std::uint32_t record,
                    std::string_view name) override {
        return storage.records.at(record).name == name;
    }
    void InsertCopy(pal4::GameDbRecordIndexState& state,
                    std::uint32_t position, std::string_view name,
                    std::uint32_t object) override {
        storage.records[0x9000u] = {std::string(name), 0u, object};
        pal4::GameDbRecordIndex::InsertRecordCopies(
            state, position, 1u, 0x9000u, storage);
    }
};

struct GameDbCopyRuntime final : pal4::IUtilitiesTextRecordCopyRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::uint32_t next_data = 0x7001u;
    std::uint32_t allocations = 0u;
    std::uint32_t shares = 0u;
    std::uint32_t detaches = 0u;
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    void Copy(std::uint32_t destination, std::uint32_t source,
              std::uint32_t length) override {
        for (auto i = 0u; i < length; ++i)
            bytes[destination + i] = bytes[source + i];
    }
    void ThrowRange() override { throw std::out_of_range("record range"); }
    void ThrowLength() override { throw std::length_error("record length"); }
    void DetachAndReset(pal4::StringBufferState&) override {
        ++detaches;
    }
    bool AllocateText(pal4::StringBufferState& state,
                      std::uint32_t length,
                      std::uint8_t mode) override {
        Check(mode == 1u, "GameDB copy allocates owned text");
        if (length == 0u) return false;
        state.data = next_data;
        state.capacity = length;
        bytes[next_data - 1u] = 0u;
        next_data += 0x100u;
        ++allocations;
        return true;
    }
    void SetTextSize(pal4::StringBufferState& state,
                     std::uint32_t length) override {
        state.length = length;
        bytes[state.data + length] = 0u;
    }
    void SetErrorFlag(pal4::StringBufferState&,
                      std::uint8_t flag) override {
        Check(flag == 1u, "GameDB shared text uses flag one");
        ++shares;
    }
    void Reallocate(pal4::StringBufferState& state,
                    std::uint32_t length) override {
        state.data = next_data;
        state.capacity = length;
        next_data += 0x100u;
    }
    void Free(std::uint32_t) override {}
    void Put(std::uint32_t record, std::uint8_t flag,
             std::uint32_t data, std::string_view name,
             std::uint8_t references,
             std::uint32_t factory, std::uint32_t object) {
        bytes[record] = flag;
        words[record + 4u] = data;
        words[record + 8u] = static_cast<std::uint32_t>(name.size());
        words[record + 12u] = static_cast<std::uint32_t>(name.size() + 4u);
        words[record + 16u] = factory;
        words[record + 20u] = object;
        if (data != 0u) {
            bytes[data - 1u] = references;
            for (std::size_t i = 0u; i < name.size(); ++i)
                bytes[data + static_cast<std::uint32_t>(i)] =
                    static_cast<std::uint8_t>(name[i]);
            bytes[data + static_cast<std::uint32_t>(name.size())] = 0u;
        }
    }
};

}  // namespace

int main() {
    pal4::GameDbRecordIndexSharedTree shared;
    Memory memory;
    pal4::GameDbRecordIndexState first;
    first.words.fill(0xffffffffu);
    Check(&pal4::GameDbRecordIndex::Initialize(
              first, shared, memory) == &first &&
              shared.nil == 0x1000u && shared.references == 1u &&
              first.words[5u] == 0x1100u &&
              first.words[0u] == 0u && first.words[1u] == 0u &&
              first.words[2u] == 0u && first.words[3u] == 0u &&
              first.words[4u] == 0u && first.words[6u] == 0u &&
              first.words[7u] == 0u &&
              memory.sizes == std::vector<std::uint32_t>{80u, 80u} &&
              memory.freed.empty(),
          "first owner initializes 32-byte state and shared nil");
    Check(memory.words[0x1000u] == 0u &&
              memory.words[0x1004u] == 0u &&
              memory.words[0x1008u] == 0u &&
              memory.words[0x104cu] == 1u &&
              memory.words[0x1100u] == 0x1100u &&
              memory.words[0x1104u] == 0x1000u &&
              memory.words[0x1108u] == 0x1100u &&
              memory.words[0x114cu] == 0u,
          "nil and head links use 80-byte native layout");

    pal4::GameDbRecordIndexState second;
    second.words.fill(0xdeadbeefu);
    Check(&pal4::GameDbRecordIndex::Initialize(
              second, shared, memory) == &second &&
              shared.nil == 0x1000u && shared.references == 2u &&
              second.words[5u] == 0x1300u &&
              second.words[0u] == 0u && second.words[7u] == 0u &&
              memory.sizes ==
                  std::vector<std::uint32_t>{80u, 80u, 80u, 80u} &&
              memory.freed == std::vector<std::uint32_t>{0x1200u} &&
              memory.words[0x1300u] == 0x1300u &&
              memory.words[0x1304u] == 0x1000u &&
              memory.words[0x1308u] == 0x1300u,
          "second owner reuses nil and frees candidate");
    Check(memory.writes[0u] == std::tuple{0x1000u, 4u, 0u} &&
              memory.writes[1u] == std::tuple{0x1000u, 76u, 1u} &&
              memory.writes[2u] == std::tuple{0x1000u, 0u, 0u} &&
              memory.writes[3u] == std::tuple{0x1000u, 8u, 0u},
          "candidate sentinel writes follow IDA order");

    CleanupRuntime cleanup;
    pal4::GameDbRecordIndexState populated;
    populated.words[1u] = 0x4000u;
    populated.words[2u] = 0x4030u;
    populated.words[5u] = 0x1000u;
    populated.words[7u] = 2u;
    pal4::GameDbRecordIndexSharedTree owners{0x9000u, 2u};
    cleanup.words[0x1000u] = 0x2000u;
    cleanup.words[0x1004u] = 0x2000u;
    Check(pal4::GameDbRecordIndex::Cleanup(
              populated, owners, cleanup) == 47 &&
              populated.words[1u] == 0u && populated.words[2u] == 0u &&
              populated.words[3u] == 0u && populated.words[5u] == 0u &&
              populated.words[7u] == 0u &&
              owners.nil == 0x9000u && owners.references == 1u &&
              cleanup.words[0x1000u] == 0x1000u &&
              cleanup.words[0x1004u] == 0x9000u &&
              cleanup.words[0x1008u] == 0x1000u &&
              cleanup.events == std::vector<std::string>{
                  "clear-records", "read:4096", "first", "read:4100",
                  "destroy:8192", "write:4100", "write:4096",
                  "write:4104", "first", "free:4096", "free:16384"},
          "populated cleanup resets tree and retains shared nil");

    cleanup.events.clear();
    cleanup.reset_end = false;
    pal4::GameDbRecordIndexState residual;
    residual.words[1u] = 0x5000u;
    residual.words[2u] = 0x5030u;
    residual.words[5u] = 0x1100u;
    cleanup.words[0x1100u] = 0x6000u;
    cleanup.successors[0x6000u] = 0x7000u;
    cleanup.successors[0x7000u] = 0x1100u;
    Check(pal4::GameDbRecordIndex::Cleanup(
              residual, owners, cleanup) == 47 &&
              owners.nil == 0u && owners.references == 0u &&
              cleanup.events == std::vector<std::string>{
                  "clear-records", "read:4352", "next:24576",
                  "erase:24576", "next:28672", "erase:28672",
                  "free:4352", "free:36864", "reset:20480",
                  "reset:20504", "free:20480"},
          "residual nodes and records clear before last nil release");

    cleanup.events.clear();
    cleanup.reset_end = true;
    pal4::GameDbRecordIndexState empty;
    empty.words[5u] = 0x1200u;
    cleanup.words[0x1200u] = 0x1200u;
    pal4::GameDbRecordIndexSharedTree no_nil{0u, 1u};
    Check(pal4::GameDbRecordIndex::Cleanup(
              empty, no_nil, cleanup) == 47 &&
              cleanup.events == std::vector<std::string>{
                  "clear-records", "read:4608", "free:4608", "free:0"},
          "empty cleanup releases head and null array buffer");

    cleanup.events.clear();
    pal4::GameDbRecordIndexState tree;
    tree.words[5u] = 0x1300u;
    tree.words[7u] = 2u;
    cleanup.words[0x1300u] = 0x2400u;
    cleanup.words[0x1304u] = 0x2400u;
    pal4::GameDbRecordIndexSharedTree tree_shared{0x9000u, 2u};
    Check(pal4::GameDbRecordIndex::CleanupTree(
              tree, tree_shared, cleanup) == 1 &&
              tree.words[5u] == 0u && tree.words[7u] == 0u &&
              tree_shared.nil == 0x9000u && tree_shared.references == 1u &&
              cleanup.words[0x1300u] == 0x1300u &&
              cleanup.words[0x1304u] == 0x9000u &&
              cleanup.words[0x1308u] == 0x1300u &&
              cleanup.events == std::vector<std::string>{
                  "read:4864", "read:4868", "destroy:9216",
                  "write:4868", "write:4864", "write:4872",
                  "first", "free:4864"},
          "tree-only cleanup resets populated tree and retains nil");

    cleanup.events.clear();
    pal4::GameDbRecordIndexState linked;
    linked.words[5u] = 0x1400u;
    cleanup.words[0x1400u] = 0x2500u;
    cleanup.successors[0x2500u] = 0x1400u;
    Check(pal4::GameDbRecordIndex::CleanupTree(
              linked, tree_shared, cleanup) == 47 &&
              tree_shared.nil == 0u && tree_shared.references == 0u &&
              cleanup.events == std::vector<std::string>{
                  "read:5120", "next:9472", "erase:9472",
                  "free:5120", "free:36864"},
          "tree-only residual path releases last nil");

    cleanup.events.clear();
    pal4::GameDbRecordIndexState empty_tree;
    empty_tree.words[5u] = 0x1500u;
    cleanup.words[0x1500u] = 0x1500u;
    pal4::GameDbRecordIndexSharedTree empty_shared{0u, 1u};
    Check(pal4::GameDbRecordIndex::CleanupTree(
              empty_tree, empty_shared, cleanup) == 0 &&
              cleanup.events == std::vector<std::string>{
                  "read:5376", "free:5376"},
          "tree-only empty path skips null nil free");

    CallbackRuntime callback;
    Check(pal4::GameDbRecordIndex::InvokeLoader(
              0x3000u, -9, 0u, callback) == 0u &&
              callback.calls == 0u,
          "null callback returns zero without invocation");
    callback.result = 0x1234u;
    Check(pal4::GameDbRecordIndex::InvokeLoader(
              0x3000u, -9, 0x7000u, callback) == 0x34u &&
              callback.calls == 1u,
          "non-null callback forwards arguments and low return byte");

    ClearRuntime clear;
    pal4::GameDbRecordIndexState records;
    records.words[1u] = 0x4000u;
    records.words[2u] = 0x4030u;
    records.words[5u] = 0x1000u;
    records.words[7u] = 3u;
    pal4::GameDbRecordIndexSharedTree clear_shared{0x9000u, 1u};
    clear.objects[0x4000u] = 0x5000u;
    clear.words[0x1000u] = 0x6000u;
    clear.words[0x1004u] = 0x6000u;
    clear.words[0x6000u] = 0x7000u;
    clear.words[0x6008u] = 0x8000u;
    clear.words[0x7000u] = 0x9000u;
    clear.words[0x7008u] = 0x9000u;
    Check(pal4::GameDbRecordIndex::ClearRecords(
              records, clear_shared, clear) == 0x1000u &&
              records.words[2u] == 0x4000u && records.words[7u] == 0u &&
              clear.words[0x1000u] == 0x1000u &&
              clear.words[0x1004u] == 0x9000u &&
              clear.words[0x1008u] == 0x1000u &&
              clear.events == std::vector<std::string>{
                  "object:16384", "reset-object:20480",
                  "object:16408", "release-text:16384",
                  "release-text:16408", "read:4096", "read:4100",
                  "read:24584", "destroy-right:32768",
                  "read:24576", "cleanup-payload:24588",
                  "free-node:24576", "read:28680",
                  "destroy-right:36864", "read:28672",
                  "cleanup-payload:28684", "free-node:28672",
                  "write:4100", "write:4096", "write:4104"},
          "record objects, text and tree payloads clear in native order");

    clear.events.clear();
    pal4::GameDbRecordIndexState residual_records;
    residual_records.words[5u] = 0x1100u;
    clear.words[0x1100u] = 0x6000u;
    clear.successors[0x6000u] = 0x1100u;
    Check(pal4::GameDbRecordIndex::ClearRecords(
              residual_records, clear_shared, clear) == 0x1100u &&
              clear.events == std::vector<std::string>{
                  "read:4352", "next:24576", "erase:24576"},
          "zero-count tree removes residual node");

    clear.events.clear();
    pal4::GameDbRecordIndexState empty_records;
    empty_records.words[5u] = 0x1200u;
    clear.words[0x1200u] = 0x1200u;
    Check(pal4::GameDbRecordIndex::ClearRecords(
              empty_records, clear_shared, clear) == 0x1200u &&
              clear.events == std::vector<std::string>{"read:4608"},
          "empty record and tree collection remains empty");

    RegisterRuntime registration;
    pal4::GameDbRecordIndexState registry;
    registry.words[1u] = 0x4000u;
    registry.words[2u] = 0x4030u;
    registration.names[0x4000u] = "shop";
    registration.names[0x4018u] = "skill";
    Check(!pal4::GameDbRecordIndex::RegisterNamedRecord(
              registry, nullptr, 0u, registration) &&
              registration.visited.empty() && registration.inserted.empty(),
          "null object exits before reading the name");
    Check(!pal4::GameDbRecordIndex::RegisterNamedRecord(
              registry, "shop", 0x5000u, registration) &&
              registration.visited == std::vector<std::uint32_t>{0x4000u} &&
              registration.inserted.empty(),
          "first exact name blocks duplicate insertion");
    registration.visited.clear();
    Check(!pal4::GameDbRecordIndex::RegisterNamedRecord(
              registry, "skill", 0x5000u, registration) &&
              registration.visited ==
                  std::vector<std::uint32_t>{0x4000u, 0x4018u},
          "second exact name stops search at match");
    registration.visited.clear();
    Check(pal4::GameDbRecordIndex::RegisterNamedRecord(
              registry, "SHOP", 0x5100u, registration) &&
              registry.words[2u] == 0x4048u &&
              registration.visited ==
                  std::vector<std::uint32_t>{0x4000u, 0x4018u} &&
              registration.inserted.back() ==
                  std::tuple{0x4030u, std::string("SHOP"), 0x5100u},
          "different case reaches array append through comparison boundary");
    registration.visited.clear();
    Check(pal4::GameDbRecordIndex::RegisterNamedRecord(
              registry, "new\0ignored", 0x5200u, registration) &&
              registration.inserted.back() ==
                  std::tuple{0x4048u, std::string("new"), 0x5200u},
          "C string input ends at first NUL");

    LookupRuntime lookup;
    pal4::GameDbRecordIndexState index;
    index.words[1u] = 0x4000u;
    index.words[2u] = 0x4000u;
    Check(pal4::GameDbRecordIndex::FindNamedRecord(
              index, "x", lookup) == 0u && lookup.visited.empty(),
          "empty index returns zero without reading records");
    index.words[2u] = 0x4060u;
    lookup.names = {{0x4000u, "shop"}, {0x4018u, "shop"},
                    {0x4030u, "shopper"}, {0x4048u, ""}};
    lookup.objects = {{0x4000u, 0x5000u}, {0x4018u, 0x6000u},
                      {0x4030u, 0x7000u}, {0x4048u, 0x8000u}};
    Check(pal4::GameDbRecordIndex::FindNamedRecord(
              index, "shop", lookup) == 0x5000u &&
              lookup.visited == std::vector<std::uint32_t>{0x4000u},
          "first duplicate wins without visiting later slots");
    lookup.visited.clear();
    Check(pal4::GameDbRecordIndex::FindNamedRecord(
              index, "shopper", lookup) == 0x7000u &&
              lookup.visited ==
                  std::vector<std::uint32_t>{0x4000u, 0x4018u, 0x4030u},
          "prefix is rejected until exact length and bytes match");
    lookup.visited.clear();
    Check(pal4::GameDbRecordIndex::FindNamedRecord(
              index, "SHOP", lookup) == 0u &&
              lookup.visited.size() == 4u,
          "lookup is byte-exact and case-sensitive");
    lookup.visited.clear();
    Check(pal4::GameDbRecordIndex::FindNamedRecord(
              index, "", lookup) == 0x8000u &&
              pal4::GameDbRecordIndex::FindNamedRecord(
                  index, "shop\0hidden", lookup) == 0x5000u,
          "empty and embedded-NUL query names follow C strings");

    lookup.visited.clear();
    Check(pal4::GameDbRecordIndex::FindOrLoadNamedRecord(
              index, "missing", lookup) == 0u &&
              lookup.visited.size() == 4u && lookup.events.empty(),
          "lazy lookup does not invoke factory for missing names");
    lookup.visited.clear();
    Check(pal4::GameDbRecordIndex::FindOrLoadNamedRecord(
              index, "shop", lookup) == 0x5000u &&
              lookup.visited == std::vector<std::uint32_t>{0x4000u} &&
              lookup.events.empty(),
          "existing first duplicate object bypasses factory");
    lookup.objects[0x4030u] = 0u;
    lookup.factories[0x4030u] = 0x9000u;
    lookup.created = 0xa000u;
    lookup.visited.clear();
    Check(pal4::GameDbRecordIndex::FindOrLoadNamedRecord(
              index, "shopper", lookup) == 0xa000u &&
              lookup.objects[0x4030u] == 0xa000u &&
              lookup.visited ==
                  std::vector<std::uint32_t>{0x4000u, 0x4018u, 0x4030u} &&
              lookup.events == std::vector<std::string>{
                  "factory:36864:shopper", "store:16432"},
          "lazy lookup invokes factory once and caches the object");
    lookup.events.clear();
    Check(pal4::GameDbRecordIndex::FindOrLoadNamedRecord(
              index, "shopper", lookup) == 0xa000u &&
              lookup.events.empty(),
          "cached object suppresses repeated factory call");
    lookup.objects[0x4030u] = 0u;
    lookup.created = 0u;
    lookup.events.clear();
    Check(pal4::GameDbRecordIndex::FindOrLoadNamedRecord(
              index, "shopper", lookup) == 0u &&
              lookup.events == std::vector<std::string>{
                  "factory:36864:shopper", "store:16432"},
          "null factory result is stored and returned");

    pal4::GameDbRecordIndexState erasing;
    erasing.words[1u] = 0x4000u;
    erasing.words[2u] = 0x4060u;
    EraseRuntime erase;
    erase.entries = {{0x4000u, {"first", 0x5000u}},
                     {0x4018u, {"target", 0x6000u}},
                     {0x4030u, {"last", 0u}},
                     {0x4048u, {"target", 0x7000u}}};
    pal4::GameDbRecordIndex::RemoveNamedRecord(
        erasing, "missing", erase);
    Check(erasing.words[2u] == 0x4060u && erase.events ==
              std::vector<std::string>{"name:16384", "name:16408",
                                       "name:16432", "name:16456"},
          "missing erase leaves array unchanged");
    erase.events.clear();
    pal4::GameDbRecordIndex::RemoveNamedRecord(
        erasing, "target", erase);
    Check(erasing.words[2u] == 0x4048u &&
              erase.entries.at(0x4018u).name == "last" &&
              erase.entries.at(0x4030u).object == 0x7000u &&
              erase.events == std::vector<std::string>{
                  "name:16384", "name:16408", "reset:24576",
                  "assign:16408:16432", "assign:16432:16456",
                  "release:16456"},
          "erase destroys first match and shifts following records");
    erase.events.clear();
    pal4::GameDbRecordIndex::RemoveNamedRecord(
        erasing, "last", erase);
    Check(erasing.words[2u] == 0x4030u &&
              erase.events == std::vector<std::string>{
                  "name:16384", "name:16408", "assign:16408:16432",
                  "release:16432"},
          "null object is skipped during erase");
    erase.events.clear();
    pal4::GameDbRecordIndex::RemoveNamedRecord(
        erasing, "target", erase);
    Check(erasing.words[2u] == 0x4018u &&
              erase.events == std::vector<std::string>{
                  "name:16384", "name:16408", "reset:28672",
                  "release:16408"},
          "last record erase does not shift array");

    std::uint32_t cursor = 17u;
    pal4::GameDbRecordIndexState first_record;
    Check(pal4::GameDbRecordIndex::FirstRecord(first_record, cursor) == 0u &&
              cursor == 17u,
          "null array leaves cursor untouched");
    first_record.words[1u] = 0x4000u;
    first_record.words[2u] = 0x4000u;
    Check(pal4::GameDbRecordIndex::FirstRecord(first_record, cursor) == 0u &&
              cursor == 17u,
          "empty allocated array leaves cursor untouched");
    first_record.words[2u] = 0x4018u;
    Check(pal4::GameDbRecordIndex::FirstRecord(first_record, cursor) ==
              0x4000u && cursor == 0u,
          "nonempty array returns begin and resets cursor");
    first_record.words[2u] = 0x4048u;
    Check(pal4::GameDbRecordIndex::NextRecord(first_record, cursor) ==
              0x4018u && cursor == 1u &&
              pal4::GameDbRecordIndex::NextRecord(first_record, cursor) ==
                  0x4030u && cursor == 2u &&
              pal4::GameDbRecordIndex::NextRecord(first_record, cursor) ==
                  0u && cursor == 3u,
          "next record advances through all slots and beyond end");
    first_record.words[1u] = 0u;
    Check(pal4::GameDbRecordIndex::NextRecord(first_record, cursor) == 0u &&
              cursor == 4u,
          "null array still increments caller cursor");

    ArrayRuntime array_runtime;
    array_runtime.records[0x9000u] = {"x", 17u, 29u};
    pal4::GameDbRecordIndexState array;
    pal4::GameDbRecordIndex::InsertRecordCopies(
        array, 0u, 2u, 0x9000u, array_runtime);
    Check(array.words[1u] == 0xa000u &&
              array.words[2u] == 0xa030u &&
              array.words[3u] == 0xa030u &&
              array_runtime.records.at(0xa000u).name == "x" &&
              array_runtime.records.at(0xa018u).object == 29u &&
              array_runtime.events == std::vector<std::string>{
                  "allocate:48", "construct:40960:36864",
                  "construct:40984:36864", "free:0"},
          "empty array grows exactly to inserted count");

    array_runtime.events.clear();
    array.words[1u] = 0x4000u;
    array.words[2u] = 0x4060u;
    array.words[3u] = 0x40c0u;
    array_runtime.records[0x4000u] = {"a", 1u, 1u};
    array_runtime.records[0x4018u] = {"b", 2u, 2u};
    array_runtime.records[0x4030u] = {"c", 3u, 3u};
    array_runtime.records[0x4048u] = {"d", 4u, 4u};
    pal4::GameDbRecordIndex::InsertRecordCopies(
        array, 0x4018u, 1u, 0x9000u, array_runtime);
    Check(array.words[2u] == 0x4078u &&
              array.words[3u] == 0x40c0u &&
              array_runtime.records.at(0x4000u).name == "a" &&
              array_runtime.records.at(0x4018u).name == "x" &&
              array_runtime.records.at(0x4030u).name == "b" &&
              array_runtime.records.at(0x4048u).name == "c" &&
              array_runtime.records.at(0x4060u).name == "d" &&
              array_runtime.events == std::vector<std::string>{
                  "construct:16480:16456", "assign:16456:16432",
                  "assign:16432:16408", "assign:16408:36864"},
          "in-place insertion shifts a tail at least as long as count");

    array_runtime.events.clear();
    array.words[1u] = 0x5000u;
    array.words[2u] = 0x5030u;
    array.words[3u] = 0x5090u;
    array_runtime.records[0x5000u] = {"a", 1u, 1u};
    array_runtime.records[0x5018u] = {"b", 2u, 2u};
    pal4::GameDbRecordIndex::InsertRecordCopies(
        array, 0x5018u, 3u, 0x9000u, array_runtime);
    Check(array.words[2u] == 0x5078u &&
              array_runtime.records.at(0x5000u).name == "a" &&
              array_runtime.records.at(0x5018u).name == "x" &&
              array_runtime.records.at(0x5030u).name == "x" &&
              array_runtime.records.at(0x5048u).name == "x" &&
              array_runtime.records.at(0x5060u).name == "b" &&
              array_runtime.events == std::vector<std::string>{
                  "construct:20576:20504", "construct:20528:36864",
                  "construct:20552:36864", "assign:20504:36864"},
          "in-place insertion with shorter tail constructs extra values");

    array_runtime.events.clear();
    array.words[1u] = 0x6000u;
    array.words[2u] = 0x6030u;
    array.words[3u] = 0x6030u;
    array_runtime.records[0x6000u] = {"a", 1u, 1u};
    array_runtime.records[0x6018u] = {"b", 2u, 2u};
    pal4::GameDbRecordIndex::InsertRecordCopies(
        array, 0x6018u, 1u, 0x9000u, array_runtime);
    Check(array.words[1u] == 0xb000u &&
              array.words[2u] == 0xb048u &&
              array.words[3u] == 0xb060u &&
              array_runtime.records.at(0xb000u).name == "a" &&
              array_runtime.records.at(0xb018u).name == "x" &&
              array_runtime.records.at(0xb030u).name == "b" &&
              array_runtime.events == std::vector<std::string>{
                  "allocate:96", "construct:45056:24576",
                  "construct:45080:36864", "construct:45104:24600",
                  "release:24576", "release:24600", "free:24576"},
          "full array doubles capacity and preserves prefix and suffix");
    array_runtime.events.clear();
    pal4::GameDbRecordIndex::InsertRecordCopies(
        array, array.words[2u], 0u, 0x9000u, array_runtime);
    Check(array.words[2u] == 0xb048u && array_runtime.events.empty(),
          "zero-count insertion leaves the array untouched");

    ArrayRuntime integrated_storage;
    IntegratedRegisterRuntime integrated(integrated_storage);
    pal4::GameDbRecordIndexState integrated_index;
    Check(pal4::GameDbRecordIndex::RegisterNamedRecord(
              integrated_index, "shop", 0x6000u, integrated) &&
              integrated_index.words[1u] == 0xa000u &&
              integrated_storage.records.at(0xa000u).name == "shop" &&
              integrated_storage.records.at(0xa000u).object == 0x6000u &&
              !pal4::GameDbRecordIndex::RegisterNamedRecord(
                  integrated_index, "shop", 0x7000u, integrated) &&
              integrated_index.words[2u] == 0xa018u,
          "named registration composes with reconstructed array insertion");

    GameDbCopyRuntime copy_runtime;
    copy_runtime.Put(0x2000u, 0x5au, 0x3001u, "shop", 2u,
                     0x6000u, 0x7000u);
    pal4::GameDbRecordIndex::CopyRecord24(
        0x1000u, 0x2000u, copy_runtime);
    Check(copy_runtime.bytes[0x1000u] == 0x5au &&
              copy_runtime.words[0x1004u] == 0x3001u &&
              copy_runtime.words[0x1008u] == 4u &&
              copy_runtime.words[0x100cu] == 8u &&
              copy_runtime.words[0x1010u] == 0x6000u &&
              copy_runtime.words[0x1014u] == 0x7000u &&
              copy_runtime.bytes[0x3000u] == 3u &&
              copy_runtime.shares == 1u && copy_runtime.allocations == 0u,
          "GameDB copy shares eligible text and copies factory/object");
    copy_runtime.Put(0x2100u, 0x11u, 0x3101u, "item", 0xfeu,
                     0x6200u, 0x7200u);
    pal4::GameDbRecordIndex::CopyRecord24(
        0x1100u, 0x2100u, copy_runtime);
    Check(copy_runtime.words[0x1104u] == 0x7001u &&
              copy_runtime.words[0x1108u] == 4u &&
              copy_runtime.words[0x1110u] == 0x6200u &&
              copy_runtime.words[0x1114u] == 0x7200u &&
              copy_runtime.bytes[0x7001u] == 'i' &&
              copy_runtime.bytes[0x7005u] == 0u &&
              copy_runtime.bytes[0x3100u] == 0xfeu &&
              copy_runtime.allocations == 1u,
          "saturated GameDB name uses an independent terminated copy");
    copy_runtime.Put(0x2200u, 0u, 0u, "", 0u, 0x6300u, 0x7300u);
    pal4::GameDbRecordIndex::CopyRecord24(
        0x1200u, 0x2200u, copy_runtime);
    Check(copy_runtime.words[0x1204u] == 0u &&
              copy_runtime.words[0x1208u] == 0u &&
              copy_runtime.words[0x1210u] == 0x6300u &&
              copy_runtime.words[0x1214u] == 0x7300u,
          "empty GameDB name still copies factory and object");
    copy_runtime.Put(0x2300u, 0u, 0u, "", 0u, 0x6400u, 0x7400u);
    pal4::GameDbRecordIndex::CopyRecord24(
        0x2300u, 0x2300u, copy_runtime);
    Check(copy_runtime.detaches == 2u &&
              copy_runtime.words[0x2310u] == 0x6400u &&
              copy_runtime.words[0x2314u] == 0x7400u,
          "self-copy follows native detach and preserves tail fields");
    const auto before_copy = copy_runtime.allocations;
    pal4::GameDbRecordIndex::CopyRecord24(
        0u, 0x2000u, copy_runtime);
    Check(copy_runtime.allocations == before_copy,
          "null destination skips GameDB record copy");
}
