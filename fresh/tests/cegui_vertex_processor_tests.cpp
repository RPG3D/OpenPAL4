#include "pal4/cegui_vertex_processor.h"

#include <bit>
#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::ICeguiVertexRuntime {
public:
    std::map<std::uint32_t, pal4::CeguiRectangleRecord> records;
    std::map<std::uint32_t, pal4::CeguiRenderVertex> vertices;
    std::vector<std::string> events;
    std::vector<std::uint32_t> bound_textures;
    std::vector<std::uint32_t> flushed_counts;
    std::vector<std::uint32_t> queried_states;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> set_states;
    bool retain_vertices = true;

    void SetRenderStates() override { events.emplace_back("states"); }
    std::uint32_t GetRenderState(const std::uint32_t state_id) override {
        events.emplace_back("get");
        queried_states.push_back(state_id);
        return 0x100u + state_id;
    }
    std::uint32_t SetRenderState(
        const std::uint32_t state_id,
        const std::uint32_t value) override {
        events.emplace_back("set");
        set_states.emplace_back(state_id, value);
        return 0x9000u + state_id + value;
    }
    float ActiveCameraDepth() override {
        events.emplace_back("camera");
        return 4.0f;
    }
    const pal4::CeguiRectangleRecord& Record(
        const std::uint32_t address) override {
        return records.at(address);
    }
    void WriteVertex(const std::uint32_t address,
                     const pal4::CeguiRenderVertex& vertex) override {
        events.emplace_back("vertex");
        if (retain_vertices) {
            vertices[address] = vertex;
        }
    }
    std::uint32_t Flush(pal4::CeguiRendererState& state) override {
        events.emplace_back("flush");
        flushed_counts.push_back(state.ReadDword(264u));
        state.WriteDword(264u, 0u);
        return 0x55u;
    }
    void BindTexture(const std::uint32_t texture_address) override {
        events.emplace_back("bind");
        bound_textures.push_back(texture_address);
    }
    void FinishNoOp(pal4::CeguiRendererState& /*state*/) override {
        events.emplace_back("finish");
    }
};

void Float(pal4::CeguiRectangleRecord& record,
           const std::uint32_t offset, const float value) {
    record.WriteDword(offset, std::bit_cast<std::uint32_t>(value));
}

pal4::CeguiRectangleRecord MakeRecord(const std::uint32_t texture) {
    pal4::CeguiRectangleRecord record;
    record.WriteDword(0u, texture);
    Float(record, 4u, 10.0f);
    Float(record, 8u, 20.0f);
    Float(record, 12u, 30.0f);
    Float(record, 16u, 40.0f);
    Float(record, 20u, 0.25f);
    Float(record, 24u, 0.125f);
    Float(record, 28u, 0.25f);
    Float(record, 32u, 0.375f);
    Float(record, 36u, 0.5f);
    record.WriteDword(40u, 0x11223344u);
    record.WriteDword(44u, 0x22334455u);
    record.WriteDword(48u, 0x33445566u);
    record.WriteDword(52u, 0x44556677u);
    return record;
}

pal4::CeguiRendererState MakeState(const std::uint32_t count) {
    pal4::CeguiRendererState state;
    state.WriteDword(188u, 0x2000u);
    state.WriteDword(192u, 0x2000u + 60u * count);
    state.WriteDword(204u, 0x9000u);
    state.WriteFloat(248u, 600.0f);
    state.WriteFloat(256u, 800.0f);
    return state;
}

void EmptyQueueStillSetsStateAndFlushes() {
    auto state = MakeState(0u);
    Runtime runtime;
    Check(pal4::CeguiVertexProcessor::ProcessScaled(
              state, 1600, 1200, runtime) == 0x55u &&
              runtime.events == std::vector<std::string>{
                  "states", "camera", "flush", "finish"} &&
              runtime.flushed_counts == std::vector<std::uint32_t>{0u},
          "empty queue runs setup and terminal flush");
}

void OneRecordBuildsFourVertices() {
    auto state = MakeState(1u);
    Runtime runtime;
    runtime.records.emplace(0x2000u, MakeRecord(0x7000u));
    Check(pal4::CeguiVertexProcessor::ProcessScaled(
              state, 1600, 1200, runtime) == 0x55u &&
              runtime.bound_textures == std::vector<std::uint32_t>{0x7000u} &&
              runtime.flushed_counts == std::vector<std::uint32_t>{0u, 4u} &&
              state.ReadDword(264u) == 0u &&
              runtime.vertices.size() == 4u,
          "one rectangle binds once and emits four vertices");
    const auto& first = runtime.vertices.at(0x9000u);
    const auto& second = runtime.vertices.at(0x9000u + 28u);
    const auto& third = runtime.vertices.at(0x9000u + 56u);
    const auto& fourth = runtime.vertices.at(0x9000u + 84u);
    Check(first.x == 59.5f && first.y == 19.5f &&
              first.depth == 0.25f &&
              first.reciprocal_camera_depth == 0.25f &&
              first.colour == 0x11443322u &&
              first.u == 0.375f && first.v == 0.125f,
          "first vertex uses scaled left/top, first colour and UV");
    Check(second.x == 59.5f && second.y == 39.5f &&
              second.colour == 0x33665544u &&
              second.u == 0.375f && second.v == 0.25f &&
              third.x == 79.5f && third.y == 39.5f &&
              third.colour == 0x44776655u &&
              third.u == 0.5f && third.v == 0.25f &&
              fourth.x == 79.5f && fourth.y == 19.5f &&
              fourth.colour == 0x22554433u &&
              fourth.u == 0.5f && fourth.v == 0.125f,
          "remaining vertices follow IDA colour and UV ordering");
}

void TextureChangesFlushBeforeBinding() {
    auto state = MakeState(3u);
    Runtime runtime;
    runtime.records.emplace(0x2000u, MakeRecord(0x7000u));
    runtime.records.emplace(0x203cu, MakeRecord(0x7000u));
    runtime.records.emplace(0x2078u, MakeRecord(0x8000u));
    static_cast<void>(pal4::CeguiVertexProcessor::ProcessScaled(
        state, 800, 600, runtime));
    Check(runtime.bound_textures ==
              std::vector<std::uint32_t>{0x7000u, 0x8000u} &&
              runtime.flushed_counts ==
                  std::vector<std::uint32_t>{0u, 8u, 4u},
          "texture switch flushes the old eight-vertex batch");
}

void VertexLimitFlushesAt4096() {
    auto state = MakeState(1024u);
    Runtime runtime;
    runtime.retain_vertices = false;
    for (std::uint32_t index = 0u; index < 1024u; ++index) {
        runtime.records.emplace(0x2000u + 60u * index,
                                MakeRecord(0x7000u));
    }
    static_cast<void>(pal4::CeguiVertexProcessor::ProcessScaled(
        state, 800, 600, runtime));
    Check(runtime.flushed_counts ==
              std::vector<std::uint32_t>{0u, 4096u, 0u} &&
              state.ReadDword(264u) == 0u,
          "vertex count above 4092 flushes after the 1024th quad");
}

void QueueRecordFeedsVertexProcessor() {
    auto state = MakeState(0u);
    state.WriteDword(188u, 0u);
    state.WriteDword(192u, 0u);
    state.WriteDword(196u, 0u);
    state.WriteByte(260u, 1u);
    pal4::CeguiRectangleQueueStub queue;
    const pal4::CeguiRectWords rect{
        std::bit_cast<std::uint32_t>(10.0f),
        std::bit_cast<std::uint32_t>(20.0f),
        std::bit_cast<std::uint32_t>(30.0f),
        std::bit_cast<std::uint32_t>(40.0f)};
    const pal4::CeguiRectWords uv{
        std::bit_cast<std::uint32_t>(0.125f),
        std::bit_cast<std::uint32_t>(0.25f),
        std::bit_cast<std::uint32_t>(0.375f),
        std::bit_cast<std::uint32_t>(0.5f)};
    const std::array<pal4::CeguiColourWords, 4> colours{{
        {1.0f, 0.0f, 0.5f, 0.25f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.5f, 0.25f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.5f, 0.25f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.5f, 0.25f, 0.0f, 0.0f},
    }};
    static_cast<void>(pal4::CeguiRectangleQueue::Draw(
        state, rect, 0u, std::bit_cast<std::uint32_t>(0.25f),
        0x7000u, uv, colours, 0x8000u, queue));
    pal4::CeguiVertexRuntimeStub vertex_runtime(queue);
    static_cast<void>(pal4::CeguiVertexProcessor::ProcessScaled(
        state, 1600, 1200, vertex_runtime));
    const auto& vertex = vertex_runtime.Vertices().at(0x9000u);
    Check(vertex_runtime.Vertices().size() == 4u &&
              vertex.x == 59.5f && vertex.y == 19.5f &&
              vertex.depth == 0.25f &&
              vertex.colour == 0xff007f3fu &&
              vertex.u == 0.375f && vertex.v == 0.125f,
          "queued rectangle feeds scaled vertices through linkable stubs");
}

void NativeModeRestoresSixRenderStates() {
    auto state = MakeState(0u);
    Runtime runtime;
    constexpr std::uint32_t ids[6]{12u, 6u, 8u, 7u, 9u, 2u};
    constexpr std::uint32_t values[6]{1u, 0u, 0u, 2u, 1u, 3u};
    Check(pal4::CeguiVertexProcessor::ProcessNative(state, runtime) ==
              0x9000u + 2u + 0x102u &&
              runtime.queried_states ==
                  std::vector<std::uint32_t>(std::begin(ids), std::end(ids)) &&
              runtime.set_states.size() == 12u &&
              runtime.events == std::vector<std::string>{
                  "get", "get", "get", "get", "get", "get",
                  "set", "set", "set", "set", "set", "set",
                  "camera", "flush", "finish",
                  "set", "set", "set", "set", "set", "set"},
          "native mode saves state before setup and restores after flush");
    for (std::uint32_t index = 0u; index < 6u; ++index) {
        Check(runtime.set_states[index] ==
                  std::pair<std::uint32_t, std::uint32_t>{ids[index],
                                                           values[index]} &&
                  runtime.set_states[index + 6u] ==
                      std::pair<std::uint32_t, std::uint32_t>{
                          ids[index], 0x100u + ids[index]},
              "native mode keeps exact state id and value order");
    }
}

void NativeModeUsesUnscaledCoordinatesAndSharedBatching() {
    auto state = MakeState(2u);
    Runtime runtime;
    runtime.records.emplace(0x2000u, MakeRecord(0x7000u));
    runtime.records.emplace(0x203cu, MakeRecord(0x8000u));
    static_cast<void>(pal4::CeguiVertexProcessor::ProcessNative(
        state, runtime));
    Check(runtime.vertices.at(0x9000u).x == 29.5f &&
              runtime.vertices.at(0x9000u).y == 9.5f &&
              runtime.vertices.at(0x9000u + 28u).y == 19.5f &&
              runtime.vertices.at(0x9000u + 56u).x == 39.5f &&
              runtime.bound_textures ==
                  std::vector<std::uint32_t>{0x7000u, 0x8000u} &&
              runtime.flushed_counts ==
                  std::vector<std::uint32_t>{0u, 4u, 4u},
          "native mode emits unscaled geometry and flushes on texture change");
}

void RendererScaledModeUsesObjectFactorsAndRestoresStates() {
    auto state = MakeState(1u);
    state.WriteFloat(272u, 1.5f);
    state.WriteFloat(276u, 2.0f);
    Runtime runtime;
    runtime.records.emplace(0x2000u, MakeRecord(0x7000u));
    Check(pal4::CeguiVertexProcessor::ProcessRendererScaled(
              state, runtime) == 0x9104u &&
              runtime.vertices.at(0x9000u).x == 44.5f &&
              runtime.vertices.at(0x9000u).y == 19.5f &&
              runtime.vertices.at(0x9000u + 56u).x == 59.5f &&
              runtime.vertices.at(0x9000u + 56u).y == 39.5f &&
              runtime.flushed_counts ==
                  std::vector<std::uint32_t>{0u, 4u} &&
              runtime.set_states.size() == 6u,
          "renderer-scaled mode uses +272/+276 and restores saved states");
    Check(runtime.events.front() == "get" &&
              runtime.events[6] == "states" &&
              runtime.events[7] == "camera" &&
              runtime.events[runtime.events.size() - 7u] == "finish",
          "renderer-scaled mode initializes after saves and restores after finish");
    constexpr std::uint32_t ids[6]{12u, 6u, 8u, 7u, 9u, 2u};
    for (std::uint32_t index = 0u; index < 6u; ++index) {
        Check(runtime.queried_states[index] == ids[index] &&
                  runtime.set_states[index] ==
                      std::pair<std::uint32_t, std::uint32_t>{
                          ids[index], 0x100u + ids[index]},
              "renderer-scaled mode restores exact state values in order");
    }
}

void WideModeAddsHorizontalBiasWithoutChangingOtherVertexFields() {
    auto state = MakeState(1u);
    state.WriteFloat(272u, 1.25f);
    state.WriteFloat(276u, 2.0f);
    state.WriteFloat(280u, 7.0f);
    Runtime runtime;
    runtime.records.emplace(0x2000u, MakeRecord(0x7000u));
    Check(pal4::CeguiVertexProcessor::ProcessWide(state, runtime) ==
              0x55u &&
              runtime.vertices.at(0x9000u).x == 44.0f &&
              runtime.vertices.at(0x9000u).y == 19.5f &&
              runtime.vertices.at(0x9000u + 56u).x == 56.5f &&
              runtime.vertices.at(0x9000u + 56u).y == 39.5f &&
              runtime.vertices.at(0x9000u).colour == 0x11443322u &&
              runtime.vertices.at(0x9000u).u == 0.375f &&
              runtime.flushed_counts ==
                  std::vector<std::uint32_t>{0u, 4u} &&
              runtime.queried_states.empty() &&
              runtime.set_states.empty(),
          "wide renderer adds +280 horizontal bias and keeps shared fields");
    Check(runtime.events.front() == "states" &&
              runtime.events.back() == "finish",
          "wide renderer sets state and finishes without save/restore");
}

}  // namespace

int main() {
    EmptyQueueStillSetsStateAndFlushes();
    OneRecordBuildsFourVertices();
    TextureChangesFlushBeforeBinding();
    VertexLimitFlushesAt4096();
    QueueRecordFeedsVertexProcessor();
    NativeModeRestoresSixRenderStates();
    NativeModeUsesUnscaledCoordinatesAndSharedBatching();
    RendererScaledModeUsesObjectFactorsAndRestoresStates();
    WideModeAddsHorizontalBiasWithoutChangingOtherVertexFields();
    std::cout << "9 CEGUI vertex processor test cases passed\n";
}
