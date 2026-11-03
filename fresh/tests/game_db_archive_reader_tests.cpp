#include "pal4/game_db_archive_reader.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(const bool okay, const char* message) {
    if (!okay) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IGameDbArchiveTableRuntime {
    std::vector<std::uint8_t> input;
    std::size_t cursor = 0u;
    std::vector<std::string> events;
    std::map<std::uint32_t, std::string> strings;
    std::uint32_t next_data = 0x5000u;
    std::vector<std::uint32_t> find_results{0x7000u};
    std::size_t find_index = 0u;
    bool names_differ = false;

    void PutWord(std::uint32_t value) {
        for (auto shift = 0u; shift < 32u; shift += 8u)
            input.push_back(static_cast<std::uint8_t>(value >> shift));
    }
    void PutText(const std::string& value) {
        PutWord(static_cast<std::uint32_t>(value.size()));
        input.insert(input.end(), value.begin(), value.end());
    }
    void PutFloat(float value) {
        PutWord(std::bit_cast<std::uint32_t>(value));
    }
    std::uint32_t InvokeVirtual(std::uint32_t stream, std::uint32_t slot,
                                std::uintptr_t buffer,
                                std::uint32_t bytes) override {
        Check(stream == 0x9000u && slot == 12u,
              "archive reader dispatches the read slot");
        events.push_back("read:" + std::to_string(bytes));
        if (cursor + bytes > input.size()) return 0u;
        std::memcpy(reinterpret_cast<void*>(buffer), input.data() + cursor,
                    bytes);
        cursor += bytes;
        return 1u;
    }
    std::uint32_t ConstructProperty(const char* name) override {
        events.push_back(std::string("construct:") + name);
        return 0x6000u;
    }
    const char* TextData(const pal4::StringBufferState& text) override {
        return text.data == 0u ? "" : strings.at(text.data).c_str();
    }
    void SetPropertyLabel(std::uint32_t property,
                          const char* label) override {
        events.push_back("label:" + std::to_string(property) + ":" + label);
    }
    void AddEntry(std::uint32_t property, std::int32_t id,
                  const char* name) override {
        events.push_back("entry:" + std::to_string(property) + ":" +
                         std::to_string(id) + ":" + name);
    }
    void InsertProperty(std::uint32_t owner,
                        std::uint32_t property) override {
        events.push_back("insert:" + std::to_string(owner) + ":" +
                         std::to_string(property));
    }
    void ReleaseText(pal4::StringBufferState& text) override {
        events.push_back("release:" + std::string(TextData(text)));
        text = {};
    }
    void DestroyProperty(std::uint32_t property) override {
        events.push_back("destroy:" + std::to_string(property));
    }
    void LogBegin(const char* name) override {
        events.push_back(std::string("begin:") + name);
    }
    void LogEnd(const char* name) override {
        events.push_back(std::string("end:") + name);
    }
    std::uint32_t FindOrLoad(std::uint32_t index,
                             const char* name) override {
        events.push_back("find:" + std::to_string(index) + ":" + name);
        return find_results.at(find_index++);
    }
    void RegisterObject(std::uint32_t index,
                        const char* name) override {
        events.push_back("register:" + std::to_string(index) + ":" + name);
    }
    bool NamesDiffer(const char* actual,
                     const char* expected) override {
        events.push_back(std::string("compare:") + actual + ":" + expected);
        return names_differ;
    }
    void SetDescription(std::uint32_t object,
                        const char* description) override {
        events.push_back("description:" + std::to_string(object) + ":" +
                         description);
    }
    void ReserveSchema(std::uint32_t count) override {
        events.push_back("reserve:" + std::to_string(count));
    }
    void AddSchemaField(const char* name, std::uint32_t type,
                        const char* label, const char* detail) override {
        events.push_back(std::string("schema:") + name + ":" +
                         std::to_string(type) + ":" + label + ":" + detail);
    }
    void AssignSchema(std::uint32_t object, std::uint32_t attribute,
                      std::uint32_t rows) override {
        events.push_back("assign:" + std::to_string(object) + ":" +
                         std::to_string(attribute) + ":" +
                         std::to_string(rows));
    }
    void PrepareObject(std::uint32_t object) override {
        events.push_back("prepare:" + std::to_string(object));
    }
    void InitializeRows(std::uint32_t fields) override {
        events.push_back("initialize-rows:" + std::to_string(fields));
    }
    void AddDefaultValue() override { events.push_back("default"); }
    void ResetValue() override { events.push_back("reset-value"); }
    void SetInteger(std::uint32_t value) override {
        events.push_back("integer:" + std::to_string(value));
    }
    void SetText(const char* value) override {
        events.push_back(std::string("text:") + value);
    }
    void SetFloat(float value) override {
        events.push_back("float:" + std::to_string(value));
    }
    void CommitValue(std::uint32_t field) override {
        events.push_back("commit-value:" + std::to_string(field));
    }
    void CommitRow(std::uint32_t object) override {
        events.push_back("commit-row:" + std::to_string(object));
    }
    void FinalizeObject(std::uint32_t object) override {
        events.push_back("finalize:" + std::to_string(object));
    }
    void CleanupRows() override { events.push_back("cleanup-rows"); }
    void CleanupSchema() override { events.push_back("cleanup-schema"); }
    const char* EmptyData() override { return ""; }
    bool AllocateText(pal4::StringBufferState& text,
                      std::uint32_t length, std::uint8_t mode) override {
        Check(mode == 1u, "archive text uses mode one");
        events.push_back("allocate:" + std::to_string(length));
        text.data = next_data;
        text.capacity = length;
        next_data += 0x100u;
        return true;
    }
    void SetErrorFlag(pal4::StringBufferState& text,
                      std::uint8_t) override { text = {}; }
    void Reallocate(pal4::StringBufferState& text,
                    std::uint32_t length) override {
        text.data = next_data;
        text.capacity = length;
        next_data += 0x100u;
    }
    void SetTextSize(pal4::StringBufferState& text,
                     std::uint32_t length) override { text.length = length; }
    std::uint8_t ReadByte(std::uint32_t) override { return 0u; }
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    void Copy(std::uint32_t destination, const char* source,
              std::uint32_t length) override {
        strings[destination] = std::string(source, length);
    }
};

struct SchemaRuntime final : pal4::IGameDbSchemaArrayRuntime {
    std::map<std::uint32_t, std::uint32_t> values;
    std::vector<std::string> events;
    std::uint32_t next_allocation = 0xa000u;
    std::uint32_t Allocate(std::uint32_t bytes) override {
        events.push_back("allocate:" + std::to_string(bytes));
        const auto result = next_allocation;
        next_allocation += 0x1000u;
        return result;
    }
    void ConstructCopy(std::uint32_t destination,
                       std::uint32_t source) override {
        events.push_back("construct:" + std::to_string(destination) +
                         ":" + std::to_string(source));
        values[destination] = values.at(source);
    }
    void AssignCopy(std::uint32_t destination,
                    std::uint32_t source) override {
        events.push_back("assign:" + std::to_string(destination) +
                         ":" + std::to_string(source));
        values[destination] = values.at(source);
    }
    void Release(std::uint32_t element) override {
        events.push_back("release:" + std::to_string(element));
    }
    void Free(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
    }
};

struct SchemaElementRuntime final : pal4::IGameDbSchemaElementRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;
    bool require_empty_destination = true;
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        events.push_back("byte:" + std::to_string(address));
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        events.push_back("word:" + std::to_string(address) + ":" +
                         std::to_string(value));
    }
    void AssignText(std::uint32_t destination,
                    std::uint32_t source) override {
        if (require_empty_destination) {
            for (auto offset = 4u; offset <= 12u; offset += 4u)
                Check(words.at(destination + offset) == 0u,
                      "text fields are zero before copy assignment");
        }
        events.push_back("text:" + std::to_string(destination));
        for (auto offset = 4u; offset <= 12u; offset += 4u)
            words[destination + offset] = words.at(source + offset);
    }
};

struct SchemaOwnerRuntime final : pal4::IGameDbSchemaOwnerRuntime {
    std::map<std::uint32_t, std::uint32_t> values;
    std::map<std::uint32_t, std::uint32_t> pointers;
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
        values[destination] = values.at(source);
    }
    void AssignCopy(std::uint32_t destination,
                    std::uint32_t source) override {
        events.push_back("assign:" + std::to_string(destination) +
                         ":" + std::to_string(source));
        values[destination] = values.at(source);
    }
    void Release(std::uint32_t element) override {
        events.push_back("release:" + std::to_string(element));
    }
    void Free(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
    }
    void CopyPointers(std::uint32_t begin, std::uint32_t end,
                      std::uint32_t destination) override {
        events.push_back("copy-pointers:" + std::to_string(begin) +
                         ":" + std::to_string(end));
        for (auto input = begin, output = destination;
             input != end; input += 4u, output += 4u)
            pointers[output] = pointers.at(input);
    }
    void AppendNullPointer(pal4::GameDbSchemaOwnerState& owner) override {
        events.push_back("append-null");
        auto& end = owner.pointers[2u];
        pointers[end] = 0u;
        end += 4u;
        if (end > owner.pointers[3u])
            owner.pointers[3u] = end;
    }
};

struct SchemaBindingRuntime final : pal4::IGameDbSchemaBindingRuntime {
    std::map<std::uint32_t, std::uint32_t> pointers;
    std::map<std::uint32_t, std::string> names;
    std::vector<std::uint32_t> allocations{0x9000u, 0u};
    std::size_t allocation_index = 0u;
    std::vector<std::string> events;
    std::uint32_t callback_count = 0u;
    std::uint32_t ReadPointer(std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return pointers.at(address);
    }
    void WritePointer(std::uint32_t address,
                      std::uint32_t value) override {
        events.push_back("write:" + std::to_string(address));
        pointers[address] = value;
    }
    std::uint32_t AllocatePlaceholder(std::uint32_t bytes) override {
        events.push_back("allocate:" + std::to_string(bytes));
        return allocations.at(allocation_index++);
    }
    void WriteVtable(std::uint32_t,
                     std::uint32_t vtable) override {
        events.push_back(vtable == 0x849898u ? "base-vtable" :
                         "derived-vtable");
    }
    void InitializePlaceholderText(std::uint32_t field) override {
        events.push_back("initialize-text:" + std::to_string(field));
    }
    std::string_view SchemaName(std::uint32_t element) override {
        events.push_back("name:" + std::to_string(element));
        return names.at(element);
    }
    void SetExceptionMessage(std::string_view message) override {
        events.push_back("message:" + std::string(message));
    }
    std::uint32_t Invoke(std::uint32_t address) override {
        events.push_back("callback:" + std::to_string(address));
        ++callback_count;
        return 7u;
    }
};

struct PlaceholderRuntime final : pal4::IGameDbPlaceholderEffectRuntime {
    std::vector<std::string> events;
    void ReleaseText(std::uint32_t data) override {
        events.push_back("release:" + std::to_string(data));
    }
    void Free(std::uint32_t object) override {
        events.push_back("free:" + std::to_string(object));
    }
};

struct SchemaProcessRuntime final : pal4::IGameDbSchemaProcessRuntime {
    std::map<std::uint32_t, std::uint32_t> pointers;
    std::vector<std::string> events;
    std::uint32_t ReadPointer(std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return pointers.at(address);
    }
    void InvokeFirstVirtual(std::uint32_t object,
                            std::uint32_t flag) override {
        events.push_back("invoke:" + std::to_string(object) + ":" +
                         std::to_string(flag));
    }
};

struct BoundEffectRuntime final : pal4::IGameDbBoundEffectRuntime {
    std::vector<std::uint32_t> allocations{0x9000u, 0x9100u, 0u};
    std::size_t index = 0u;
    std::map<std::uint32_t, pal4::GameDbBoundEffectState> objects;
    std::vector<std::string> events;
    bool bind_result = true;
    std::uint32_t Allocate(std::uint32_t bytes) override {
        events.push_back("allocate:" + std::to_string(bytes));
        return allocations.at(index++);
    }
    pal4::GameDbBoundEffectState& Object(
        std::uint32_t address) override {
        return objects[address];
    }
    void AssignText(pal4::GameDbBoundEffectState& object,
                    std::uint32_t source) override {
        events.push_back("assign-text:" + std::to_string(source));
        object.text_data = source + 4u;
        object.text_length = 3u;
        object.text_capacity = 3u;
    }
    bool Bind(std::uint32_t owner, std::uint32_t source,
              std::uint32_t object, std::uint32_t kind) override {
        events.push_back("bind:" + std::to_string(owner) + ":" +
                         std::to_string(source) + ":" +
                         std::to_string(object) + ":" +
                         std::to_string(kind));
        return bind_result;
    }
};

struct SchemaBindRuntime final : pal4::IGameDbSchemaBindRuntime {
    std::map<std::uint32_t, std::string> schemas;
    std::map<std::uint32_t, std::uint32_t> kinds;
    std::map<std::uint32_t, std::string> sources;
    std::map<std::uint32_t, std::uint32_t> pointers;
    std::vector<std::string> events;
    std::uint32_t callback_slot = 0x1234u;
    std::string_view SchemaName(std::uint32_t element) override {
        events.push_back("schema-name:" + std::to_string(element));
        return schemas.at(element);
    }
    std::uint32_t SchemaKind(std::uint32_t element) override {
        events.push_back("schema-kind:" + std::to_string(element));
        return kinds.at(element);
    }
    std::string_view SourceName(std::uint32_t source) override {
        events.push_back("source-name:" + std::to_string(source));
        return sources.at(source);
    }
    std::uint32_t ReadPointer(std::uint32_t slot) override {
        events.push_back("read:" + std::to_string(slot));
        return pointers.at(slot);
    }
    void WritePointer(std::uint32_t slot,
                      std::uint32_t object) override {
        events.push_back("write:" + std::to_string(slot) + ":" +
                         std::to_string(object));
        pointers[slot] = object;
    }
    void DeleteObject(std::uint32_t object,
                      std::uint32_t flag) override {
        events.push_back("delete:" + std::to_string(object) + ":" +
                         std::to_string(flag));
    }
    std::uint32_t GlobalCallbackSlot() override { return callback_slot; }
    void SetExceptionMessage(std::string_view message) override {
        events.push_back("message:" + std::string(message));
    }
    std::uint32_t Invoke(std::uint32_t address) override {
        events.push_back("callback:" + std::to_string(address));
        return 0u;
    }
    std::uint32_t VirtualKind(std::uint32_t object) override {
        events.push_back("virtual-kind:" + std::to_string(object));
        return 2u;
    }
    std::uint32_t VirtualSource(std::uint32_t object) override {
        events.push_back("virtual-source:" + std::to_string(object));
        return 0x6000u;
    }
};

struct BoundPropertyRuntime final : pal4::IGameDbBoundEffectPropertyRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        events.push_back("write:" + std::to_string(address) + ":" +
                         std::to_string(value));
    }
    std::uint8_t AssignText(std::uint32_t destination,
                            std::uint32_t source) override {
        events.push_back("assign-text:" + std::to_string(destination) +
                         ":" + std::to_string(source));
        return 7u;
    }
    void SetErrorFlag(pal4::GameDbTypedValueErrorState& state,
                      std::uint8_t flag) override {
        events.push_back("flag:" + std::to_string(flag));
        state.flag = flag;
    }
    std::uint32_t AllocateText(std::uint32_t length) override {
        events.push_back("allocate:" + std::to_string(length));
        return 0xa000u;
    }
    void CopyText(std::uint32_t destination, const char* source,
                  std::uint32_t length) override {
        events.push_back("copy:" + std::to_string(destination) + ":" +
                         std::string(source, length));
    }
    void RegisterCleanup() override { events.push_back("register"); }
    void ReleaseText(std::uint32_t data) override {
        events.push_back("release:" + std::to_string(data));
    }
};

struct FactoryRuntime final : pal4::IGameDbObjectFactoryRuntime {
    std::uint32_t allocation = 0x7000u;
    std::vector<std::string> events;
    std::uint32_t Allocate(std::uint32_t bytes) override {
        events.push_back("allocate:" + std::to_string(bytes));
        return allocation;
    }
    std::uint32_t Construct(std::uint32_t object,
                            const char* name) override {
        events.push_back("construct:" + std::to_string(object) + ":" + name);
        return object + 4u;
    }
};

struct PropertyRuntime final : pal4::IGameDbPropertyRuntime {
    std::vector<std::string> events;
    std::map<std::uint32_t, std::int32_t> record_ids;
    std::map<std::uint32_t, std::string> record_texts;
    std::uint32_t allocation = 0x7000u;
    std::uint32_t text_allocation = 0x8000u;
    std::uint32_t Allocate(std::uint32_t bytes) override {
        events.push_back("allocate:" + std::to_string(bytes));
        return allocation;
    }
    std::uint32_t AllocateText(std::uint32_t length,
                               std::uint8_t mode) override {
        events.push_back("allocate-text:" + std::to_string(length) +
                         ":" + std::to_string(mode));
        return text_allocation;
    }
    void CopyText(std::uint32_t destination, const char* source,
                  std::uint32_t length) override {
        events.push_back("copy-text:" + std::to_string(destination) +
                         ":" + std::string(source, length));
    }
    void CleanupRecord(std::uint32_t address) override {
        events.push_back("cleanup:" + std::to_string(address));
    }
    void ConstructCopy(std::uint32_t destination,
                       std::uint32_t source) override {
        events.push_back("construct:" + std::to_string(destination) +
                         ":" + std::to_string(source));
        record_ids[destination] = record_ids.at(source);
    }
    void AssignCopy(std::uint32_t destination,
                    std::uint32_t source) override {
        events.push_back("assign:" + std::to_string(destination) +
                         ":" + std::to_string(source));
        record_ids[destination] = record_ids.at(source);
    }
    void ReleaseText(std::uint32_t address) override {
        events.push_back("release:" + std::to_string(address));
    }
    int Free(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
        return 7;
    }
    std::int32_t ReadRecordId(std::uint32_t address) override {
        events.push_back("read-id:" + std::to_string(address));
        return record_ids.at(address);
    }
    std::string_view ReadRecordText(std::uint32_t address) override {
        events.push_back("read-text:" + std::to_string(address));
        return record_texts.at(address);
    }
    void InsertRecord(pal4::GameDbPropertyState& state,
                      std::uint32_t position, std::int32_t id,
                      std::uint32_t text, std::uint32_t length) override {
        events.push_back("insert:" + std::to_string(position) + ":" +
                         std::to_string(id) + ":" +
                         std::to_string(text) + ":" +
                         std::to_string(length));
        state.words[2u] += 24u;
    }
};

}  // namespace

int main() {
    constexpr pal4::ArchiveStreamAdapterState stream{0x9000u, 0u, {}};
    Runtime invalid_start;
    invalid_start.PutWord(17u);
    Check(!pal4::GameDbArchiveReader::ReadPropertyRecord(
              "outer", 0x1000u, stream, invalid_start) &&
              invalid_start.events == std::vector<std::string>{"read:4"},
          "wrong start marker returns before constructing a property");

    Runtime valid;
    valid.PutWord(pal4::GameDbArchiveWriter::kPropertyStart);
    valid.PutText("display");
    valid.PutWord(2u);
    valid.PutWord(10u);
    valid.PutText("one");
    valid.PutWord(20u);
    valid.PutText("two");
    valid.PutWord(pal4::GameDbArchiveWriter::kPropertyEnd);
    Check(pal4::GameDbArchiveReader::ReadPropertyRecord(
              "outer", 0x1000u, stream, valid) &&
              valid.cursor == valid.input.size() &&
              valid.events == std::vector<std::string>{
                  "read:4", "construct:outer", "read:4", "read:7",
                  "allocate:7", "label:24576:display", "read:4",
                  "read:4", "read:4", "read:3", "allocate:3",
                  "entry:24576:10:one", "read:4", "read:4", "read:3",
                  "allocate:3", "entry:24576:20:two", "read:4",
                  "insert:4096:24576", "release:two", "release:display",
                  "destroy:24576"},
          "matching record inserts every entry then releases temporaries");

    Runtime invalid_end;
    invalid_end.PutWord(pal4::GameDbArchiveWriter::kPropertyStart);
    invalid_end.PutText("x");
    invalid_end.PutWord(0u);
    invalid_end.PutWord(99u);
    Check(!pal4::GameDbArchiveReader::ReadPropertyRecord(
              "outer", 0x1000u, stream, invalid_end) &&
              invalid_end.events[invalid_end.events.size() - 3u] ==
                  "release:" &&
              invalid_end.events[invalid_end.events.size() - 2u] ==
                  "release:x" &&
              invalid_end.events.back() == "destroy:24576",
          "bad end marker rejects insertion but cleans both text states");

    Runtime wrong_table;
    wrong_table.PutWord(17u);
    Check(!pal4::GameDbArchiveReader::ReadTable(
              "outer", 0x1000u, stream, wrong_table) &&
              wrong_table.events == std::vector<std::string>{
                  "begin:outer", "read:4"},
          "wrong table marker exits before object lookup");

    Runtime missing;
    missing.find_results = {0u, 0u};
    missing.PutWord(pal4::GameDbArchiveWriter::kObjectStart);
    missing.PutText("outer");
    Check(!pal4::GameDbArchiveReader::ReadTable(
              "outer", 0x1000u, stream, missing) &&
              std::find(missing.events.begin(), missing.events.end(),
                        "register:4096:outer") != missing.events.end() &&
              missing.events.back() == "release:outer",
          "missing table is registered and retried before failure");

    Runtime mismatch;
    mismatch.names_differ = true;
    mismatch.PutWord(pal4::GameDbArchiveWriter::kObjectStart);
    mismatch.PutText("other");
    Check(!pal4::GameDbArchiveReader::ReadTable(
              "outer", 0x1000u, stream, mismatch) &&
              std::find(mismatch.events.begin(), mismatch.events.end(),
                        "compare:other:outer") != mismatch.events.end() &&
              mismatch.events.back() == "release:other",
          "table name mismatch releases temporary name");

    Runtime table;
    table.PutWord(pal4::GameDbArchiveWriter::kObjectStart);
    table.PutText("outer");
    table.PutText("description");
    table.PutWord(9u);
    table.PutWord(3u);
    table.PutWord(1u);
    table.PutText("int");
    table.PutWord(1u);
    table.PutText("Integer");
    table.PutText("number");
    table.PutText("str");
    table.PutWord(2u);
    table.PutText("String");
    table.PutText("letters");
    table.PutText("flt");
    table.PutWord(3u);
    table.PutText("Float");
    table.PutText("decimal");
    table.PutWord(pal4::GameDbArchiveWriter::kObjectRowStart);
    table.PutWord(42u);
    table.PutText("hello");
    table.PutFloat(1.25f);
    table.PutWord(pal4::GameDbArchiveWriter::kObjectRowEnd);
    table.PutWord(pal4::GameDbArchiveWriter::kObjectEnd);
    Check(pal4::GameDbArchiveReader::ReadTable(
              "outer", 0x1000u, stream, table) &&
              table.cursor == table.input.size() &&
              std::find(table.events.begin(), table.events.end(),
                        "assign:28672:9:1") != table.events.end() &&
              std::find(table.events.begin(), table.events.end(),
                        "schema:int:1:Integer:number") != table.events.end() &&
              std::find(table.events.begin(), table.events.end(),
                        "schema:str:2:String:letters") != table.events.end() &&
              std::find(table.events.begin(), table.events.end(),
                        "schema:flt:3:Float:decimal") != table.events.end() &&
              std::find(table.events.begin(), table.events.end(),
                        "integer:42") != table.events.end() &&
              std::find(table.events.begin(), table.events.end(),
                        "text:hello") != table.events.end() &&
              std::find(table.events.begin(), table.events.end(),
                        "float:1.250000") != table.events.end() &&
              std::find(table.events.begin(), table.events.end(),
                        "commit-row:28672") != table.events.end() &&
              table.events[table.events.size() - 9u] == "end:outer",
          "table reader builds three typed fields and commits one row");

    Runtime bad_row;
    bad_row.PutWord(pal4::GameDbArchiveWriter::kObjectStart);
    bad_row.PutText("outer");
    bad_row.PutText("description");
    bad_row.PutWord(9u);
    bad_row.PutWord(0u);
    bad_row.PutWord(1u);
    bad_row.PutWord(99u);
    Check(!pal4::GameDbArchiveReader::ReadTable(
              "outer", 0x1000u, stream, bad_row) &&
              std::find(bad_row.events.begin(), bad_row.events.end(),
                        "commit-row:28672") == bad_row.events.end() &&
              bad_row.events.back() == "release:outer",
          "bad row marker cleans temporary state before row commit");

    const auto put_header = [](Runtime& target,
                               std::uint32_t fields,
                               std::uint32_t rows) {
        target.PutWord(pal4::GameDbArchiveWriter::kObjectStart);
        target.PutText("outer");
        target.PutText("description");
        target.PutWord(9u);
        target.PutWord(fields);
        target.PutWord(rows);
    };
    Runtime bad_type;
    put_header(bad_type, 1u, 1u);
    bad_type.PutText("unsupported");
    bad_type.PutWord(4u);
    bad_type.PutText("label");
    bad_type.PutText("detail");
    bad_type.PutWord(pal4::GameDbArchiveWriter::kObjectRowStart);
    Check(!pal4::GameDbArchiveReader::ReadTable(
              "outer", 0x1000u, stream, bad_type) &&
              std::find(bad_type.events.begin(), bad_type.events.end(),
                        "reset-value") != bad_type.events.end() &&
              std::find(bad_type.events.begin(), bad_type.events.end(),
                        "commit-value:0") == bad_type.events.end() &&
              bad_type.events.back() == "release:outer",
          "unknown field type rejects row after value reset");

    Runtime bad_row_end;
    put_header(bad_row_end, 0u, 1u);
    bad_row_end.PutWord(pal4::GameDbArchiveWriter::kObjectRowStart);
    bad_row_end.PutWord(77u);
    Check(!pal4::GameDbArchiveReader::ReadTable(
              "outer", 0x1000u, stream, bad_row_end) &&
              std::find(bad_row_end.events.begin(), bad_row_end.events.end(),
                        "commit-row:28672") == bad_row_end.events.end(),
          "wrong row end marker blocks row commit");

    Runtime bad_object_end;
    put_header(bad_object_end, 0u, 0u);
    bad_object_end.PutWord(77u);
    Check(!pal4::GameDbArchiveReader::ReadTable(
              "outer", 0x1000u, stream, bad_object_end) &&
              std::find(bad_object_end.events.begin(),
                        bad_object_end.events.end(),
                        "finalize:28672") == bad_object_end.events.end(),
          "wrong object end marker blocks finalization");

    pal4::GameDbSchemaArrayState schema;
    schema.words[2u] = 0x1234u;
    schema.words[3u] = 0x5678u;
    Check(pal4::GameDbSchemaArray::Count(schema) == 0u &&
              pal4::GameDbSchemaArray::Capacity(schema) == 0u,
          "null schema begin reports zero regardless of other words");
    schema.words[1u] = 0x8000u;
    schema.words[2u] = 0x8078u;
    schema.words[3u] = 0x81e0u;
    Check(pal4::GameDbSchemaArray::Count(schema) == 2u &&
              pal4::GameDbSchemaArray::Capacity(schema) == 8u,
          "schema count and capacity use 60-byte strides");

    SchemaRuntime copied;
    copied.values[0x5000u] = 99u;
    copied.values[0x7000u] = 1u;
    copied.values[0x703cu] = 2u;
    Check(pal4::GameDbSchemaArray::CopyElements(
              0x7000u, 0x7000u, 0x8000u, copied) == 0x8000u &&
              copied.events.empty(),
          "empty schema copy preserves destination cursor");
    Check(pal4::GameDbSchemaArray::CopyElements(
              0x7000u, 0x7078u, 0x8000u, copied) == 0x8078u &&
              copied.values[0x8000u] == 1u &&
              copied.values[0x803cu] == 2u,
          "schema copy constructs each 60-byte element");
    copied.events.clear();
    pal4::GameDbSchemaArray::ConstructRepeated(
        0x9000u, 0u, 0x5000u, copied);
    Check(copied.events.empty(), "zero repeated constructions do nothing");
    pal4::GameDbSchemaArray::ConstructRepeated(
        0x9000u, 2u, 0x5000u, copied);
    Check(copied.values[0x9000u] == 99u &&
              copied.values[0x903cu] == 99u &&
              copied.events == std::vector<std::string>{
                  "construct:36864:20480", "construct:36924:20480"},
          "repeated construction uses the same source and 60-byte stride");
    copied.events.clear();
    Check(pal4::GameDbSchemaArray::AssignElements(
              0x8000u, 0x8078u, 0x5000u, copied) == 0x8078u &&
              copied.values[0x8000u] == 99u &&
              copied.values[0x803cu] == 99u &&
              copied.events == std::vector<std::string>{
                  "assign:32768:20480", "assign:32828:20480"},
          "range assignment updates every existing element in order");
    copied.events.clear();
    Check(pal4::GameDbSchemaArray::AssignElements(
              0x8000u, 0x8000u, 0x5000u, copied) == 0x8000u &&
              copied.events.empty(),
          "empty range assignment returns its end");
    copied.values[0x7000u] = 1u;
    copied.values[0x703cu] = 2u;
    Check(pal4::GameDbSchemaArray::AssignPairwise(
              0x7000u, 0x7000u, 0x8000u, copied) == 0x8000u &&
              copied.events.empty(),
          "empty pairwise assignment preserves destination cursor");
    Check(pal4::GameDbSchemaArray::AssignPairwise(
              0x7000u, 0x7078u, 0x8000u, copied) == 0x8078u &&
              copied.values[0x8000u] == 1u &&
              copied.values[0x803cu] == 2u &&
              copied.events == std::vector<std::string>{
                  "assign:32768:28672", "assign:32828:28732"},
          "pairwise schema assignment follows source order");
    copied.events.clear();
    Check(pal4::GameDbSchemaArray::MoveElementsBackward(
              0x7000u, 0x7000u, 0x8000u, copied) == 0x8000u &&
              copied.events.empty(),
          "empty backward move preserves destination end");
    copied.values[0x7000u] = 1u;
    copied.values[0x703cu] = 2u;
    copied.values[0x7078u] = 3u;
    Check(pal4::GameDbSchemaArray::MoveElementsBackward(
              0x7000u, 0x7078u, 0x70b4u, copied) == 0x703cu &&
              copied.values[0x703cu] == 1u &&
              copied.values[0x7078u] == 2u &&
              copied.events == std::vector<std::string>{
                  "assign:28792:28732", "assign:28732:28672"},
          "overlapping backward move preserves source values and order");
    copied.events.clear();
    pal4::GameDbSchemaArray::FreeElements(
        0x7000u, 0x7078u, copied);
    Check(copied.events == std::vector<std::string>{
              "release:28672", "release:28732"},
          "schema cleanup visits the half-open range in order");

    SchemaElementRuntime element;
    for (const auto offset : {0u, 20u, 36u}) {
        element.bytes[0x5000u + offset] =
            static_cast<std::uint8_t>(offset + 1u);
        for (auto field = offset + 4u; field <= offset + 12u;
             field += 4u) {
            element.words[0x5000u + field] = field + 100u;
            element.words[0x6000u + field] = 999u;
        }
    }
    element.words[0x5010u] = 17u;
    element.words[0x5034u] = 52u;
    element.words[0x5038u] = 56u;
    pal4::GameDbSchemaElement::ConstructCopy(0u, 0x5000u, element);
    Check(element.events.empty(), "null schema destination skips construction");
    pal4::GameDbSchemaElement::ConstructCopy(0x6000u, 0x5000u, element);
    Check(element.events == std::vector<std::string>{
              "byte:24576", "word:24580:0", "word:24584:0",
              "word:24588:0", "text:24576", "word:24592:17",
              "byte:24596", "word:24600:0", "word:24604:0",
              "word:24608:0", "text:24596", "byte:24612",
              "word:24616:0", "word:24620:0", "word:24624:0",
              "text:24612", "word:24628:52", "word:24632:56"},
          "schema element constructor follows IDA field and text order");
    for (const auto offset : {0u, 20u, 36u}) {
        Check(element.bytes.at(0x6000u + offset) ==
                  element.bytes.at(0x5000u + offset),
              "schema flag byte copied");
        for (auto field = offset + 4u; field <= offset + 12u;
             field += 4u)
            Check(element.words.at(0x6000u + field) ==
                      element.words.at(0x5000u + field),
                  "schema text state copied");
    }
    element.require_empty_destination = false;
    element.events.clear();
    element.words[0x5010u] = 31u;
    element.words[0x5034u] = 71u;
    element.words[0x5038u] = 72u;
    Check(pal4::GameDbSchemaElement::AssignCopy(
              0x6000u, 0x5000u, element) == 0x6000u &&
              element.events == std::vector<std::string>{
                  "text:24576", "word:24592:31", "text:24596",
                  "text:24612", "word:24628:71", "word:24632:72"} &&
              element.words[0x6010u] == 31u &&
              element.words[0x6034u] == 71u &&
              element.words[0x6038u] == 72u,
          "schema assignment copies text then scalar fields in native order");
    element.events.clear();
    Check(pal4::GameDbSchemaElement::AssignCopy(
              0x6000u, 0x6000u, element) == 0x6000u &&
              element.events == std::vector<std::string>{
                  "text:24576", "word:24592:31", "text:24596",
                  "text:24612", "word:24628:71", "word:24632:72"},
          "schema self-assignment still dispatches each text field");

    SchemaOwnerRuntime owner_runtime;
    owner_runtime.values = {{0x5000u, 11u}, {0x503cu, 22u},
                            {0x5078u, 33u}, {0x7000u, 1u},
                            {0x703cu, 2u}};
    pal4::GameDbSchemaOwnerState owner;
    owner.schemas.words = {0u, 0x7000u, 0x7078u, 0x70b4u};
    owner.pointers = {0u, 0x8000u, 0x8000u, 0x8008u};
    Check(pal4::GameDbSchemaOwner::AssignFrom(
              owner, owner, owner_runtime) &&
              owner.schemas.words[2u] == 0x7078u &&
              owner.pointers[2u] == 0x8008u &&
              owner_runtime.events == std::vector<std::string>{
                  "append-null", "append-null"},
          "schema owner self-assignment skips schema copy but appends slots");

    pal4::GameDbSchemaOwnerState one_source;
    one_source.schemas.words = {0u, 0x5000u, 0x503cu, 0x503cu};
    owner_runtime.events.clear();
    owner.pointers[2u] = 0x8000u;
    Check(pal4::GameDbSchemaOwner::AssignFrom(
              owner, one_source, owner_runtime) &&
              owner.schemas.words[2u] == 0x703cu &&
              owner_runtime.values[0x7000u] == 11u &&
              owner_runtime.events == std::vector<std::string>{
                  "assign:28672:20480", "release:28732", "append-null"},
          "shorter schema assignment releases unused destination elements");

    pal4::GameDbSchemaOwnerState three_source;
    three_source.schemas.words = {0u, 0x5000u, 0x50b4u, 0x50b4u};
    owner_runtime.events.clear();
    owner.pointers = {0u, 0x8000u, 0x8000u, 0x8004u};
    Check(pal4::GameDbSchemaOwner::AssignFrom(
              owner, three_source, owner_runtime) &&
              owner.schemas.words[2u] == 0x70b4u &&
              owner_runtime.values[0x7000u] == 11u &&
              owner_runtime.values[0x703cu] == 22u &&
              owner_runtime.values[0x7078u] == 33u &&
              owner.pointers[1u] == 0xa000u &&
              owner.pointers[2u] == 0xa00cu &&
              owner_runtime.events == std::vector<std::string>{
                  "assign:28672:20480", "construct:28732:20540",
                  "construct:28792:20600", "allocate:12",
                  "copy-pointers:32768:32768", "free:32768",
                  "append-null", "append-null", "append-null"},
          "schema growth within capacity constructs tail and grows pointer slots");

    SchemaOwnerRuntime reallocated;
    reallocated.values = owner_runtime.values;
    pal4::GameDbSchemaOwnerState full_owner;
    full_owner.schemas.words = {0u, 0x7000u, 0x703cu, 0x703cu};
    Check(pal4::GameDbSchemaOwner::AssignFrom(
              full_owner, three_source, reallocated) &&
              full_owner.schemas.words[1u] == 0xa000u &&
              full_owner.schemas.words[2u] == 0xa0b4u &&
              full_owner.schemas.words[3u] == 0xa0b4u &&
              full_owner.pointers[1u] == 0xb000u &&
              reallocated.values[0xa000u] == 11u &&
              reallocated.values[0xa03cu] == 22u &&
              reallocated.values[0xa078u] == 33u &&
              reallocated.events.front() == "release:28672" &&
              reallocated.events[1u] == "free:28672" &&
              reallocated.events[2u] == "allocate:180",
          "full schema assignment reallocates exact source count first");

    SchemaBindingRuntime binding;
    binding.pointers = {{0x8000u, 0u}, {0x8004u, 0xa000u},
                        {0x8008u, 0u}};
    binding.names = {{0x5000u, "first"}, {0x503cu, "second"},
                     {0x5078u, "third"}};
    pal4::GameDbSchemaOwnerState binding_owner;
    binding_owner.schemas.words = {0u, 0x5000u, 0x50b4u, 0x50b4u};
    binding_owner.pointers = {0u, 0x8000u, 0x800cu, 0x800cu};
    std::uint32_t temporary_offset = 0u;
    pal4::GameDbSchemaOwner::BindMissing(
        binding_owner, temporary_offset, 0x1234u, binding);
    Check(binding.pointers[0x8000u] == 0x9000u &&
              binding.pointers[0x8004u] == 0xa000u &&
              binding.pointers[0x8008u] == 0u &&
              temporary_offset == 1u && binding.callback_count == 2u &&
              binding.events == std::vector<std::string>{
                  "read:32768", "allocate:20", "base-vtable",
                  "initialize-text:36868", "derived-vtable",
                  "write:32768", "name:20480",
                  "message:find unbinded field:first", "callback:4660",
                  "read:32772", "read:32776", "allocate:20",
                  "write:32776", "name:20600",
                  "message:find unbinded field:third", "callback:4660"},
          "binding reports only missing fields and handles allocation failure");
    binding.events.clear();
    binding.pointers[0x8008u] = 0xb000u;
    pal4::GameDbSchemaOwner::BindMissing(
        binding_owner, temporary_offset, 0x1234u, binding);
    Check(binding.events == std::vector<std::string>{
              "read:32768", "read:32772", "read:32776"} &&
              temporary_offset == 1u,
          "already bound fields require no new effects or callback");
    binding.events.clear();
    pal4::GameDbSchemaOwnerState no_schema;
    pal4::GameDbSchemaOwner::BindMissing(
        no_schema, temporary_offset, 0x1234u, binding);
    Check(binding.events.empty(), "null schema array skips binding loop");

    PlaceholderRuntime placeholder_runtime;
    pal4::GameDbPlaceholderEffectState placeholder{
        0x84b4b0u, 1u, 0xb000u, 5u, 8u};
    temporary_offset = 2u;
    pal4::GameDbPlaceholderEffect::Cleanup(
        placeholder, temporary_offset, placeholder_runtime);
    Check(placeholder.vtable == 0x849898u &&
              placeholder.flag == 1u &&
              placeholder.data == 0u && placeholder.length == 0u &&
              placeholder.capacity == 0u && temporary_offset == 1u &&
              placeholder_runtime.events ==
                  std::vector<std::string>{"release:45056"},
          "placeholder cleanup releases text and restores base vtable");
    placeholder_runtime.events.clear();
    placeholder.vtable = 0x84b4b0u;
    Check(pal4::GameDbPlaceholderEffect::Delete(
              0x9000u, placeholder, 0u, temporary_offset,
              placeholder_runtime) == 0x9000u &&
              temporary_offset == 0u &&
              placeholder_runtime.events.empty(),
          "nondeleting placeholder destructor does not free storage");
    placeholder_runtime.events.clear();
    placeholder.data = 0xb100u;
    temporary_offset = 1u;
    Check(pal4::GameDbPlaceholderEffect::Delete(
              0x9000u, placeholder, 3u, temporary_offset,
              placeholder_runtime) == 0x9000u &&
              temporary_offset == 0u &&
              placeholder_runtime.events == std::vector<std::string>{
                  "release:45312", "free:36864"},
          "deleting placeholder destructor frees after text cleanup");

    SchemaProcessRuntime process_runtime;
    process_runtime.pointers = {{0x8000u, 0x9000u},
                                {0x8004u, 0u},
                                {0x8008u, 0xa000u}};
    Check(pal4::GameDbSchemaOwner::ProcessBindings(
              binding_owner, process_runtime) &&
              process_runtime.events == std::vector<std::string>{
                  "read:32768", "invoke:36864:1", "read:32772",
                  "read:32776", "invoke:40960:1"},
          "binding processor invokes only live objects in array order");
    process_runtime.events.clear();
    Check(pal4::GameDbSchemaOwner::ProcessBindings(
              no_schema, process_runtime) &&
              process_runtime.events.empty(),
          "empty pointer array processes no objects");

    BoundEffectRuntime bound_effects;
    temporary_offset = 0u;
    Check(pal4::GameDbBoundEffectFactory::CreateKindOne(
              0x7000u, 0x5001u, 17u,
              temporary_offset, bound_effects) &&
              temporary_offset == 1u &&
              bound_effects.objects[0x9000u].vtable == 0x84b4c0u &&
              bound_effects.objects[0x9000u].value == 17u &&
              bound_effects.objects[0x9000u].text_flag == 1u &&
              bound_effects.objects[0x9000u].text_data == 0x5005u &&
              bound_effects.events == std::vector<std::string>{
                  "allocate:24", "assign-text:20481",
                  "bind:28672:20481:36864:1"},
          "kind-one effect creates 24-byte object and binds with type one");
    bound_effects.events.clear();
    bound_effects.bind_result = false;
    Check(!pal4::GameDbBoundEffectFactory::CreateKindTwo(
              0x7000u, 0x6000u, 23u,
              temporary_offset, bound_effects) &&
              temporary_offset == 2u &&
              bound_effects.objects[0x9100u].vtable == 0x84b4d0u &&
              bound_effects.objects[0x9100u].value == 23u &&
              bound_effects.events == std::vector<std::string>{
                  "allocate:24", "assign-text:24576",
                  "bind:28672:24576:37120:2"},
          "kind-two effect uses its vtable/type and returns binding failure");
    bound_effects.events.clear();
    bound_effects.bind_result = true;
    Check(pal4::GameDbBoundEffectFactory::CreateKindOne(
              0x7000u, 0x5001u, 17u,
              temporary_offset, bound_effects) &&
              temporary_offset == 2u &&
              bound_effects.events == std::vector<std::string>{
                  "allocate:24", "bind:28672:20481:0:1"},
          "allocation failure still forwards null object to binder");

    SchemaBindRuntime binder;
    binder.schemas = {{0x5000u, "first"}, {0x503cu, "second"},
                      {0x5078u, "third"}};
    binder.kinds = {{0x5000u, 1u}, {0x503cu, 2u},
                    {0x5078u, 1u}};
    binder.sources = {{0x6000u, "second"},
                      {0x6100u, "missing"},
                      {0x6200u, "first"}};
    binder.pointers = {{0x8000u, 0u}, {0x8004u, 0x9000u},
                       {0x8008u, 0u}};
    Check(pal4::GameDbSchemaOwner::BindFromObject(
              binding_owner, 0xa000u, binder) &&
              binder.pointers[0x8004u] == 0xa000u &&
              binder.events == std::vector<std::string>{
                  "virtual-kind:40960", "virtual-source:40960",
                  "source-name:24576", "schema-name:20480",
                  "schema-name:20540", "schema-kind:20540",
                  "read:32772", "delete:36864:1", "source-name:24576",
                  "message:find bind a field which is already binded!! field:second",
                  "callback:4660",
                  "write:32772:40960"},
          "virtual wrapper replaces a matching binding after deleting old one");
    binder.events.clear();
    Check(!pal4::GameDbSchemaOwner::BindEffect(
              binding_owner, 0x6200u, 0xa100u, 2u, binder) &&
              binder.events == std::vector<std::string>{
                  "source-name:25088", "schema-name:20480",
                  "schema-kind:20480", "delete:41216:1",
                  "source-name:25088",
                  "message:binding field requested not match the dbField's type !! field:first",
                  "callback:4660"},
          "type mismatch deletes proposed effect and reports rejection");
    binder.events.clear();
    Check(!pal4::GameDbSchemaOwner::BindEffect(
              binding_owner, 0x6100u, 0xa200u, 1u, binder) &&
              binder.events.back() == "callback:4660" &&
              binder.events[binder.events.size() - 2u] ==
                  "message:binding field requested not find!! field:missing" &&
              std::find(binder.events.begin(), binder.events.end(),
                        "delete:41472:1") != binder.events.end() &&
              binder.pointers[0x8000u] == 0u,
          "missing schema name deletes candidate without changing slots");
    binder.events.clear();
    Check(pal4::GameDbSchemaOwner::BindEffect(
              binding_owner, 0x6200u, 0xa300u, 1u, binder) &&
              binder.pointers[0x8000u] == 0xa300u &&
              std::find(binder.events.begin(), binder.events.end(),
                        "message:find bind a field which is already binded!! field:first") ==
                  binder.events.end(),
          "first binding writes empty slot without rebind report");
    binder.events.clear();
    binder.callback_slot = 0u;
    pal4::GameDbBindingDiagnostics::ReportMissing(0x6200u, binder);
    Check(binder.events == std::vector<std::string>{
              "source-name:25088",
              "message:binding field requested not find!! field:first"},
          "diagnostic still updates message when callback is unset");

    BoundPropertyRuntime property_values;
    pal4::GameDbBoundEffectState bound_object;
    bound_object.vtable = 0x84b4c0u;
    bound_object.value = 12u;
    bound_object.text_data = 0xb000u;
    bound_object.text_length = 4u;
    bound_object.text_capacity = 8u;
    const pal4::GameDbTypedValueState int_value{1u, 37u};
    Check(pal4::GameDbBoundEffect::ApplyInteger(
              bound_object, 0x7000u, int_value,
              property_values) == 37 &&
              property_values.words[0x700cu] == 37u &&
              property_values.events == std::vector<std::string>{
                  "write:28684:37"},
          "kind-one property writes integer at configured record offset");
    property_values.events.clear();
    pal4::GameDbTypedValueErrorState fallback_text;
    const pal4::GameDbTypedValueState string_value{2u, 0x9000u};
    Check(pal4::GameDbBoundEffect::ApplyString(
              bound_object, 0x7000u, string_value,
              fallback_text, 0u, property_values) == 7u &&
              property_values.events == std::vector<std::string>{
                  "assign-text:28684:36864"},
          "string property uses tagged source object and record offset");
    property_values.events.clear();
    Check(pal4::GameDbBoundEffect::ApplyString(
              bound_object, 0x7000u, int_value,
              fallback_text, 0u, property_values) == 7u &&
              fallback_text.initialized &&
              property_values.events.back() ==
                  "assign-text:28684:9686732",
          "mismatched string property composes static error text fallback");
    property_values.events.clear();
    temporary_offset = 2u;
    pal4::GameDbBoundEffect::CleanupKindOne(
        bound_object, temporary_offset, property_values);
    Check(bound_object.vtable == 0x849898u &&
              bound_object.text_data == 0u &&
              bound_object.text_length == 0u &&
              bound_object.text_capacity == 0u &&
              temporary_offset == 1u &&
              property_values.events ==
                  std::vector<std::string>{"release:45056"},
          "kind-one destructor releases text and decrements offset");
    property_values.events.clear();
    bound_object.vtable = 0x84b4d0u;
    pal4::GameDbBoundEffect::CleanupKindTwo(
        bound_object, temporary_offset, property_values);
    Check(bound_object.vtable == 0x849898u &&
              temporary_offset == 0u && property_values.events.empty(),
          "kind-two destructor handles empty text");

    SchemaRuntime grown;
    grown.values[0x5000u] = 99u;
    pal4::GameDbSchemaArrayState empty_schema;
    Check(pal4::GameDbSchemaArray::InsertCopy(
              empty_schema, 0u, 0x5000u, grown) == 0xa000u &&
              empty_schema.words[1u] == 0xa000u &&
              empty_schema.words[2u] == 0xa03cu &&
              empty_schema.words[3u] == 0xa03cu &&
              grown.values[0xa000u] == 99u &&
              grown.events == std::vector<std::string>{
                  "allocate:60", "construct:40960:20480", "free:0"},
          "empty schema insertion allocates one slot and returns its address");

    SchemaRuntime spare;
    spare.values[0x5000u] = 99u;
    spare.values[0x7000u] = 1u;
    spare.values[0x703cu] = 2u;
    pal4::GameDbSchemaArrayState in_place;
    in_place.words = {0u, 0x7000u, 0x7078u, 0x70b4u};
    Check(pal4::GameDbSchemaArray::InsertCopy(
              in_place, 0x7000u, 0x5000u, spare) == 0x7000u &&
              in_place.words[2u] == 0x70b4u &&
              spare.values[0x7000u] == 99u &&
              spare.values[0x703cu] == 1u &&
              spare.values[0x7078u] == 2u &&
              spare.events == std::vector<std::string>{
                  "construct:28792:28732", "assign:28732:28672",
                  "assign:28672:20480"},
          "in-place head insertion shifts tail backward before assignment");
    spare.events.clear();
    in_place.words = {0u, 0x7000u, 0x7078u, 0x70b4u};
    Check(pal4::GameDbSchemaArray::InsertCopy(
              in_place, 0x7078u, 0x5000u, spare) == 0x7078u &&
              spare.events == std::vector<std::string>{
                  "construct:28792:20480"},
          "in-place tail insertion constructs only the appended element");

    SchemaRuntime full;
    full.values[0x5000u] = 99u;
    full.values[0x7000u] = 1u;
    full.values[0x703cu] = 2u;
    pal4::GameDbSchemaArrayState replacement;
    replacement.words = {0u, 0x7000u, 0x7078u, 0x7078u};
    Check(pal4::GameDbSchemaArray::InsertCopy(
              replacement, 0x703cu, 0x5000u, full) == 0xa03cu &&
              replacement.words[1u] == 0xa000u &&
              replacement.words[2u] == 0xa0b4u &&
              replacement.words[3u] == 0xa0f0u &&
              full.values[0xa000u] == 1u &&
              full.values[0xa03cu] == 99u &&
              full.values[0xa078u] == 2u &&
              full.events == std::vector<std::string>{
                  "allocate:240", "construct:40960:28672",
                  "construct:41020:20480", "construct:41080:28732",
                  "release:28672", "release:28732", "free:28672"},
          "full schema insertion doubles capacity and preserves element order");

    FactoryRuntime factory;
    Check(pal4::GameDbObjectFactory::Create("outer", factory) == 0x7004u &&
              factory.events == std::vector<std::string>{
                  "allocate:92", "construct:28672:outer"},
          "GameDB factory constructs allocated named objects");
    factory.events.clear();
    factory.allocation = 0u;
    Check(pal4::GameDbObjectFactory::Create("outer", factory) == 0u &&
              factory.events == std::vector<std::string>{"allocate:92"},
          "GameDB factory skips constructor on allocation failure");

    PropertyRuntime property_runtime;
    pal4::GameDbPropertyState property;
    property.words[0u] = 0xaabbcc00u;
    property.words[4u] = 0xddeeff00u;
    property.words[8u] = 0x11223300u;
    constexpr char property_name[] = "outer";
    const auto low_byte = static_cast<std::uint8_t>(
        reinterpret_cast<std::uintptr_t>(property_name));
    Check(&pal4::GameDbProperty::Construct(
              property, property_name, property_runtime) == &property &&
              property.words[0u] == (0xaabbcc00u | low_byte) &&
              property.words[4u] == (0xddeeff00u | low_byte) &&
              property.words[8u] == (0x11223300u | low_byte) &&
              property.words[1u] == 0x7000u &&
              property.words[2u] == 0x7000u &&
              property.words[3u] == 0x70c0u &&
              property.words[5u] == 0x8000u &&
              property.words[6u] == 5u && property.words[7u] == 5u &&
              property.words[9u] == 0u &&
              property_runtime.events == std::vector<std::string>{
                  "allocate-text:5:1", "copy-text:32768:outer",
                  "allocate:192", "free:0"},
          "property constructor initializes text and reserves eight records");
    property_runtime.events.clear();
    property.words[2u] = 0x7030u;
    pal4::GameDbProperty::ClearRecords(property, property_runtime);
    Check(property.words[2u] == 0x7000u &&
              property_runtime.events == std::vector<std::string>{
                  "cleanup:28672", "cleanup:28696"},
          "property clear visits live records in order and retains capacity");
    property_runtime.events.clear();
    property.words[2u] = 0x7018u;
    property.words[9u] = 0x9000u;
    Check(pal4::GameDbProperty::Destroy(property, property_runtime) == 7 &&
              property.words[1u] == 0u && property.words[2u] == 0u &&
              property.words[3u] == 0u && property.words[5u] == 0u &&
              property.words[9u] == 0u &&
              property_runtime.events == std::vector<std::string>{
                  "cleanup:28672", "release:36864", "release:32768",
                  "free:28672"},
          "property destructor clears records, text, and allocation in order");

    PropertyRuntime no_text;
    no_text.text_allocation = 0u;
    pal4::GameDbPropertyState empty_property;
    (void)pal4::GameDbProperty::Construct(
        empty_property, "", no_text);
    Check(empty_property.words[5u] == 0u &&
              empty_property.words[6u] == 0u &&
              empty_property.words[7u] == 0u &&
              no_text.events == std::vector<std::string>{
                  "allocate-text:0:1", "allocate:192", "free:0"},
          "property constructor tolerates failed or empty text allocation");

    PropertyRuntime entries;
    pal4::GameDbPropertyState entry_property;
    entry_property.words[1u] = 0x7000u;
    entry_property.words[2u] = 0x7048u;
    entry_property.words[3u] = 0x70c0u;
    entries.record_ids = {{0x7000u, 10}, {0x7018u, 30},
                          {0x7030u, 50}};
    entries.record_texts = {{0x7000u, "first"},
                            {0x7018u, "duplicate-name"},
                            {0x7030u, "last"}};
    Check(!pal4::GameDbProperty::AddEntry(
              entry_property, -1, "bad", entries) &&
              entries.events.empty(),
          "negative property entry ID exits before searching");
    Check(!pal4::GameDbProperty::AddEntry(
              entry_property, 30, "duplicate-id", entries) &&
              std::find(entries.events.begin(), entries.events.end(),
                        "read-text:28672") == entries.events.end(),
          "duplicate numeric ID rejects before name lookup");
    entries.events.clear();
    Check(!pal4::GameDbProperty::AddEntry(
              entry_property, 20, "duplicate-name", entries) &&
              entries.events.back() == "read-id:28696",
          "duplicate name rejects before allocating temporary text");
    entries.events.clear();
    Check(pal4::GameDbProperty::AddEntry(
              entry_property, 20, "middle", entries) &&
              entry_property.words[2u] == 0x7060u &&
              entries.events == std::vector<std::string>{
                  "read-id:28696", "read-id:28672",
                  "read-id:28696", "read-text:28672",
                  "read-text:28696", "read-text:28720",
                  "allocate-text:6:1", "copy-text:32768:middle",
                  "insert:28696:20:32768:6", "release:32768"},
          "entry insertion finds sorted middle and releases temporary text");
    entries.events.clear();
    entry_property.words[2u] = 0x7048u;
    Check(pal4::GameDbProperty::FindEntryByName(
              entry_property, "first", entries) == 10 &&
              pal4::GameDbProperty::FindEntryByName(
                  entry_property, "missing", entries) == -1,
          "name search returns matching ID or minus one");
    entries.events.clear();
    Check(pal4::GameDbProperty::FindEntryById(
              entry_property, 30, entries) == 0x7018u &&
              pal4::GameDbProperty::FindEntryById(
                  entry_property, 20, entries) == 0u &&
              pal4::GameDbProperty::FindEntryTextById(
                  entry_property, 50, entries) == 0x7034u &&
              pal4::GameDbProperty::FindEntryTextById(
                  entry_property, 51, entries) == 0u,
          "ID search uses lower bound and text wrapper adds four bytes");
    Check(pal4::GameDbProperty::EntryAt(entry_property, 0u) == 0x7000u &&
              pal4::GameDbProperty::EntryAt(entry_property, 2u) == 0x7030u &&
              pal4::GameDbProperty::EntryAt(entry_property, 3u) == 0u,
          "index getter accepts only live record positions");
    pal4::GameDbPropertyState null_entries;
    Check(pal4::GameDbProperty::EntryAt(null_entries, 0u) == 0u &&
              pal4::GameDbProperty::FindEntryByName(
                  null_entries, "anything", entries) == -1,
          "empty property lookup returns no entry");

    PropertyRuntime shifted;
    shifted.record_ids = {{0x5000u, 99}, {0x6000u, 10},
                          {0x6018u, 20}, {0x6030u, 30}};
    pal4::GameDbPropertyState shift_state;
    shift_state.words[1u] = 0x6000u;
    shift_state.words[2u] = 0x6048u;
    shift_state.words[3u] = 0x6090u;
    Check(pal4::GameDbProperty::InsertCopies(
              shift_state, 0x6018u, 0u, 0x5000u, shifted) == 0x6018u &&
              shift_state.words[2u] == 0x6048u &&
              shifted.events.empty(),
          "zero insertion leaves existing array untouched");
    Check(pal4::GameDbProperty::InsertCopies(
              shift_state, 0x6000u, 1u, 0x5000u, shifted) == 0x6000u &&
              shift_state.words[2u] == 0x6060u &&
              shifted.record_ids[0x6000u] == 99 &&
              shifted.record_ids[0x6018u] == 10 &&
              shifted.record_ids[0x6030u] == 20 &&
              shifted.record_ids[0x6048u] == 30 &&
              shifted.events == std::vector<std::string>{
                  "construct:24648:24624", "assign:24624:24600",
                  "assign:24600:24576", "assign:24576:20480"},
          "spare-capacity head insertion shifts overlapping records backward");

    PropertyRuntime short_tail;
    short_tail.record_ids = {{0x5000u, 99}, {0x6000u, 10},
                             {0x6018u, 20}, {0x6030u, 30}};
    pal4::GameDbPropertyState short_state;
    short_state.words[1u] = 0x6000u;
    short_state.words[2u] = 0x6048u;
    short_state.words[3u] = 0x60a8u;
    Check(pal4::GameDbProperty::InsertCopies(
              short_state, 0x6030u, 3u, 0x5000u, short_tail) == 0x6030u &&
              short_state.words[2u] == 0x6090u &&
              short_tail.record_ids[0x6030u] == 99 &&
              short_tail.record_ids[0x6048u] == 99 &&
              short_tail.record_ids[0x6060u] == 99 &&
              short_tail.record_ids[0x6078u] == 30 &&
              short_tail.events == std::vector<std::string>{
                  "construct:24696:24624", "construct:24648:20480",
                  "construct:24672:20480", "assign:24624:20480"},
          "short-tail insertion constructs displaced and extra slots");

    PropertyRuntime grown_property;
    grown_property.record_ids = {{0x5000u, 99}, {0x6000u, 10},
                                 {0x6018u, 20}};
    pal4::GameDbPropertyState full_property;
    full_property.words[1u] = 0x6000u;
    full_property.words[2u] = full_property.words[3u] = 0x6030u;
    Check(pal4::GameDbProperty::InsertCopies(
              full_property, 0x6018u, 1u, 0x5000u,
              grown_property) == 0x7018u &&
              full_property.words[1u] == 0x7000u &&
              full_property.words[2u] == 0x7048u &&
              full_property.words[3u] == 0x7060u &&
              grown_property.record_ids[0x7000u] == 10 &&
              grown_property.record_ids[0x7018u] == 99 &&
              grown_property.record_ids[0x7030u] == 20 &&
              grown_property.events == std::vector<std::string>{
                  "allocate:96", "construct:28672:24576",
                  "construct:28696:20480", "construct:28720:24600",
                  "cleanup:24576", "cleanup:24600", "free:24576"},
          "full array doubles capacity and preserves prefix and suffix");
    grown_property.events.clear();
    Check(pal4::GameDbProperty::DeleteRecord(
              0x7000u, 0u, grown_property) == 0x7000u &&
              grown_property.events == std::vector<std::string>{
                  "cleanup:28672"},
          "record delete without bit zero only cleans its fields");
    grown_property.events.clear();
    Check(pal4::GameDbProperty::DeleteRecord(
              0x7000u, 3u, grown_property) == 0x7000u &&
              grown_property.events == std::vector<std::string>{
                  "cleanup:28672", "free:28672"},
          "record delete with bit zero frees after cleanup");
    std::cout << "GameDB archive reader tests passed\n";
}
