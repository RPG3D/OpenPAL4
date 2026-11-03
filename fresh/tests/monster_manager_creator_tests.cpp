#include "pal4/monster_manager_creator.h"

#include <cstdlib>
#include <iostream>
#include <new>
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

struct Runtime final : pal4::MonsterManagerCreateRuntimeStub {
    std::uint32_t CStringLength(std::uint32_t address) override {
        events.emplace_back("length");
        Check(address == 0x12344u, "length reads the name address");
        return 6u;
    }
    void SetErrorFlag(pal4::ModeStringBufferRecord& text,
                      std::uint8_t flag) override {
        events.emplace_back(flag == 0u ? "set:0" : "set:1");
        Check((text.mode_word & 0xffu) == 0x44u,
              "temporary record retains name pointer's low byte");
        if (flag != 0u && text.buffer.data != 0u) {
            FreeTextHeader(text.buffer.data - 1u);
        }
        text.buffer = {};
    }
    bool AllocateText(pal4::ModeStringBufferRecord& text,
                      std::uint32_t length,
                      std::uint8_t mode) override {
        events.emplace_back("allocate-text");
        Check(length == 6u && mode == 1u,
              "name buffer uses exact length and mode one");
        if (!allocate_text) return false;
        text.buffer.data = 0x8001u;
        headers[0x8000u] = 0u;
        return true;
    }
    void CopyBytes(std::uint32_t destination,
                   std::uint32_t source,
                   std::uint32_t length) override {
        events.emplace_back("copy");
        Check(destination == 0x8001u && source == 0x12344u &&
                  length == 6u, "name copy preserves source and length");
    }
    void SetTextSize(pal4::ModeStringBufferRecord& text,
                     std::uint32_t length) override {
        events.emplace_back("size");
        Check(text.buffer.data == 0x8001u && length == 6u,
              "name size is committed after copy");
        text.buffer.length = length;
    }
    std::uint32_t SearchNode(
        std::uint32_t tree,
        const pal4::ModeStringBufferRecord& text) override {
        events.emplace_back("search");
        Check(tree == 0x5028u &&
                  text.buffer.data == (allocate_text ? 0x8001u : 0u),
              "search receives the manager tree and temporary name");
        return search_result;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        events.emplace_back("read");
        if (address == 0x502cu) return 0x7000u;
        Check(address == 0xb030u,
              "state flag reads source record at offset 0x30");
        return 0x55u;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        events.emplace_back("read-header");
        return headers.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        events.emplace_back("write-header");
        headers[address] = value;
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        events.emplace_back("write-grid-link");
        Check(address == 0xa204u && value == 0x5038u,
              "behavior manager stores parent grid pointer at +0x204");
    }
    void FreeTextHeader(std::uint32_t address) override {
        events.emplace_back("free-header");
        freed_headers.push_back(address);
    }
    void LoadScripts() override { events.emplace_back("load-scripts"); }
    void CleanupScripts() override {
        events.emplace_back("cleanup-scripts");
    }
    std::uint32_t AllocateObject(std::uint32_t bytes) override {
        events.emplace_back("allocate-object");
        Check(bytes == 0x73cu, "behavior object allocation is 0x73c bytes");
        return object_storage;
    }
    void FreeObjectStorage(std::uint32_t address) override {
        events.emplace_back("free-object");
        Check(address == 0xa000u,
              "failed behavior construction frees original allocation");
    }
    std::uint32_t ConstructObject(std::uint32_t storage) override {
        events.emplace_back("construct");
        Check(storage == 0xa000u,
              "constructor receives allocated behavior storage");
        if (constructor_throws) throw std::runtime_error("ctor");
        return storage;
    }
    void CallSlot30(std::uint32_t object, std::uint32_t name,
                    std::uint32_t config,
                    std::uint32_t enabled) override {
        events.emplace_back("slot30");
        Check(object == 0xa000u && name == 0x12344u &&
                  config == 0x1111u && enabled == 1u,
              "slot 0x30 receives name, config and final argument");
    }
    void CallSlot4(std::uint32_t object,
                   std::uint32_t value,
                   std::uint32_t zero) override {
        events.emplace_back("slot4");
        Check(object == 0xa000u && value == 0x2222u && zero == 0u,
              "slot 4 receives position and zero");
    }
    void CallSlot8(std::uint32_t object,
                   std::uint32_t value,
                   std::uint32_t zero) override {
        events.emplace_back("slot8");
        Check(object == 0xa000u && value == 0x3333u && zero == 0u,
              "slot 8 receives facing and zero");
    }
    void SetStateFlag(std::uint32_t object,
                      std::uint32_t flag) override {
        events.emplace_back("state-flag");
        Check(object == 0xa000u && flag == 0x55u,
              "state flag comes from source record word 12");
    }
    void AssignName(pal4::ModeStringBufferRecord& destination,
                    const pal4::ModeStringBufferRecord& source,
                    std::uint32_t start,
                    std::uint32_t count) override {
        events.emplace_back("assign");
        Check(start == 0u && count == 0xffffffffu &&
                  source.buffer.data == 0x8001u,
              "insert key copies the entire temporary name");
        destination.buffer = source.buffer;
        if (independent_key) {
            destination.buffer.data = 0x9001u;
            headers[0x9000u] = 0xffu;
        } else {
            headers[0x8000u] = assigned_reference_count;
        }
    }
    void InsertNode(std::uint32_t tree,
                    const pal4::ModeStringBufferRecord& name,
                    std::uint32_t object) override {
        events.emplace_back("insert");
        Check(tree == 0x5028u &&
                  name.buffer.data == (independent_key ? 0x9001u : 0x8001u) &&
                  object == 0xa000u,
              "tree insert receives copied key and behavior pointer");
    }
    void InitializeComponent(std::uint32_t object,
                             std::uint32_t record,
                             std::uint32_t dispatch) override {
        events.emplace_back("component");
        Check(object == 0xa000u && record == 0xb000u && dispatch == 0u,
              "component initialization uses original source record");
    }

    bool allocate_text{true};
    bool constructor_throws{};
    bool independent_key{};
    std::uint8_t assigned_reference_count{1u};
    std::uint32_t search_result{0x7000u};
    std::uint32_t object_storage{0xa000u};
    std::unordered_map<std::uint32_t, std::uint8_t> headers;
    std::vector<std::uint32_t> freed_headers;
    std::vector<std::string> events;
};

constexpr pal4::MonsterCreationArguments kArguments{
    0x12344u, 0x1111u, 0x2222u, 0x3333u, 0xb000u, 1u};

void TestSuccessfulCreate() {
    Runtime runtime;
    Check(pal4::MonsterManagerCreator::Create(
              0x5000u, kArguments, runtime) == 0xa000u,
          "create returns constructed monster manager");
    Check(runtime.events == std::vector<std::string>{
              "set:0", "length", "allocate-text", "copy", "size",
              "search", "read", "load-scripts", "allocate-object",
              "construct", "slot30", "slot4", "slot8",
              "cleanup-scripts", "read", "state-flag", "set:0",
              "assign", "insert", "read-header", "write-header",
              "write-grid-link", "component", "read-header",
              "free-header"},
          "successful path preserves IDA call and cleanup order");
    Check(runtime.headers.at(0x8000u) == 0u &&
              runtime.freed_headers ==
                  std::vector<std::uint32_t>{0x8000u},
          "two temporary owners decrement then free shared name");
}

void TestDuplicateName() {
    Runtime runtime;
    runtime.search_result = 0x7100u;
    Check(pal4::MonsterManagerCreator::Create(
              0x5000u, kArguments, runtime) == 0u &&
              runtime.events == std::vector<std::string>{
                  "set:0", "length", "allocate-text", "copy", "size",
                  "search", "read", "set:1", "free-header"},
          "duplicate name is rejected before loading scripts");
}

void TestTextAllocationFailureStillSearches() {
    Runtime runtime;
    runtime.allocate_text = false;
    runtime.search_result = 0x7100u;
    Check(pal4::MonsterManagerCreator::Create(
              0x5000u, kArguments, runtime) == 0u &&
              runtime.events == std::vector<std::string>{
                  "set:0", "length", "allocate-text", "search",
                  "read", "set:1"},
          "failed text allocation skips copy but still searches");
}

void TestObjectAllocationFailure() {
    Runtime runtime;
    runtime.object_storage = 0u;
    bool threw = false;
    try {
        static_cast<void>(pal4::MonsterManagerCreator::Create(
            0x5000u, kArguments, runtime));
    } catch (const std::bad_alloc&) {
        threw = true;
    }
    Check(threw && runtime.events == std::vector<std::string>{
              "set:0", "length", "allocate-text", "copy", "size",
              "search", "read", "load-scripts", "allocate-object",
              "read-header", "free-header"},
          "portable allocation failure exits at original null dereference");
}

void TestConstructorExceptionFreesStorage() {
    Runtime runtime;
    runtime.constructor_throws = true;
    bool threw = false;
    try {
        static_cast<void>(pal4::MonsterManagerCreator::Create(
            0x5000u, kArguments, runtime));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.events == std::vector<std::string>{
              "set:0", "length", "allocate-text", "copy", "size",
              "search", "read", "load-scripts", "allocate-object",
              "construct", "free-object", "read-header", "free-header"},
          "constructor exception frees allocation and temporary name");
}

void TestIndependentKeyAndSentinelHeader() {
    Runtime runtime;
    runtime.independent_key = true;
    Check(pal4::MonsterManagerCreator::Create(
              0x5000u, kArguments, runtime) == 0xa000u &&
              runtime.freed_headers ==
                  std::vector<std::uint32_t>{0x9000u, 0x8000u},
          "sentinel key header and separate source header are both freed");
}

void TestSharedReferenceDecrement() {
    Runtime runtime;
    runtime.assigned_reference_count = 2u;
    Check(pal4::MonsterManagerCreator::Create(
              0x5000u, kArguments, runtime) == 0xa000u &&
              runtime.headers.at(0x8000u) == 0u &&
              runtime.freed_headers.empty(),
          "nonterminal shared references are decremented twice");
}

}  // namespace

int main() {
    TestSuccessfulCreate();
    TestDuplicateName();
    TestTextAllocationFailureStillSearches();
    TestObjectAllocationFailure();
    TestConstructorExceptionFreesStorage();
    TestIndependentKeyAndSentinelHeader();
    TestSharedReferenceDecrement();
}
