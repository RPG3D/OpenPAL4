#include "pal4/shared_text_record_array.h"

#include "pal4/arena_sequence_resources.h"
#include "pal4/utilities_record_array_runtime.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

struct Runtime final : pal4::IArenaSequenceRuntime {
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> frees;
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t) override { return 0u; }
    std::uint32_t Free(std::uint32_t pointer) override {
        frees.push_back(pointer);
        return 0u;
    }
    std::uint32_t Call(pal4::ArenaSequenceCall,
                       std::initializer_list<std::uint32_t>) override {
        return 0u;
    }
};

struct InsertRuntime final : pal4::IUtilitiesRecordArrayRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;

    InsertRuntime(std::uint32_t used, std::uint32_t capacity) {
        const auto begin = capacity == 0u ? 0u : 0x2000u;
        words[0x1004u] = begin;
        words[0x1008u] = begin + used * 20u;
        words[0x100cu] = begin + capacity * 20u;
        for (std::uint32_t i = 0; i < used; ++i)
            words[begin + i * 20u] = i + 1u;
        words[0x9000u] = 9u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        const auto found = words.find(address);
        Check(found != words.end(), "insert reads initialized word");
        return found->second;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("allocate:" + std::to_string(bytes));
        return 0x5000u;
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
    void CopyConstruct(std::uint32_t destination, std::uint32_t source,
                       std::uint32_t stride) override {
        Check(stride == 20u, "copy stride");
        words[destination] = ReadWord(source);
        calls.push_back("copy");
    }
    void Assign(std::uint32_t destination, std::uint32_t source,
                std::uint32_t stride) override {
        Check(stride == 20u, "assign stride");
        words[destination] = ReadWord(source);
        calls.push_back("assign");
    }
    void MoveBackward(std::uint32_t begin, std::uint32_t end,
                      std::uint32_t destination_end,
                      std::uint32_t stride) override {
        Check(stride == 20u, "move stride");
        const auto displacement = destination_end - end;
        while (end != begin) {
            end -= stride;
            words[end + displacement] = ReadWord(end);
        }
        calls.push_back("move");
    }
    void DestroyRange(std::uint32_t begin, std::uint32_t end,
                      std::uint32_t stride) override {
        Check(stride == 20u, "destroy stride");
        calls.push_back("destroy:" + std::to_string((end - begin) / stride));
    }
};

void CheckContents(InsertRuntime& runtime, std::uint32_t begin,
                   std::initializer_list<std::uint32_t> expected) {
    std::uint32_t address = begin;
    for (auto value : expected) {
        Check(runtime.ReadWord(address) == value, "record order");
        address += 20u;
    }
    Check(runtime.ReadWord(0x1008u) == address, "record range end");
}

void TestInsertCopies() {
    InsertRuntime runtime(3u, 5u);
    pal4::SharedTextRecordArray::InsertCopies(
        0x1000u, 0x2014u, 1u, 0x9000u, runtime);
    CheckContents(runtime, 0x2000u, {1u, 9u, 2u, 3u});
    Check(runtime.calls == std::vector<std::string>{
          "copy", "move", "assign"},
          "sufficient tail copies last, moves backward and assigns source");

    runtime = InsertRuntime(3u, 7u);
    pal4::SharedTextRecordArray::InsertCopies(
        0x1000u, 0x2028u, 3u, 0x9000u, runtime);
    CheckContents(runtime, 0x2000u, {1u, 2u, 9u, 9u, 9u, 3u});
    Check(runtime.calls == std::vector<std::string>{
          "copy", "copy", "copy", "assign"},
          "short tail moves tail then constructs excess source copies");

    runtime = InsertRuntime(3u, 3u);
    pal4::SharedTextRecordArray::InsertCopies(
        0x1000u, 0x2014u, 2u, 0x9000u, runtime);
    CheckContents(runtime, 0x5000u, {1u, 9u, 9u, 2u, 3u});
    Check(runtime.ReadWord(0x100cu) == 0x5078u &&
          runtime.calls == std::vector<std::string>{
              "allocate:120", "copy", "copy", "copy", "copy", "copy",
              "destroy:3", "free:8192"},
          "reallocation copies prefix, sources and suffix before release");

    runtime = InsertRuntime(0u, 0u);
    pal4::SharedTextRecordArray::InsertCopies(
        0x1000u, 0u, 2u, 0x9000u, runtime);
    CheckContents(runtime, 0x5000u, {9u, 9u});
    Check(runtime.ReadWord(0x100cu) == 0x5028u &&
          runtime.calls.front() == "allocate:40" &&
          runtime.calls.back() == "free:0",
          "null array allocates and forwards null free");

    runtime = InsertRuntime(2u, 4u);
    pal4::SharedTextRecordArray::InsertCopies(
        0x1000u, 0x2014u, 0u, 0x9000u, runtime);
    CheckContents(runtime, 0x2000u, {1u, 2u});
    Check(runtime.calls.empty(), "zero count leaves storage untouched");
}

}  // namespace

int main() {
    TestInsertCopies();
    static_assert(sizeof(pal4::SharedTextRecordArrayState) == 16u);

    Runtime runtime;
    pal4::SharedTextRecordArrayState state{0x12345678u, 0x1000u,
                                           0x1028u, 0x1050u};
    runtime.words[0x1004u] = 0x2001u;
    runtime.words[0x1018u] = 0x3001u;
    runtime.bytes[0x2000u] = 0u;
    runtime.bytes[0x3000u] = 2u;
    Check(pal4::SharedTextRecordArray::Clear(state, runtime) == 0,
          "clear returns zero");
    Check(runtime.frees == std::vector<std::uint32_t>{0x2000u, 0x1000u},
          "releases records in order before freeing array");
    Check(runtime.bytes[0x3000u] == 1u,
          "shared second record decrements its reference count");
    Check(state.prefix == 0x12345678u && state.begin == 0u &&
          state.end == 0u && state.capacity_end == 0u,
          "clears only the three array pointers");

    runtime.frees.clear();
    state = {0x55u, 0x4000u, 0x4000u, 0x4040u};
    Check(pal4::SharedTextRecordArray::Clear(state, runtime) == 0 &&
          runtime.frees == std::vector<std::uint32_t>{0x4000u} &&
          state.prefix == 0x55u && state.capacity_end == 0u,
          "empty allocated range still frees its allocation");

    runtime.frees.clear();
    state = {0x66u, 0u, 0u, 0u};
    Check(pal4::SharedTextRecordArray::Clear(state, runtime) == 0 &&
          runtime.frees == std::vector<std::uint32_t>{0u} &&
          state.prefix == 0x66u,
          "null range forwards the native null free");
}
