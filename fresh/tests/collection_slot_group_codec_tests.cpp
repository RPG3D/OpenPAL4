#include "pal4/collection_slot_group_codec.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICollectionSlotGroupCodecRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> slots;
    std::vector<std::uint32_t> written_words;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> spans;
    std::vector<std::uint32_t> input_words;
    std::vector<std::uint32_t> input_span;
    std::vector<std::uint32_t> freed;
    std::vector<std::uint32_t> destroyed;
    std::vector<std::uint32_t> constructed;
    std::vector<std::uint32_t> appended;
    std::vector<std::uint32_t> allocation_results;
    std::uint32_t read_index = 0u;
    std::uint32_t allocated_bytes = 0u;
    std::uint32_t allocation_result = 0x5000u;
    bool fail_record = false;
    bool flag_valid = true;

    struct RecordRuntime final : pal4::IInventoryConditionStreamRuntime {
        Runtime& owner;
        explicit RecordRuntime(Runtime& runtime) : owner(runtime) {}
        std::uint32_t ReadWord(std::uint32_t address) override {
            return owner.words[address];
        }
        void WriteStreamWord(std::uint32_t stream,
                             std::uint32_t address,
                             std::uint32_t bytes) override {
            Check(stream == 0x2000u && bytes == 4u,
                  "record field stream");
            owner.calls.emplace_back("record field");
            owner.slots.push_back(address);
        }
        void ReadStreamWord(std::uint32_t stream, std::uint32_t address,
                            std::uint32_t bytes) override {
            Check(stream == 0x2000u && bytes == 4u,
                  "record read field stream");
            owner.calls.emplace_back("record read field");
            owner.slots.push_back(address);
        }
        std::int32_t SerializeRecord(std::uint32_t record,
                                     std::uint32_t stream) override {
            Check(stream == 0x2000u && record != 0u,
                  "record tail arguments");
            owner.calls.emplace_back("record tail");
            if (owner.fail_record) throw std::runtime_error("record failure");
            return -37;
        }
        std::int32_t DeserializeRecord(std::uint32_t record,
                                       std::uint32_t stream) override {
            Check(stream == 0x2000u && record != 0u,
                  "record read tail arguments");
            owner.calls.emplace_back("record read tail");
            return 0x66;
        }
    } record_runtime{*this};

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    void ReleaseForWrite(std::uint32_t object,
                         std::uint32_t stream) override {
        Check(object == 0x1000u && stream == 0x2000u,
              "object release arguments");
        calls.emplace_back("release");
    }
    void WriteSerializationFlag(std::uint32_t slot,
                                std::uint32_t stream,
                                const char* flag) override {
        Check(stream == 0x2000u &&
                  std::string(flag) == "EC_SERIALIZE_FLAG",
              "slot serialization flag");
        calls.emplace_back("flag");
        slots.push_back(slot);
    }
    void ReleaseSerializationFlag(std::uint32_t) override {
        calls.emplace_back("release flag");
    }
    pal4::IInventoryConditionStreamRuntime& RecordRuntime() override {
        return record_runtime;
    }
    void WriteStreamWord(std::uint32_t stream,
                         std::uint32_t value) override {
        Check(stream == 0x2000u, "word stream");
        calls.emplace_back("word");
        written_words.push_back(value);
    }
    void WriteStreamSpan(std::uint32_t stream, std::uint32_t address,
                         std::uint32_t bytes) override {
        Check(stream == 0x2000u, "span stream");
        calls.emplace_back("span");
        spans.emplace_back(address, bytes);
    }
    std::uint32_t CreateEmptyTemporaryTree() override {
        calls.emplace_back("tree create");
        return 0x6000u;
    }
    void DestroyTemporaryTree(std::uint32_t tree) override {
        Check(tree == 0x6000u, "temporary tree identity");
        calls.emplace_back("tree destroy");
    }
    void ReleaseForRead(std::uint32_t object,
                        std::uint32_t stream) override {
        Check(object == 0x1000u && stream == 0x2000u,
              "object read release arguments");
        calls.emplace_back("release read");
    }
    bool ReadSerializationFlag(std::uint32_t slot,
                               std::uint32_t stream,
                               const char* expected) override {
        Check(stream == 0x2000u &&
                  std::string(expected) == "EC_SERIALIZE_FLAG",
              "read serialization flag");
        calls.emplace_back("read flag");
        slots.push_back(slot);
        return flag_valid;
    }
    void MarkSerializationError(std::uint32_t) override {
        calls.emplace_back("mark error");
    }
    void DestroyRecord(std::uint32_t record) override {
        calls.emplace_back("destroy record");
        destroyed.push_back(record);
    }
    void ConstructRecord(std::uint32_t record) override {
        calls.emplace_back("construct record");
        constructed.push_back(record);
    }
    void AppendRecord(std::uint32_t slot,
                      std::uint32_t record) override {
        calls.emplace_back("append record");
        appended.push_back(record);
        const auto end = words[slot + 8u];
        words[end] = record;
        words[slot + 8u] = end + 4u;
    }
    std::uint32_t ReadStreamWord(std::uint32_t stream) override {
        Check(stream == 0x2000u && read_index < input_words.size(),
              "read word stream");
        calls.emplace_back("read word");
        return input_words[read_index++];
    }
    void ReadStreamSpan(std::uint32_t stream, std::uint32_t destination,
                        std::uint32_t bytes) override {
        Check(stream == 0x2000u && bytes == input_span.size() * 4u,
              "read span stream");
        calls.emplace_back("read span");
        spans.emplace_back(destination, bytes);
        for (std::uint32_t index = 0u; index < input_span.size(); ++index) {
            words[destination + index * 4u] = input_span[index];
        }
    }
    std::uint32_t AllocateMemory(std::uint32_t bytes) override {
        calls.emplace_back("allocate");
        allocated_bytes = bytes;
        if (!allocation_results.empty()) {
            const auto result = allocation_results.front();
            allocation_results.erase(allocation_results.begin());
            return result;
        }
        return allocation_result;
    }
    void FreeMemory(std::uint32_t address) override {
        calls.emplace_back("free");
        freed.push_back(address);
    }
};

void CheckSerialize(std::uint32_t begin, std::uint32_t end,
                    std::uint32_t expected_count) {
    Runtime runtime;
    runtime.words[0x107cu] = begin;
    runtime.words[0x1080u] = end;
    Check(pal4::CollectionSlotGroupCodec::Serialize(
              0x1000u, 0x2000u, runtime) == 1u,
          "serialize returns the seventh slot result");
    for (std::uint32_t index = 0u; index < 7u; ++index) {
        Check(runtime.slots[index] == 0x1008u + index * 16u,
              "all slots are encoded in address order");
    }
    Check(runtime.written_words ==
              std::vector<std::uint32_t>{0u, 0u, 0u, 0u,
                                         0u, 0u, 0u, expected_count, 0u} &&
              runtime.spans ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {begin, expected_count * 4u}},
          "queue count and byte span precede empty tree count");
    std::vector<std::string> expected_calls{"release"};
    for (int index = 0; index < 7; ++index) {
        expected_calls.insert(expected_calls.end(),
                              {"flag", "word", "release flag"});
    }
    expected_calls.insert(expected_calls.end(),
                          {"word", "span", "tree create", "word",
                           "tree destroy"});
    Check(runtime.calls == expected_calls,
          "serialization preserves release, slot, queue and tree order");
}

void CheckSerializeSlot() {
    Runtime runtime;
    runtime.words[0x100cu] = 0x3000u;
    runtime.words[0x1010u] = 0x3008u;
    runtime.words[0x3000u] = 0x4000u;
    runtime.words[0x3004u] = 0x4100u;
    Check(pal4::CollectionSlotGroupCodec::SerializeSlot(
              0x1008u, 0x2000u, runtime) == 1u &&
              runtime.written_words == std::vector<std::uint32_t>{2u} &&
              runtime.slots == std::vector<std::uint32_t>{
                  0x1008u, 0x4190u, 0x4290u} &&
              runtime.calls == std::vector<std::string>{
                  "flag", "word", "record field", "record tail",
                  "record field", "record tail", "release flag"},
          "slot writer composes with both record serializers");

    runtime.calls.clear();
    runtime.slots.clear();
    runtime.fail_record = true;
    bool threw = false;
    try {
        (void)pal4::CollectionSlotGroupCodec::SerializeSlot(
            0x1008u, 0x2000u, runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.calls.back() == "release flag" &&
              runtime.slots ==
                  std::vector<std::uint32_t>{0x1008u, 0x4190u},
          "slot flag is released when record serialization throws");
}

void CheckDeserializeWithGrowth() {
    Runtime runtime;
    runtime.input_words = {0u, 0u, 0u, 0u, 0u, 0u, 0u,
                           3u, 2u, 0xaaaau, 0xbbbbu};
    runtime.input_span = {11u, 12u, 13u};
    Check(pal4::CollectionSlotGroupCodec::Deserialize(
              0x1000u, 0x2000u, runtime) == 1u &&
              runtime.slots == std::vector<std::uint32_t>{
                  0x1008u, 0x1018u, 0x1028u, 0x1038u,
                  0x1048u, 0x1058u, 0x1068u} &&
              runtime.allocated_bytes == 12u &&
              runtime.freed == std::vector<std::uint32_t>{0u} &&
              runtime.words[0x107cu] == 0x5000u &&
              runtime.words[0x1080u] == 0x500cu &&
              runtime.words[0x1084u] == 0x500cu &&
              runtime.words[0x5000u] == 11u &&
              runtime.words[0x5004u] == 12u &&
              runtime.words[0x5008u] == 13u &&
              runtime.read_index == 11u &&
              runtime.spans ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x5000u, 12u}},
          "deserialize grows queue, fills words and discards two tree keys");
    std::vector<std::string> expected_calls{"release read"};
    for (int index = 0; index < 7; ++index) {
        expected_calls.insert(expected_calls.end(),
                              {"read flag", "read word", "release flag"});
    }
    expected_calls.insert(expected_calls.end(),
                          {"read word", "allocate", "free", "read span",
                           "read word", "read word", "read word"});
    Check(runtime.calls == expected_calls,
          "deserialize preserves slot, queue and tree read order");
}

void CheckDeserializeReusesCapacity() {
    Runtime runtime;
    runtime.words[0x107cu] = 0x7000u;
    runtime.words[0x1080u] = 0x700cu;
    runtime.words[0x1084u] = 0x7010u;
    runtime.words[0x7000u] = 91u;
    runtime.input_words = {0u, 0u, 0u, 0u, 0u, 0u, 0u,
                           2u, 0xffffffffu};
    runtime.input_span = {21u, 22u};
    Check(pal4::CollectionSlotGroupCodec::Deserialize(
              0x1000u, 0x2000u, runtime) == 1u &&
              runtime.words[0x107cu] == 0x7000u &&
              runtime.words[0x1080u] == 0x7008u &&
              runtime.words[0x1084u] == 0x7010u &&
              runtime.words[0x7000u] == 21u &&
              runtime.words[0x7004u] == 22u &&
              runtime.freed.empty() &&
              runtime.read_index == 9u,
          "deserialize clears old length and reuses sufficient capacity");

    Runtime failing;
    failing.input_words = {0u, 0u, 0u, 0u, 0u, 0u, 0u, 1u};
    failing.input_span = {1u};
    failing.allocation_result = 0u;
    bool threw = false;
    try {
        (void)pal4::CollectionSlotGroupCodec::Deserialize(
            0x1000u, 0x2000u, failing);
    } catch (const std::bad_alloc&) {
        threw = true;
    }
    Check(threw && failing.allocated_bytes == 4u &&
              failing.freed.empty() && failing.read_index == 8u,
          "allocation failure stops before the input span is read");
}

void CheckDeserializeSlot() {
    Runtime mismatch;
    mismatch.flag_valid = false;
    mismatch.words[0x100cu] = 0x3000u;
    mismatch.words[0x1010u] = 0x3004u;
    mismatch.words[0x3000u] = 0x4000u;
    Check(pal4::CollectionSlotGroupCodec::DeserializeSlot(
              0x1008u, 0x2000u, mismatch) == 0u &&
              mismatch.words[0x1010u] == 0x3004u &&
              mismatch.destroyed.empty() &&
              mismatch.calls == std::vector<std::string>{
                  "read flag", "mark error", "release flag"},
          "mismatched flag marks error without changing old records");

    Runtime runtime;
    runtime.words[0x100cu] = 0x3000u;
    runtime.words[0x1010u] = 0x3008u;
    runtime.words[0x3000u] = 0x4000u;
    runtime.words[0x3004u] = 0u;
    runtime.input_words = {2u};
    runtime.allocation_results = {0x5000u, 0x6000u};
    Check(pal4::CollectionSlotGroupCodec::DeserializeSlot(
              0x1008u, 0x2000u, runtime) == 0x66u &&
              runtime.destroyed == std::vector<std::uint32_t>{0x4000u} &&
              runtime.freed == std::vector<std::uint32_t>{0x4000u} &&
              runtime.constructed ==
                  std::vector<std::uint32_t>{0x5000u, 0x6000u} &&
              runtime.appended ==
                  std::vector<std::uint32_t>{0x5000u, 0x6000u} &&
              runtime.words[0x1010u] == 0x3008u &&
              runtime.words[0x3000u] == 0x5000u &&
              runtime.words[0x3004u] == 0x6000u &&
              runtime.slots == std::vector<std::uint32_t>{
                  0x1008u, 0x5190u, 0x6190u} &&
              runtime.calls == std::vector<std::string>{
                  "read flag", "destroy record", "free", "read word",
                  "allocate", "construct record", "record read field",
                  "record read tail", "append record", "allocate",
                  "construct record", "record read field",
                  "record read tail", "append record", "release flag"},
          "matching flag replaces old entries and composes record readers");

    Runtime failing;
    failing.input_words = {1u};
    failing.allocation_result = 0u;
    bool threw = false;
    try {
        (void)pal4::CollectionSlotGroupCodec::DeserializeSlot(
            0x1008u, 0x2000u, failing);
    } catch (const std::bad_alloc&) {
        threw = true;
    }
    Check(threw && failing.calls.back() == "release flag" &&
              failing.appended.empty(),
          "record allocation failure releases the flag before unwinding");
}

}  // namespace

int main() {
    try {
        CheckSerialize(0x5000u, 0x500cu, 3u);
        CheckSerialize(0u, 0u, 0u);
        CheckSerializeSlot();
        CheckDeserializeWithGrowth();
        CheckDeserializeReusesCapacity();
        CheckDeserializeSlot();
        std::cout << "collection_slot_group_codec_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
