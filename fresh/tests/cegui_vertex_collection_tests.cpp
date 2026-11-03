#include "pal4/cegui_vertex_collection.h"

#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

pal4::CeguiVertexRecord Value(const std::uint8_t first) {
    pal4::CeguiVertexRecord result{};
    result.fill(first);
    return result;
}

class Runtime final : public pal4::CeguiVertexCollectionStub {
public:
    std::uint32_t allocation_result = 0x3000u;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> allocation_sizes;

    std::uint32_t Allocate(const std::uint32_t bytes) override {
        calls.emplace_back("allocate");
        allocation_sizes.push_back(bytes);
        return allocation_result;
    }
    void Free(const std::uint32_t address) override {
        calls.emplace_back("free:" + std::to_string(address));
    }
    void CopyRecord(const std::uint32_t destination,
                    const std::uint32_t source) override {
        calls.emplace_back("copy:" + std::to_string(source) + ":" +
                           std::to_string(destination));
        pal4::CeguiVertexCollectionStub::CopyRecord(destination, source);
    }
    void WriteRecord(const std::uint32_t destination,
                     const pal4::CeguiVertexRecord& value) override {
        calls.emplace_back("fill:" + std::to_string(destination));
        pal4::CeguiVertexCollectionStub::WriteRecord(destination, value);
    }
};

void EmptyCollectionAllocatesExactNewCount() {
    pal4::CeguiRendererState state;
    Runtime runtime;
    const auto result = pal4::CeguiVertexCollection::Insert(
        state, 0u, 3u, Value(0xaau), runtime);
    Check(result == 0u &&
              runtime.allocation_sizes == std::vector<std::uint32_t>{84u} &&
              state.ReadDword(204u) == 0x3000u &&
              state.ReadDword(208u) == 0x3054u &&
              state.ReadDword(212u) == 0x3054u,
          "empty insertion allocates three 28-byte records");
    for (std::uint32_t index = 0u; index < 3u; ++index) {
        Check(runtime.Record(0x3000u + 28u * index) == Value(0xaau),
              "new array contains each inserted value");
    }
    Check(runtime.calls.back() == "free:0",
          "reallocation invokes original free call for null old begin");
}

void GrowthPreservesPrefixAndSuffix() {
    pal4::CeguiRendererState state;
    Runtime runtime;
    state.WriteDword(204u, 0x1000u);
    state.WriteDword(208u, 0x1054u);
    state.WriteDword(212u, 0x1054u);
    runtime.WriteRecord(0x1000u, Value(1u));
    runtime.WriteRecord(0x101cu, Value(2u));
    runtime.WriteRecord(0x1038u, Value(3u));
    runtime.calls.clear();
    const auto result = pal4::CeguiVertexCollection::Insert(
        state, 0x101cu, 2u, Value(9u), runtime);
    Check(result == 0u &&
              runtime.allocation_sizes == std::vector<std::uint32_t>{168u} &&
              state.ReadDword(204u) == 0x3000u &&
              state.ReadDword(208u) == 0x308cu &&
              state.ReadDword(212u) == 0x30a8u,
          "growth uses old size plus max(old size, insertion count)");
    const std::uint8_t expected[]{1u, 9u, 9u, 2u, 3u};
    for (std::uint32_t index = 0u; index < 5u; ++index) {
        Check(runtime.Record(0x3000u + 28u * index) ==
                  Value(expected[index]),
              "growth copies prefix, fills gap and copies suffix");
    }
    Check(runtime.calls == std::vector<std::string>{
              "allocate", "copy:4096:12288", "fill:12316", "fill:12344",
              "copy:4124:12372", "copy:4152:12400", "free:4096"},
          "growth preserves copy and free order");
}

void InPlaceMoreInsertionThanTail() {
    pal4::CeguiRendererState state;
    Runtime runtime;
    state.WriteDword(204u, 0x1000u);
    state.WriteDword(208u, 0x1038u);
    state.WriteDword(212u, 0x10a8u);
    runtime.WriteRecord(0x1000u, Value(1u));
    runtime.WriteRecord(0x101cu, Value(2u));
    runtime.calls.clear();
    const auto result = pal4::CeguiVertexCollection::Insert(
        state, 0x101cu, 3u, Value(8u), runtime);
    Check(result == 0x108cu && state.ReadDword(208u) == 0x108cu &&
              runtime.allocation_sizes.empty(),
          "tail-short insertion grows end without reallocating");
    const std::uint8_t expected[]{1u, 8u, 8u, 8u, 2u};
    for (std::uint32_t index = 0u; index < 5u; ++index) {
        Check(runtime.Record(0x1000u + 28u * index) ==
                  Value(expected[index]),
              "tail-short insertion copies tail then fills both sections");
    }
}

void InPlaceEnoughTailCopiesBackward() {
    pal4::CeguiRendererState state;
    Runtime runtime;
    state.WriteDword(204u, 0x1000u);
    state.WriteDword(208u, 0x1070u);
    state.WriteDword(212u, 0x10a8u);
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        runtime.WriteRecord(0x1000u + 28u * index, Value(index + 1u));
    }
    runtime.calls.clear();
    const auto result = pal4::CeguiVertexCollection::Insert(
        state, 0x101cu, 2u, Value(7u), runtime);
    Check(result == 0x1054u && state.ReadDword(208u) == 0x10a8u,
          "tail-long insertion returns end of inserted gap");
    const std::uint8_t expected[]{1u, 7u, 7u, 2u, 3u, 4u};
    for (std::uint32_t index = 0u; index < 6u; ++index) {
        Check(runtime.Record(0x1000u + 28u * index) ==
                  Value(expected[index]),
              "tail-long insertion preserves overlapping source records");
    }
    Check(runtime.calls[0] == "copy:4152:4208" &&
              runtime.calls[1] == "copy:4180:4236" &&
              runtime.calls[2] == "copy:4124:4180",
          "append and backward shift follow IDA order");
}

void ZeroCountAndFailures() {
    pal4::CeguiRendererState state;
    Runtime runtime;
    state.WriteDword(204u, 0x1000u);
    state.WriteDword(208u, 0x101cu);
    state.WriteDword(212u, 0x1038u);
    Check(pal4::CeguiVertexCollection::Insert(
              state, 0x1000u, 0u, Value(0u), runtime) == 0u &&
              runtime.calls.empty(),
          "zero-count insertion makes no memory calls");
    bool invalid = false;
    try {
        static_cast<void>(pal4::CeguiVertexCollection::Insert(
            state, 0x1001u, 1u, Value(0u), runtime));
    } catch (const std::invalid_argument&) {
        invalid = true;
    }
    Check(invalid, "misaligned insertion position is rejected");
    runtime.allocation_result = 0u;
    bool failed = false;
    try {
        static_cast<void>(pal4::CeguiVertexCollection::Insert(
            state, 0x1000u, 3u, Value(0u), runtime));
    } catch (const std::bad_alloc&) {
        failed = true;
    }
    Check(failed && state.ReadDword(204u) == 0x1000u &&
              state.ReadDword(208u) == 0x101cu &&
              state.ReadDword(212u) == 0x1038u,
          "failed growth keeps original collection pointers");
}

void ForwardMoveUsesSourceThroughOldEnd() {
    pal4::CeguiRendererState state;
    Runtime runtime;
    state.WriteDword(204u, 0x1000u);
    state.WriteDword(208u, 0x108cu);
    state.WriteDword(212u, 0x10a8u);
    for (std::uint32_t index = 0u; index < 5u; ++index) {
        runtime.WriteRecord(0x1000u + 28u * index, Value(index + 1u));
    }
    runtime.calls.clear();
    const auto result = pal4::CeguiVertexCollection::MoveRange(
        state, 0x1000u, 0x1038u, runtime);
    Check(result == 0x1000u && state.ReadDword(208u) == 0x1054u &&
              runtime.Record(0x1000u) == Value(3u) &&
              runtime.Record(0x101cu) == Value(4u) &&
              runtime.Record(0x1038u) == Value(5u),
          "move copies three records forward and truncates collection end");
    Check(runtime.calls == std::vector<std::string>{
              "copy:4152:4096", "copy:4180:4124", "copy:4208:4152"},
          "move visits source records in increasing address order");
    runtime.calls.clear();
    Check(pal4::CeguiVertexCollection::MoveRange(
              state, 0x101cu, 0x1054u, runtime) == 0x101cu &&
              state.ReadDword(208u) == 0x101cu &&
              runtime.calls.empty(),
          "empty move only rewrites end to destination");
}

}  // namespace

int main() {
    EmptyCollectionAllocatesExactNewCount();
    GrowthPreservesPrefixAndSuffix();
    InPlaceMoreInsertionThanTail();
    InPlaceEnoughTailCopiesBackward();
    ZeroCountAndFailures();
    ForwardMoveUsesSourceThroughOldEnd();
    std::cout << "6 CEGUI vertex collection test cases passed\n";
}
