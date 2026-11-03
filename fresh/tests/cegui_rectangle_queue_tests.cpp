#include "pal4/cegui_rectangle_queue.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime : public pal4::CeguiRectangleQueueStub {
public:
    std::vector<std::string> events;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> freed;
    std::uint32_t forwarded_unused = 0u;
    std::uint32_t forwarded_depth = 0u;
    std::uint32_t forwarded_texture = 0u;
    std::uint32_t forwarded_colours = 0u;

    std::uint32_t Forward(
        pal4::CeguiRendererState& /*state*/,
        const pal4::CeguiRectWords& /*rectangle*/,
        const std::uint32_t unused, const std::uint32_t depth,
        const std::uint32_t texture,
        const pal4::CeguiRectWords& /*uv*/,
        const std::array<pal4::CeguiColourWords, 4>& /*colours*/,
        const std::uint32_t colour_source_address) override {
        events.emplace_back("forward");
        forwarded_unused = unused;
        forwarded_depth = depth;
        forwarded_texture = texture;
        forwarded_colours = colour_source_address;
        return 0x12345678u;
    }

    void AssignRect(pal4::CeguiRectWords& destination,
                    const pal4::CeguiRectWords& source) override {
        events.emplace_back("rect");
        pal4::CeguiRectangleQueueStub::AssignRect(destination, source);
    }

    void CopyColour(pal4::CeguiColourWords& destination,
                    const pal4::CeguiColourWords& source) override {
        events.emplace_back("colour");
        pal4::CeguiRectangleQueueStub::CopyColour(destination, source);
    }

    std::uint32_t Allocate(const std::uint32_t bytes) override {
        events.emplace_back("allocate");
        allocations.push_back(bytes);
        return pal4::CeguiRectangleQueueStub::Allocate(bytes);
    }

    void CopyRecord(const std::uint32_t destination,
                    const std::uint32_t source) override {
        events.emplace_back("copy");
        pal4::CeguiRectangleQueueStub::CopyRecord(destination, source);
    }

    void FillRecord(const std::uint32_t destination,
                    const pal4::CeguiRectangleRecord& record) override {
        events.emplace_back("fill");
        pal4::CeguiRectangleQueueStub::FillRecord(destination, record);
    }

    void CopyStructuredRange(
        const std::uint32_t begin, const std::uint32_t end,
        const pal4::CeguiRectangleRecord& record) override {
        events.emplace_back("structured");
        pal4::CeguiRectangleQueueStub::CopyStructuredRange(begin, end,
                                                             record);
    }

    void RangeNoOp(const std::uint32_t begin,
                   const std::uint32_t end) override {
        events.emplace_back("range");
        Check(end >= begin, "cleanup range is ordered");
    }

    void Free(const std::uint32_t address) override {
        events.emplace_back("free");
        freed.push_back(address);
    }
};

constexpr pal4::CeguiRectWords kRect{11u, 22u, 33u, 44u};
constexpr pal4::CeguiRectWords kUv{55u, 66u, 77u, 88u};
const std::array<pal4::CeguiColourWords, 4> kColours{{
    {1.0f, 0.0f, 0.5f, 0.25f, 7.0f, 8.0f},
    {0.0f, 1.0f, 0.0f, 0.0f, 7.0f, 8.0f},
    {0.0f, 0.0f, 1.0f, 0.0f, 7.0f, 8.0f},
    {0.0f, 0.0f, 0.0f, 1.0f, 7.0f, 8.0f},
}};

std::uint32_t Draw(pal4::CeguiRendererState& state, Runtime& runtime) {
    return pal4::CeguiRectangleQueue::Draw(
        state, kRect, 0x1111u, 0x2222u, 0x3333u, kUv,
        kColours, 0x4444u, runtime);
}

void DisabledQueueForwardsAllArguments() {
    pal4::CeguiRendererState state;
    Runtime runtime;
    Check(Draw(state, runtime) == 0x12345678u &&
              runtime.events == std::vector<std::string>{"forward"} &&
              runtime.forwarded_unused == 0x1111u &&
              runtime.forwarded_depth == 0x2222u &&
              runtime.forwarded_texture == 0x3333u &&
              runtime.forwarded_colours == 0x4444u &&
              state.ReadDword(188u) == 0u,
          "disabled queue delegates without touching records");
}

void EmptyQueueAllocatesAndWritesRecord() {
    pal4::CeguiRendererState state;
    state.WriteByte(260u, 1u);
    Runtime runtime;
    Check(Draw(state, runtime) == 0x1003cu &&
              state.ReadDword(188u) == 0x10000u &&
              state.ReadDword(192u) == 0x1003cu &&
              state.ReadDword(196u) == 0x1003cu &&
              runtime.allocations == std::vector<std::uint32_t>{60u} &&
              runtime.freed == std::vector<std::uint32_t>{0u},
          "empty queue allocates one 60-byte record");
    const auto& record = runtime.Record(0x10000u);
    Check(record.ReadDword(0u) == 0x3333u &&
              record.ReadDword(20u) == 0x2222u &&
              record.ReadDword(56u) == 0x4444u,
          "texture, depth, and colour source retain their 32-bit fields");
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        Check(record.ReadDword(4u + index * 4u) == kRect[index] &&
                  record.ReadDword(24u + index * 4u) == kUv[index],
              "both Rect values occupy observed offsets");
    }
    Check(record.ReadDword(40u) == 0xff3f7f00u &&
              record.ReadDword(44u) == 0x000000ffu &&
              record.ReadDword(48u) == 0x0000ff00u &&
              record.ReadDword(52u) == 0x00ff0000u,
          "four colours retain observed channel packing");
    Check(runtime.events == std::vector<std::string>{
              "rect", "rect", "colour", "colour", "colour", "colour",
              "allocate", "fill", "range", "free"},
          "external copies and growth helpers keep call order");
}

void ExistingCapacityAppendsWithoutAllocation() {
    pal4::CeguiRendererState state;
    state.WriteByte(260u, 1u);
    state.WriteDword(188u, 0x20000u);
    state.WriteDword(192u, 0x20000u);
    state.WriteDword(196u, 0x20078u);
    Runtime runtime;
    Check(Draw(state, runtime) == 0x2003cu &&
              state.ReadDword(192u) == 0x2003cu &&
              runtime.allocations.empty() &&
              runtime.Record(0x20000u).ReadDword(0u) == 0x3333u &&
              runtime.events.back() == "structured",
          "spare capacity writes the record in place");
}

void FullQueueCopiesExistingRecordsAndGrows() {
    pal4::CeguiRendererState state;
    state.WriteByte(260u, 1u);
    state.WriteDword(188u, 0x20000u);
    state.WriteDword(192u, 0x20078u);
    state.WriteDword(196u, 0x20078u);
    Runtime runtime;
    pal4::CeguiRectangleRecord first;
    first.WriteDword(0u, 0xaaaau);
    pal4::CeguiRectangleRecord second;
    second.WriteDword(0u, 0xbbbbu);
    runtime.FillRecord(0x20000u, first);
    runtime.FillRecord(0x2003cu, second);
    runtime.events.clear();
    Check(Draw(state, runtime) == 0x100b4u &&
              state.ReadDword(188u) == 0x10000u &&
              state.ReadDword(192u) == 0x100b4u &&
              state.ReadDword(196u) == 0x100f0u &&
              runtime.allocations == std::vector<std::uint32_t>{240u} &&
              runtime.freed == std::vector<std::uint32_t>{0x20000u} &&
              runtime.Record(0x10000u).ReadDword(0u) == 0xaaaau &&
              runtime.Record(0x1003cu).ReadDword(0u) == 0xbbbbu &&
              runtime.Record(0x10078u).ReadDword(0u) == 0x3333u,
          "full queue doubles capacity and copies old records first");
}

void ClearRewindsEndWithoutReleasingCapacity() {
    pal4::CeguiRendererState state;
    state.WriteDword(188u, 0x20000u);
    state.WriteDword(192u, 0x20078u);
    state.WriteDword(196u, 0x200f0u);
    state.WriteDword(204u, 0xabcdu);
    pal4::CeguiRectangleQueue::Clear(state);
    Check(state.ReadDword(188u) == 0x20000u &&
              state.ReadDword(192u) == 0x20000u &&
              state.ReadDword(196u) == 0x200f0u &&
              state.ReadDword(204u) == 0xabcdu,
          "clear only rewinds 60-byte record end to begin");
}

}  // namespace

int main() {
    DisabledQueueForwardsAllArguments();
    EmptyQueueAllocatesAndWritesRecord();
    ExistingCapacityAppendsWithoutAllocation();
    FullQueueCopiesExistingRecordsAndGrows();
    ClearRewindsEndWithoutReleasingCapacity();
    std::cout << "5 CEGUI rectangle queue test cases passed\n";
}
