#include "pal4/host_server_data_codec.h"

#include <array>
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

struct Write {
    bool count;
    std::uint32_t stream;
    std::uint32_t value;
    std::uint32_t size;
    bool operator==(const Write&) const = default;
};

struct Runtime final : pal4::IHostServerStreamRuntime {
    std::vector<Write> writes;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> frees;
    std::uint32_t marker_data{0x9001u};
    std::uint8_t first_count_result{1u};
    std::uint32_t count_calls{};
    std::uint8_t observed_flag{};
    std::string marker_text;

    void AssignMarker(pal4::ConfigStringState& marker,
                      std::string_view text) override {
        observed_flag = marker.flag;
        marker_text = text;
        marker.data = marker_data;
        marker.size = static_cast<std::uint32_t>(text.size());
    }
    std::uint32_t EmptyStringAddress() override { return 0xf000u; }
    std::uint8_t WriteCount(std::uint32_t stream,
                            std::uint32_t value) override {
        writes.push_back({true, stream, value, 4u});
        ++count_calls;
        return count_calls == 1u ? first_count_result : 0u;
    }
    std::uint8_t WriteBytes(std::uint32_t stream, std::uint32_t address,
                            std::uint32_t size) override {
        writes.push_back({false, stream, address, size});
        return 0u;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    void Free(std::uint32_t address) override { frees.push_back(address); }
};

struct ReadRuntime final : pal4::IHostServerReadRuntime {
    std::array<std::uint32_t, 11> counts{};
    std::array<std::vector<std::uint32_t>, 11> payloads{};
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> frees;
    std::vector<Write> reads;
    std::uint32_t next_allocation{0x8000u};
    std::uint32_t count_index{};
    std::uint32_t data_index{};
    std::uint32_t pair_fill_calls{};
    std::uint8_t marker_flag{};
    bool marker_matches{true};

    std::uint8_t ReadMarker(std::uint32_t stream,
                            pal4::ConfigStringState& marker) override {
        Check(stream == 0x1234u, "marker stream address");
        marker_flag = marker.flag;
        marker.data = 0x9001u;
        marker.size = 17u;
        return 0u;
    }
    std::int32_t CompareMarker(const pal4::ConfigStringState& marker,
                               std::string_view expected) override {
        Check(marker.size == 17u && expected == "MGC_HOST_SER_FLAG",
              "read marker comparison arguments");
        return marker_matches ? 0 : 1;
    }
    std::uint8_t ReadCount(std::uint32_t stream,
                           std::uint32_t& count) override {
        Check(stream == 0x1234u && count_index < counts.size(),
              "count read stream and index");
        count = counts[count_index++];
        reads.push_back({true, stream, count, 4u});
        return 0u;
    }
    std::uint8_t ReadBytes(std::uint32_t stream, std::uint32_t destination,
                           std::uint32_t size) override {
        Check(stream == 0x1234u && data_index < payloads.size(),
              "data read stream and index");
        if (data_index == 0u && size == 12u)
            Check(words.at(destination) == 0u &&
                      words.at(destination + 4u) == 0u &&
                      words.at(destination + 8u) == 0u,
                  "word array was cleared and zero-filled before reading");
        if (data_index == 1u && size == 8u)
            Check(words.at(destination) == 0xabu &&
                      words.at(destination + 4u) == 0xcdu,
                  "pair array was cleared and filled before reading");
        reads.push_back({false, stream, destination, size});
        const auto& payload = payloads[data_index++];
        Check(payload.size() * 4u == size, "read payload matches byte count");
        for (std::size_t i = 0; i < payload.size(); ++i)
            words[destination + static_cast<std::uint32_t>(i) * 4u] = payload[i];
        return 0u;
    }
    std::array<std::uint32_t, 2> PairFillWords() override {
        ++pair_fill_calls;
        return {0xabu, 0xcdu};
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        allocations.push_back(size);
        const auto result = next_allocation;
        next_allocation += 0x100u;
        return result;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    void BeforeFreePairs(std::uint32_t, std::uint32_t) override {}
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    void Free(std::uint32_t address) override { frees.push_back(address); }
};

void TestArrayOrderAndSizes() {
    pal4::IndexedCollectionResetState state{};
    for (std::uint32_t index = 0u; index < 5u; ++index) {
        state.values[index].begin = 0x1000u + index * 0x100u;
        state.values[index].end = state.values[index].begin + 4u * (index + 1u);
        state.key_values[index].begin = 0x3000u + index * 0x100u;
        state.key_values[index].end = state.key_values[index].begin +
                                      8u * (5u - index);
    }
    state.key_values[5].begin = 0x7000u;
    state.key_values[5].end = 0x700cu;
    Runtime runtime;
    runtime.bytes[0x9000u] = 0u;
    Check(pal4::HostServerDataCodec::Serialize(state, 0x1234u, runtime),
          "serialization returns true");
    Check(runtime.observed_flag == 0x34u &&
              runtime.marker_text == "MGC_HOST_SER_FLAG" &&
              runtime.writes.size() == 24u && runtime.count_calls == 12u &&
              runtime.writes[0] == Write{true, 0x1234u, 17u, 4u} &&
              runtime.writes[1] == Write{false, 0x1234u, 0x9001u, 17u},
          "marker is prepared and written before the arrays");
    for (std::uint32_t index = 0u; index < 5u; ++index) {
        const auto at = 2u + 4u * index;
        Check(runtime.writes[at] == Write{true, 0x1234u, index + 1u, 4u} &&
                  runtime.writes[at + 1u] == Write{
                      false, 0x1234u, 0x1000u + index * 0x100u,
                      4u * (index + 1u)} &&
                  runtime.writes[at + 2u] == Write{
                      true, 0x1234u, 5u - index, 4u} &&
                  runtime.writes[at + 3u] == Write{
                      false, 0x1234u, 0x3000u + index * 0x100u,
                      8u * (5u - index)},
              "five interleaved value and pair arrays retain order and size");
    }
    Check(runtime.writes[22] == Write{true, 0x1234u, 3u, 4u} &&
              runtime.writes[23] == Write{false, 0x1234u, 0x7000u, 12u} &&
              runtime.frees == std::vector<std::uint32_t>{0x9000u},
          "last array has four-byte stride and marker header is freed");
}

void TestMarkerGateAndReferenceCount() {
    pal4::IndexedCollectionResetState state{};
    Runtime runtime;
    runtime.first_count_result = 0u;
    runtime.bytes[0x9000u] = 2u;
    Check(pal4::HostServerDataCodec::Serialize(state, 0u, runtime) &&
              runtime.writes.size() == 23u &&
              runtime.writes[0] == Write{true, 0u, 17u, 4u} &&
              runtime.writes[1] == Write{true, 0u, 0u, 4u} &&
              runtime.writes[2] == Write{false, 0u, 0u, 0u} &&
              runtime.bytes[0x9000u] == 1u && runtime.frees.empty(),
          "failed marker length write skips text but continues empty arrays");

    runtime = {};
    runtime.marker_data = 0u;
    Check(pal4::HostServerDataCodec::Serialize(state, 0u, runtime) &&
              runtime.writes[1] == Write{false, 0u, 0xf000u, 17u} &&
              runtime.frees.empty(),
          "null marker data uses the empty-string address and needs no release");

    runtime = {};
    runtime.bytes[0x9000u] = 0xffu;
    Check(pal4::HostServerDataCodec::Serialize(state, 0u, runtime) &&
              runtime.frees == std::vector<std::uint32_t>{0x9000u},
          "0xff marker reference header is freed");
}

void TestReadMarkerMismatch() {
    pal4::IndexedCollectionResetState state{};
    state.values[0].begin = 0x1111u;
    ReadRuntime runtime;
    runtime.marker_matches = false;
    runtime.bytes[0x9000u] = 2u;
    Check(!pal4::HostServerDataCodec::Deserialize(
              state, 0x1234u, runtime) &&
              state.values[0].begin == 0x1111u &&
              runtime.marker_flag == 0x34u &&
              runtime.count_index == 0u && runtime.data_index == 0u &&
              runtime.bytes[0x9000u] == 1u && runtime.frees.empty(),
          "mismatched marker releases its reference without touching arrays");
}

void TestReadGrowthAndShrink() {
    pal4::IndexedCollectionResetState state{};
    state.values[0] = {0u, 0x1000u, 0x1008u, 0x1008u};
    state.key_values[0] = {0u, 0x2000u, 0x2018u, 0x2020u};
    ReadRuntime runtime;
    runtime.bytes[0x9000u] = 0u;
    runtime.words[0x1000u] = 1u;
    runtime.words[0x1004u] = 2u;
    runtime.counts = {3u, 1u, 0u, 2u, 0u, 0u,
                      0u, 0u, 0u, 0u, 2u};
    runtime.payloads[0] = {10u, 20u, 30u};
    runtime.payloads[1] = {70u, 80u};
    runtime.payloads[3] = {100u, 101u, 102u, 103u};
    runtime.payloads[10] = {500u, 600u};

    Check(pal4::HostServerDataCodec::Deserialize(state, 0x1234u, runtime),
          "matching marker deserializes");
    Check(runtime.count_index == 11u && runtime.data_index == 11u &&
              runtime.reads.size() == 22u &&
              runtime.allocations ==
                  std::vector<std::uint32_t>{12u, 16u, 8u} &&
              runtime.frees ==
                  std::vector<std::uint32_t>{0x1000u, 0u, 0u, 0x9000u} &&
              runtime.pair_fill_calls == 2u,
          "all eleven counts and data blocks are read with expected growth");
    Check(state.values[0].begin == 0x8000u &&
              state.values[0].end == 0x800cu &&
              state.values[0].capacity_end == 0x800cu &&
              runtime.words.at(0x8000u) == 10u &&
              runtime.words.at(0x8004u) == 20u &&
              runtime.words.at(0x8008u) == 30u &&
              state.key_values[0].begin == 0x2000u &&
              state.key_values[0].end == 0x2008u &&
              runtime.words.at(0x2000u) == 70u &&
              runtime.words.at(0x2004u) == 80u,
          "value array grows and pair array shrinks before payload reads");
    Check(state.key_values[1].begin == 0x8100u &&
              state.key_values[1].end == 0x8110u &&
              runtime.words.at(0x8100u) == 100u &&
              runtime.words.at(0x810cu) == 103u &&
              state.key_values[5].begin == 0x8200u &&
              state.key_values[5].end == 0x8208u &&
              runtime.words.at(0x8200u) == 500u &&
              runtime.words.at(0x8204u) == 600u &&
              runtime.reads[20] == Write{true, 0x1234u, 2u, 4u} &&
              runtime.reads[21] == Write{false, 0x1234u, 0x8200u, 8u},
          "pair growth and final four-byte array preserve layout and order");
}

void TestRoundTripLayout() {
    pal4::IndexedCollectionResetState source{};
    source.values[0] = {0u, 0x1000u, 0x1008u, 0x1008u};
    source.key_values[0] = {0u, 0x2000u, 0x2010u, 0x2010u};
    source.key_values[5] = {0u, 0x3000u, 0x3004u, 0x3004u};
    const std::map<std::uint32_t, std::uint32_t> source_words{
        {0x1000u, 11u}, {0x1004u, 22u},
        {0x2000u, 31u}, {0x2004u, 32u},
        {0x2008u, 41u}, {0x200cu, 42u}, {0x3000u, 99u}};
    Runtime writer;
    writer.bytes[0x9000u] = 0u;
    Check(pal4::HostServerDataCodec::Serialize(source, 0x1234u, writer),
          "round-trip writer succeeds");

    ReadRuntime reader;
    reader.bytes[0x9000u] = 0u;
    for (std::size_t index = 0u; index < 11u; ++index) {
        const auto& count = writer.writes[2u + index * 2u];
        const auto& data = writer.writes[3u + index * 2u];
        Check(count.count && !data.count,
              "serialized arrays alternate count and payload");
        reader.counts[index] = count.value;
        for (std::uint32_t offset = 0u; offset < data.size; offset += 4u)
            reader.payloads[index].push_back(
                source_words.at(data.value + offset));
    }
    pal4::IndexedCollectionResetState destination{};
    Check(pal4::HostServerDataCodec::Deserialize(
              destination, 0x1234u, reader) &&
              destination.values[0].end - destination.values[0].begin == 8u &&
              destination.key_values[0].end -
                  destination.key_values[0].begin == 16u &&
              destination.key_values[5].end -
                  destination.key_values[5].begin == 4u &&
              reader.words.at(destination.values[0].begin) == 11u &&
              reader.words.at(destination.values[0].begin + 4u) == 22u &&
              reader.words.at(destination.key_values[0].begin) == 31u &&
              reader.words.at(destination.key_values[0].begin + 12u) == 42u &&
              reader.words.at(destination.key_values[5].begin) == 99u,
          "serialized layout round-trips through all three array kinds");
}

}  // namespace

int main() {
    TestArrayOrderAndSizes();
    TestMarkerGateAndReferenceCount();
    TestReadMarkerMismatch();
    TestReadGrowthAndShrink();
    TestRoundTripLayout();
    pal4::HostServerStreamRuntimeStub stub;
    pal4::IndexedCollectionResetState state{};
    Check(pal4::HostServerDataCodec::Serialize(state, 0u, stub),
          "default stream stub links");
    pal4::HostServerReadRuntimeStub read_stub;
    Check(!pal4::HostServerDataCodec::Deserialize(state, 0u, read_stub),
          "default read runtime stub links");
    std::cout << "7 host server data codec cases passed\n";
}
