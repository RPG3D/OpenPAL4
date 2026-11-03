#include "pal4/monster_storage_object.h"

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

struct TreeRuntimeProbe final : pal4::IScriptAttributeTreeRuntime {
    explicit TreeRuntimeProbe(
        std::unordered_map<std::uint32_t, std::uint32_t>& words)
        : memory(words) {}
    std::unordered_map<std::uint32_t, std::uint32_t>& memory;
    bool throw_on_read{};
    std::uint32_t NilAddress() override { return 0u; }
    std::uint32_t EmptyStringAddress() override { return 0u; }
    std::uint32_t ReadDword(std::uint32_t address) override {
        if (throw_on_read)
            throw std::runtime_error("tree read");
        return memory[address];
    }
    std::int32_t CompareNodeKey(std::uint32_t node,
                                std::uint32_t query) override {
        Check(node == 0x810cu && query == 0x9000u,
              "attribute node and query addresses");
        return 0;
    }
    std::int32_t CompareBytes(std::uint32_t query,
                              std::uint32_t node,
                              std::uint32_t length) override {
        Check(query == 0xa100u && node == 0xa000u && length == 20u,
              "attribute key comparison arguments");
        return 0;
    }
};

struct StringRuntimeProbe final : pal4::IStringAssignRuntime {
    explicit StringRuntimeProbe(std::vector<std::string>& log) : calls(log) {}
    std::vector<std::string>& calls;
    void ThrowRange() override { throw std::out_of_range("range"); }
    void ThrowLength() override {
        calls.push_back("throw-length");
        throw std::length_error("length");
    }
    void DetachAndReset(pal4::StringBufferState& state) override {
        state = {};
    }
    bool AllocateText(pal4::StringBufferState&,
                      std::uint32_t, std::uint8_t) override {
        Check(false, "StringBuffer composes its own allocation logic");
        return false;
    }
    void SetTextSize(pal4::StringBufferState& state,
                     std::uint32_t length) override {
        calls.push_back("set-size:" + std::to_string(length));
        state.length = length;
    }
    void SetErrorFlag(pal4::StringBufferState& state,
                      std::uint8_t) override { state = {}; }
    void Reallocate(pal4::StringBufferState& state,
                    std::uint32_t length) override {
        calls.push_back("reallocate:" + std::to_string(length));
        state.data = 0xd001u;
        state.capacity = length | 0x1fu;
    }
    std::uint8_t ReadByte(std::uint32_t) override { return 0u; }
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    void Copy(std::uint32_t destination, std::uint32_t source,
              std::uint32_t length) override {
        Check(destination == 0xd001u && source == 0xc000u,
              "name copy source and allocation");
        calls.push_back("copy:" + std::to_string(length));
    }
    void Free(std::uint32_t) override {}
};

struct Runtime final : pal4::IMonsterStorageRuntime {
    std::vector<std::string> calls;
    std::unordered_map<std::uint32_t, std::uint32_t> memory;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    TreeRuntimeProbe tree{memory};
    StringRuntimeProbe string{calls};
    pal4::MonsterStorageRecordState record{
        0xaaaaaaaau, 0x12345678u, 0x9999u, 0x9999u, 0x9999u, 0x7777u};
    std::int32_t parsed{1};
    std::uint32_t allocation{0xb000u};
    std::uint32_t name_length{4u};
    bool throw_on_byte{};

    Runtime() {
        memory[0x604cu] = 0x7000u;
        memory[0x7018u] = 0x8000u;
        memory[0x8004u] = 0x8100u;
        memory[0x8100u] = 0u;
        memory[0x8110u] = 0xa000u;
        memory[0x8114u] = 20u;
        memory[0x9004u] = 0xa100u;
        memory[0x9008u] = 20u;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return memory[address];
    }
    std::uint32_t CreateQuery(const char* name) override {
        Check(std::string(name) == "PAL4_MST_NEED_STORAGE",
              "storage requirement query name");
        calls.push_back("query-create");
        return 0x9000u;
    }
    void DestroyQuery(std::uint32_t address) override {
        Check(address == 0x9000u, "temporary query address");
        calls.push_back("query-destroy");
    }
    pal4::IScriptAttributeTreeRuntime& TreeRuntime() override {
        return tree;
    }
    std::int32_t ParseInt(std::uint32_t address,
                          std::int32_t fallback) override {
        Check(address == 0x811cu && fallback == 0,
              "requirement attribute and default");
        calls.push_back("parse-int");
        return parsed;
    }
    std::uint32_t AllocateObject(std::uint32_t bytes) override {
        Check(bytes == 24u, "storage record allocation size");
        calls.push_back("allocate");
        return allocation;
    }
    void FreeObject(std::uint32_t address) override {
        Check(address == 0xb000u, "exception cleanup address");
        calls.push_back("free-object");
    }
    pal4::MonsterStorageRecordState& RecordAt(
        std::uint32_t address) override {
        Check(address == 0xb000u, "record address");
        calls.push_back("record-at");
        return record;
    }
    std::uint8_t IncomingByte() override {
        calls.push_back("incoming-byte");
        if (throw_on_byte)
            throw std::runtime_error("constructor byte");
        return 0x44u;
    }
    std::uint32_t CStringLength(std::uint32_t address) override {
        Check(address == 0xc000u, "manager name pointer");
        calls.push_back("name-length");
        return name_length;
    }
    pal4::IStringAssignRuntime& StringRuntime() override {
        return string;
    }
    [[noreturn]] void NullObjectDereference() override {
        calls.push_back("null-dereference");
        throw std::runtime_error("null object");
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("read-byte:" + std::to_string(address));
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.push_back("write-byte:" + std::to_string(address));
        bytes[address] = value;
    }
    std::uint32_t FreeStringStorage(std::uint32_t address) override {
        calls.push_back("free-string:" + std::to_string(address));
        return 0xdeadu;
    }
};

pal4::MonsterBehaviorManagerState Manager(bool active = true) {
    pal4::MonsterBehaviorManagerState state{};
    state.words[0x654u / 4u] = active ? 0x6000u : 0u;
    state.words[1] = 0xc000u;
    state.words[0x1f0u / 4u] = 0x1234u;
    return state;
}

void TestInitializeRecordPreservesUnwrittenBytes() {
    Runtime runtime;
    Check(pal4::MonsterStorageObject::InitializeRecord(
              runtime.record, 0xb000u, 0x44u) == 0xb000u &&
              runtime.record.vtable == 0x842d68u &&
              runtime.record.flags == 0x12345644u &&
              runtime.record.text_data == 0u &&
              runtime.record.text_length == 0u &&
              runtime.record.text_capacity == 0u &&
              runtime.record.property == 0x7777u,
          "24-byte constructor only writes observed fields");
}

void TestAllocateAndInitialize() {
    Runtime runtime;
    Check(pal4::MonsterStorageObject::AllocateAndInitialize(runtime) ==
              0xb000u &&
              runtime.calls == std::vector<std::string>{
                  "allocate", "record-at", "incoming-byte"} &&
              runtime.record.vtable == 0x842d68u,
          "24-byte factory allocates and invokes record constructor");
    Runtime null_runtime;
    null_runtime.allocation = 0u;
    Check(pal4::MonsterStorageObject::AllocateAndInitialize(null_runtime) ==
              0u &&
              null_runtime.calls == std::vector<std::string>{"allocate"},
          "factory returns zero on null allocation");
}

void TestMissingSourceAndMissingAttribute() {
    Runtime runtime;
    Check(pal4::MonsterStorageObject::Create(
              Manager(false), 0x5000u, runtime) == 0u &&
              runtime.calls.empty(),
          "missing manager source exits before memory reads");
    runtime.memory[0x8004u] = 0u;
    Check(pal4::MonsterStorageObject::Create(
              Manager(), 0x5000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "read:24652", "query-create", "query-destroy",
                  "read:28696"},
          "missing attribute releases query without allocating object");
}

void TestNonpositiveAttribute() {
    for (auto parsed : {0, -2}) {
        Runtime runtime;
        runtime.parsed = parsed;
        Check(pal4::MonsterStorageObject::Create(
                  Manager(), 0x5000u, runtime) == 0u &&
                  runtime.calls.back() == "parse-int" &&
                  runtime.record.vtable == 0xaaaaaaaau,
              "nonpositive storage requirement skips allocation");
    }
}

void TestCreateWithName() {
    Runtime runtime;
    Check(pal4::MonsterStorageObject::Create(
              Manager(), 0x5000u, runtime) == 0xb000u &&
              runtime.calls == std::vector<std::string>{
                  "read:24652", "query-create", "query-destroy",
                  "read:28696", "parse-int", "allocate", "record-at",
                  "incoming-byte", "record-at", "name-length", "reallocate:4",
                  "copy:4", "set-size:4"} &&
              runtime.record.vtable == 0x842d68u &&
              runtime.record.flags == 0x12345644u &&
              runtime.record.text_data == 0xd001u &&
              runtime.record.text_length == 4u &&
              runtime.record.text_capacity == 31u &&
              runtime.record.property == 0x1234u,
          "positive storage requirement constructs record and copies name");
}

void TestEmptyName() {
    Runtime runtime;
    runtime.name_length = 0u;
    Check(pal4::MonsterStorageObject::Create(
              Manager(), 0x5000u, runtime) == 0xb000u &&
              runtime.calls.back() == "name-length" &&
              runtime.record.text_data == 0u &&
              runtime.record.text_length == 0u &&
              runtime.record.property == 0x1234u,
          "empty name leaves initialized string buffer empty");
}

void TestQueryExceptionReleasesTemporary() {
    Runtime runtime;
    runtime.tree.throw_on_read = true;
    try {
        (void)pal4::MonsterStorageObject::Create(
            Manager(), 0x5000u, runtime);
        Check(false, "tree failure must escape");
    } catch (const std::runtime_error&) {
    }
    Check(runtime.calls == std::vector<std::string>{
              "read:24652", "query-create", "query-destroy"},
          "tree lookup failure releases query by SEH cleanup path");
}

void TestAllocationAndConstructorFailures() {
    Runtime runtime;
    runtime.allocation = 0u;
    try {
        (void)pal4::MonsterStorageObject::Create(
            Manager(), 0x5000u, runtime);
        Check(false, "null allocation must be explicit");
    } catch (const std::runtime_error&) {
    }
    Check(runtime.calls.back() == "null-dereference" &&
              runtime.record.vtable == 0xaaaaaaaau,
          "original null allocation dereference is an explicit boundary");

    Runtime failed_constructor;
    failed_constructor.throw_on_byte = true;
    try {
        (void)pal4::MonsterStorageObject::Create(
            Manager(), 0x5000u, failed_constructor);
        Check(false, "constructor failure must escape");
    } catch (const std::runtime_error&) {
    }
    Check(failed_constructor.calls.back() == "free-object",
          "allocation is freed when record constructor throws");
}

void TestNameLengthFailureDoesNotFreeConstructedObject() {
    Runtime runtime;
    runtime.name_length = 0xfffffffeu;
    try {
        (void)pal4::MonsterStorageObject::Create(
            Manager(), 0x5000u, runtime);
        Check(false, "oversized name must throw");
    } catch (const std::length_error&) {
    }
    Check(runtime.calls.back() == "throw-length" &&
              runtime.record.vtable == 0x842d68u,
          "length error follows construction with no constructor-stage free");
}

void TestDestroyRecordReferenceBranches() {
    for (auto reference : {0u, 0xffu, 2u}) {
        Runtime runtime;
        runtime.record.text_data = 0xd001u;
        runtime.record.text_length = 4u;
        runtime.record.text_capacity = 31u;
        runtime.bytes[0xd000u] = static_cast<std::uint8_t>(reference);
        Check(pal4::MonsterStorageObject::Destroy(
                  runtime.record, runtime) ==
                  (reference == 2u ? 0xd000u : 0xdeadu) &&
                  runtime.calls == (reference == 2u ?
                      std::vector<std::string>{
                          "read-byte:53248", "write-byte:53248"} :
                      std::vector<std::string>{
                          "read-byte:53248", "free-string:53248"}) &&
                  runtime.record.vtable == 0x841e40u &&
                  runtime.record.text_data == 0u &&
                  runtime.record.text_length == 0u &&
                  runtime.record.text_capacity == 0u &&
                  runtime.record.flags == 0x12345678u &&
                  runtime.record.property == 0x7777u,
              "record destructor releases owned or shared string and resets vtable");
        if (reference == 2u)
            Check(runtime.bytes[0xd000u] == 1u,
                  "shared reference byte decremented");
    }
}

void TestDestroyEmptyRecord() {
    Runtime runtime;
    runtime.record.text_data = 0u;
    Check(pal4::MonsterStorageObject::Destroy(
              runtime.record, runtime) == 0u &&
              runtime.calls.empty() &&
              runtime.record.vtable == 0x841e40u,
          "empty record destruction avoids string-memory operations");
}

}  // namespace

int main() {
    Check(pal4::MonsterStorageObject::GetTypeCode() == 11u,
          "storage-record vtable type-code thunk");
    TestInitializeRecordPreservesUnwrittenBytes();
    TestAllocateAndInitialize();
    TestMissingSourceAndMissingAttribute();
    TestNonpositiveAttribute();
    TestCreateWithName();
    TestEmptyName();
    TestQueryExceptionReleasesTemporary();
    TestAllocationAndConstructorFailures();
    TestNameLengthFailureDoesNotFreeConstructedObject();
    TestDestroyRecordReferenceBranches();
    TestDestroyEmptyRecord();
    pal4::MonsterStorageRuntimeStub stub;
    Check(pal4::MonsterStorageObject::Create(
              Manager(false), 0x5000u, stub) == 0u,
          "default storage runtime links");
    std::cout << "12 monster storage object cases passed\n";
}
