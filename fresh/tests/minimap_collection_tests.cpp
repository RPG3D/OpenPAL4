#include "pal4/minimap_collection.h"
#include "pal4/range_no_op_callback.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IMinimapInputRuntime,
                      public pal4::IMinimapOutputRuntime {
public:
    std::unordered_map<std::uint32_t, std::uint8_t> memory;
    std::vector<std::uint32_t> frees;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;
    std::vector<std::uint32_t> serialized_values;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> serialized_bytes;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> allocation_sizes;
    std::vector<std::uint32_t> input_values;
    std::vector<std::uint32_t> input_words;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> input_reads;
    std::vector<std::string> input_events;
    std::size_t allocation_index = 0u;
    std::size_t value_index = 0u;
    std::size_t word_index = 0u;
    std::uint32_t allocation = 0x1000u;
    std::uint32_t allocated_bytes = 0u;
    bool throw_on_byte_write = false;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        allocated_bytes = bytes;
        allocation_sizes.push_back(bytes);
        input_events.emplace_back("allocate");
        if (allocation_index < allocations.size())
            return allocations[allocation_index++];
        return allocation;
    }
    void Free(std::uint32_t address) override {
        frees.push_back(address);
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        std::uint32_t value = 0u;
        for (std::uint32_t i = 0u; i < 4u; ++i)
            value |= static_cast<std::uint32_t>(memory[address + i]) <<
                     (i * 8u);
        return value;
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        writes.emplace_back(address, value);
        for (std::uint32_t i = 0u; i < 4u; ++i)
            memory[address + i] = static_cast<std::uint8_t>(value >> (i * 8u));
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        if (throw_on_byte_write) throw std::runtime_error("constructor");
        memory[address] = value;
    }
    std::uint32_t WriteValue(std::uint32_t value) override {
        serialized_values.push_back(value);
        return 0x77u;
    }
    std::uint32_t WriteBytes(std::uint32_t source,
                             std::uint32_t bytes) override {
        serialized_bytes.emplace_back(source, bytes);
        return 0x88u;
    }
    std::uint32_t ReadValue() override {
        Check(value_index < input_values.size(), "input value count");
        input_events.emplace_back("read-value");
        return input_values[value_index++];
    }
    void ReadBytes(std::uint32_t destination,
                   std::uint32_t bytes) override {
        input_events.emplace_back("read-bytes");
        input_reads.emplace_back(destination, bytes);
        Check(bytes % 4u == 0u, "input byte alignment");
        for (std::uint32_t offset = 0u; offset < bytes; offset += 4u) {
            Check(word_index < input_words.size(), "input word count");
            WriteDword(destination + offset, input_words[word_index++]);
        }
    }
    void ResizeInner(std::uint32_t element,
                     std::uint32_t count,
                     std::uint32_t fill) override {
        Check(fill == 0u, "inner resize fill value");
        input_events.emplace_back("resize");
        const auto begin = ReadDword(element + 4u);
        const auto end = ReadDword(element + 8u);
        const auto old_count = begin == 0u ? 0u : (end - begin) / 4u;
        for (std::uint32_t i = old_count; i < count; ++i)
            WriteDword(begin + i * 4u, fill);
        WriteDword(element + 8u, begin + count * 4u);
    }
    void InsertElement(std::uint32_t collection,
                       std::uint32_t element) override {
        Check(collection == 0x1004u, "outer collection address");
        input_events.emplace_back("insert");
        const auto end = ReadDword(collection + 8u);
        Check(end < ReadDword(collection + 12u),
              "outer collection has insertion capacity");
        WriteDword(end, element);
        WriteDword(collection + 8u, end + 4u);
    }
};

void TestAllocationAndConstruction() {
    Runtime runtime;
    for (std::uint32_t i = 0u; i < 24u; ++i)
        runtime.memory[0x1000u + i] = 0xaau;
    Check(pal4::MinimapCollection::AllocateAndConstruct(0x5au, runtime) ==
              0x1000u && runtime.allocated_bytes == 24u,
          "factory allocates 24 bytes and returns object address");
    Check(runtime.memory[0x1004u] == 0x5au &&
              runtime.memory[0x1005u] == 0xaau &&
              runtime.memory[0x1006u] == 0xaau &&
              runtime.memory[0x1007u] == 0xaau,
          "constructor writes one stack-derived byte and preserves neighbors");
    Check(runtime.writes ==
              std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {0x1008u, 0u}, {0x100cu, 0u}, {0x1010u, 0u},
                  {0x1000u, 0x841e2cu}, {0x1014u, 0u}},
          "constructor DWORD write order and vtable");
    Check(pal4::MinimapCollection::LabelAddress() == 0x8a595cu &&
              pal4::MinimapCollection::Label() == "Minimap",
          "virtual name accessor uses IDA literal");

    Runtime failure;
    failure.allocation = 0u;
    Check(pal4::MinimapCollection::AllocateAndConstruct(0xffu, failure) ==
              0u && failure.frees.empty(),
          "failed allocation skips constructor");
    Runtime throwing;
    throwing.throw_on_byte_write = true;
    bool caught = false;
    try {
        (void)pal4::MinimapCollection::AllocateAndConstruct(0x11u,
                                                              throwing);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && throwing.frees == std::vector<std::uint32_t>{0x1000u},
          "constructor exception frees allocated object");
}

void SeedCollection(Runtime& runtime) {
    runtime.WriteDword(0x1000u, 0x11111111u);
    runtime.WriteDword(0x1008u, 0x2000u);
    runtime.WriteDword(0x100cu, 0x200cu);
    runtime.WriteDword(0x1010u, 0x2020u);
    runtime.WriteDword(0x1014u, 3u);
    runtime.WriteDword(0x2000u, 0x3000u);
    runtime.WriteDword(0x2004u, 0u);
    runtime.WriteDword(0x2008u, 0x3100u);
    runtime.WriteDword(0x3004u, 0x4000u);
    runtime.WriteDword(0x3008u, 4u);
    runtime.WriteDword(0x300cu, 8u);
    runtime.WriteDword(0x3104u, 0x4100u);
    runtime.WriteDword(0x3108u, 4u);
    runtime.WriteDword(0x310cu, 8u);
    runtime.frees.clear();
    runtime.writes.clear();
}

void TestElementCleanup() {
    Runtime runtime;
    SeedCollection(runtime);
    Check(pal4::MinimapCollection::CleanupElements(0x1000u, runtime) ==
              0x200cu,
          "collection cleanup returns original end pointer");
    Check(runtime.frees ==
              std::vector<std::uint32_t>{0x4000u, 0x3000u,
                                         0x4100u, 0x3100u},
          "each element data is freed before its record");
    Check(runtime.ReadDword(0x3004u) == 0u &&
              runtime.ReadDword(0x3008u) == 0u &&
              runtime.ReadDword(0x300cu) == 0u &&
              runtime.ReadDword(0x3104u) == 0u &&
              runtime.ReadDword(0x100cu) == 0x2000u &&
              runtime.ReadDword(0x1014u) == 0u &&
              runtime.ReadDword(0x2000u) == 0x3000u,
          "cleanup clears element fields and logical length but leaves slots");
    runtime.frees.clear();
    Check(pal4::MinimapCollection::CleanupElements(0x1000u, runtime) ==
              0x2000u && runtime.frees.empty(),
          "empty collection returns begin without freeing elements");
}

void TestDestruction() {
    Runtime runtime;
    SeedCollection(runtime);
    Check(pal4::MinimapCollection::Destroy(0x1000u, 0u, runtime) ==
              0x1000u &&
              runtime.frees ==
                  std::vector<std::uint32_t>{0x4000u, 0x3000u,
                                             0x4100u, 0x3100u, 0x2000u},
          "destructor clears collection and frees backing array only");
    Check(runtime.ReadDword(0x1000u) == 0x841e40u &&
              runtime.ReadDword(0x1008u) == 0u &&
              runtime.ReadDword(0x100cu) == 0u &&
              runtime.ReadDword(0x1010u) == 0u,
          "cleanup installs base vtable and clears vector pointers");
    SeedCollection(runtime);
    Check(pal4::MinimapCollection::Destroy(0x1000u, 3u, runtime) ==
              0x1000u && runtime.frees.back() == 0x1000u &&
              runtime.frees.size() == 6u,
          "destructor flag bit zero frees object last");
}

void TestSerialization() {
    Runtime runtime;
    runtime.WriteDword(0x1008u, 0u);
    runtime.WriteDword(0x100cu, 0u);
    runtime.WriteDword(0x1014u, 0xabcdu);
    Check(pal4::MinimapCollection::Serialize(0x1000u, runtime) == 0x77u &&
              runtime.serialized_values ==
                  std::vector<std::uint32_t>{0xabcd0000u} &&
              runtime.serialized_bytes.empty(),
          "null backing array returns header writer result");

    runtime.serialized_values.clear();
    runtime.WriteDword(0x1008u, 0x2000u);
    runtime.WriteDword(0x100cu, 0x2000u);
    Check(pal4::MinimapCollection::Serialize(0x1000u, runtime) == 0u &&
              runtime.serialized_values ==
                  std::vector<std::uint32_t>{0xabcd0000u},
          "non-null empty backing array returns element count");

    runtime.serialized_values.clear();
    runtime.WriteDword(0x100cu, 0x2008u);
    runtime.WriteDword(0x2000u, 0x3000u);
    runtime.WriteDword(0x2004u, 0x3100u);
    runtime.WriteDword(0x3004u, 0x4000u);
    runtime.WriteDword(0x3008u, 0x4008u);
    runtime.WriteDword(0x3104u, 0u);
    runtime.WriteDword(0x3108u, 0u);
    Check(pal4::MinimapCollection::Serialize(0x1000u, runtime) == 2u &&
              runtime.serialized_values ==
                  std::vector<std::uint32_t>{0xabcd0002u, 2u, 0u} &&
              runtime.serialized_bytes ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x4000u, 8u}, {0u, 0u}},
          "serializer writes header, each length, and raw data in order");
}

void TestDeserialization() {
    Runtime empty;
    SeedCollection(empty);
    empty.input_values = {0xabc00000u};
    Check(pal4::MinimapCollection::Deserialize(
              0x1000u, 0x123456abu, empty) == 0u &&
              empty.ReadDword(0x1014u) == 0xabc0u &&
              empty.ReadDword(0x100cu) == 0x2000u &&
              empty.frees ==
                  std::vector<std::uint32_t>{0x4000u, 0x3000u,
                                             0x4100u, 0x3100u} &&
              empty.input_events == std::vector<std::string>{"read-value"},
          "empty input clears prior elements and stores header metadata");

    Runtime runtime;
    SeedCollection(runtime);
    runtime.input_values = {0x12340002u, 2u, 0u};
    runtime.input_words = {0x11111111u, 0x22222222u};
    runtime.allocations = {0x5000u, 0x5100u, 0x5200u};
    Check(pal4::MinimapCollection::Deserialize(
              0x1000u, 0x123456abu, runtime) == 2u &&
              runtime.ReadDword(0x1014u) == 0x1234u &&
              runtime.ReadDword(0x100cu) == 0x2008u &&
              runtime.ReadDword(0x2000u) == 0x5000u &&
              runtime.ReadDword(0x2004u) == 0x5200u,
          "two input elements are inserted in stream order");
    Check(runtime.allocation_sizes ==
              std::vector<std::uint32_t>{16u, 8u, 16u} &&
              runtime.memory[0x5000u] == 0xabu &&
              runtime.memory[0x5200u] == 0xabu &&
              runtime.ReadDword(0x5004u) == 0x5100u &&
              runtime.ReadDword(0x5008u) == 0x5108u &&
              runtime.ReadDword(0x500cu) == 0x5108u &&
              runtime.ReadDword(0x5100u) == 0x11111111u &&
              runtime.ReadDword(0x5104u) == 0x22222222u,
          "inner record layout, stack-derived byte and raw data");
    Check(runtime.input_reads ==
              std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {0x5100u, 8u}, {0u, 0u}} &&
              runtime.input_events ==
                  std::vector<std::string>{
                      "read-value", "allocate", "read-value", "allocate",
                      "resize", "read-bytes", "insert", "allocate",
                      "read-value", "resize", "read-bytes", "insert"},
          "input read, growth, resize and insertion ordering");
}

void TestBaseLifecycleAndRangeCallback() {
    Runtime runtime;
    pal4::MinimapCollection::SetBaseVtable(0x1000u, runtime);
    Check(runtime.ReadDword(0x1000u) ==
              pal4::MinimapCollection::kBaseVtableAddress &&
              runtime.frees.empty(),
          "base setter writes the verified minimap vtable");
    Check(pal4::MinimapCollection::DestroyBase(
              0x1000u, 0u, runtime) == 0x1000u &&
              runtime.frees.empty(),
          "base destructor returns the object without freeing for zero flags");
    Check(pal4::MinimapCollection::DestroyBase(
              0x1000u, 3u, runtime) == 0x1000u &&
              runtime.frees == std::vector<std::uint32_t>{0x1000u},
          "base destructor frees when low flag bit is set");
    const auto count = runtime.writes.size();
    pal4::RangeNoOpCallback::Invoke(0x1234u, 0x5678u);
    Check(runtime.writes.size() == count && runtime.frees.size() == 1u,
          "range callback has no observable effect");
}

}  // namespace

int main() {
    TestAllocationAndConstruction();
    TestElementCleanup();
    TestDestruction();
    TestSerialization();
    TestDeserialization();
    TestBaseLifecycleAndRangeCallback();
    return EXIT_SUCCESS;
}
