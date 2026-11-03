#include "pal4/track_task.h"

#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {
void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
float Read(const pal4::TrackTaskState& state, std::size_t word) {
    return std::bit_cast<float>(state.words[word]);
}
void Write(pal4::TrackTaskState& state, std::size_t word, float value) {
    state.words[word] = std::bit_cast<std::uint32_t>(value);
}
bool Close(float a, float b) { return std::fabs(a - b) < 0.0001f; }
struct Runtime final : pal4::ITrackTaskRuntime {
    std::vector<std::string> calls;
    std::vector<std::size_t> faces;
    std::vector<pal4::TrackPoint> positions;
    std::uint32_t freed = 0xffffffffu;
    void ConstructBase(pal4::TrackTaskState& state) override {
        calls.emplace_back("construct"); state.words[1u] = 0x1234u;
    }
    void DestroyBase(pal4::TrackTaskState& state) override {
        Check(state.words[0u] == pal4::TrackTask::kVtable,
              "derived vtable visible to base cleanup");
        calls.emplace_back("destroy");
    }
    void FreeNativePointBuffer(std::uint32_t address) override {
        freed = address; calls.emplace_back("free");
    }
    std::uint8_t ConstructorStackByte() override { return 0x5au; }
    void TransformPoints(std::uint32_t owner_matrix,
                         pal4::TrackPointArray& points) override {
        Check(owner_matrix == 0x7000u, "owner transform source");
        calls.emplace_back("transform");
        for (auto& point : points.MutablePoints()) point[0] += 1.0f;
    }
    pal4::TrackPoint Normalize(const pal4::TrackPoint& point) override {
        calls.emplace_back("normalize");
        const auto length = std::sqrt(point[0] * point[0] +
                                      point[1] * point[1] +
                                      point[2] * point[2]);
        if (length == 0.0f) return {};
        return {point[0] / length, point[1] / length, point[2] / length};
    }
    std::uint32_t ResolveFacet(std::uint32_t owner,
                               std::string_view name) override {
        Check(owner == 0x5000u && name == "TrackTask", "native facet lookup");
        calls.emplace_back("resolve"); return 0x6000u;
    }
    void SetPosition(std::uint32_t facet,
                     const pal4::TrackPoint& point) override {
        Check(facet == 0x6000u, "native position facet");
        calls.emplace_back("position"); positions.push_back(point);
    }
    void FacePoint(std::uint32_t facet, const pal4::TrackPointArray& points,
                   std::size_t index) override {
        Check(facet == 0x6000u && points.Size() >= 4u, "native face facet");
        calls.emplace_back("face"); faces.push_back(index);
    }
};
pal4::TrackPointArray MakePoints() {
    pal4::TrackPointArray points;
    for (float x : {0.0f, 1.0f, 2.0f, 3.0f})
        static_cast<void>(points.InsertOne(points.Size(), {x, 0.0f, 0.0f}));
    return points;
}
}  // namespace

int main() {
    pal4::TrackTaskState state{};
    state.words.fill(0xdeadbeefu);
    Runtime runtime;
    Check(&pal4::TrackTask::Construct(state, runtime) == &state &&
          sizeof(state) == 144u && state.words[0u] == 0x84b030u &&
          state.words[1u] == 0x1234u &&
          ((state.words[23u] >> 0u) & 0xffu) == 0x5au &&
          state.words[24u] == 0u && state.words[25u] == 0u &&
          state.words[26u] == 0u && state.words[35u] == 0u &&
          pal4::TrackTask::ClassName() == "TrackTask",
          "native constructor, size, fields and class name");

    auto points = MakePoints();
    state.words[16u] = 0x5000u;
    state.words[17u] = 0x7000u;
    Write(state, 33u, 3.0f);
    Write(state, 34u, 0.0f);
    state.words[18u] = 0x10100u;  // autoface + result
    runtime.calls.clear();
    Check(pal4::TrackTask::Start(state, points, runtime) == 1u &&
          points.At(0u)[0] == 1.0f && points.At(3u)[0] == 4.0f &&
          Close(Read(state, 19u), 1.0f) && Read(state, 20u) == 0.0f &&
          Close(Read(state, 30u), 1.0f) && Close(Read(state, 27u), 1.0f) &&
          state.words[22u] == 1u && state.words[35u] == 0x6000u &&
          state.words[21u] == 0u && runtime.faces == std::vector<std::size_t>{1u},
          "start transforms, measures, resolves and faces first segment");
    Check(runtime.calls == std::vector<std::string>{
        "transform", "resolve", "position", "normalize", "face"},
        "start dispatch order");

    runtime.calls.clear();
    Check(pal4::TrackTask::Update(state, 0.25f, points, runtime) == 1u &&
          Close(Read(state, 30u), 1.25f) &&
          Close(Read(state, 21u), 0.25f) && state.words[22u] == 1u &&
          runtime.faces.back() == 1u,
          "update integrates displacement within a segment");
    Check(pal4::TrackTask::Update(state, 0.75f, points, runtime) == 1u &&
          Close(Read(state, 30u), 2.0f) && state.words[22u] == 2u &&
          Close(Read(state, 21u), 1.0f) && runtime.faces.back() == 2u,
          "waypoint transition retains elapsed time");
    Check(pal4::TrackTask::Update(state, 2.5f, points, runtime) == 1u &&
          state.words[22u] == 3u && Close(Read(state, 30u), 3.0f),
          "overshoot advances one segment only");
    Check(pal4::TrackTask::Update(state, 1.0f, points, runtime) == 1u &&
          state.words[22u] == 4u && runtime.faces.back() == 4u,
          "nonloop completion exposes one-past-end face index");
    const auto position_count = runtime.positions.size();
    Check(pal4::TrackTask::Update(state, 1.0f, points, runtime) == 1u &&
          runtime.positions.size() == position_count,
          "completed nonloop update is inert");

    state.words[18u] |= 1u;  // loop at byte +72
    state.words[22u] = 3u;
    Write(state, 30u, 3.0f);
    Write(state, 21u, 5.0f);
    Write(state, 19u, 1.0f);
    Write(state, 20u, 0.0f);
    Check(pal4::TrackTask::Update(state, 1.0f, points, runtime) == 1u &&
          state.words[22u] == 1u && state.words[21u] == 0u &&
          Close(Read(state, 30u), 1.0f) && runtime.faces.back() == 1u,
          "loop completion resets index, position, direction and elapsed");
    Check(pal4::TrackTask::Stop(state) == 1u &&
          (state.words[18u] & 0xffu) == 0u &&
          ((state.words[18u] >> 8u) & 0xffu) == 1u,
          "stop clears loop flag alone");

    Write(state, 34u, 1.0f);
    Check(pal4::TrackTask::Start(state, points, runtime) == 1u &&
          Close(Read(state, 19u), 1.0f) &&
          Close(Read(state, 20u), 4.0f / 3.0f),
          "result acceleration uses final speed branch");
    state.words[18u] &= ~(0xffu << 16u);
    Check(pal4::TrackTask::Start(state, points, runtime) == 1u &&
          Close(Read(state, 20u), 4.0f),
          "ordinary acceleration uses minimum time branch");

    state.words[24u] = 0x8000u;
    runtime.calls.clear();
    Check(&pal4::TrackTask::Destroy(state, points, runtime) == &state &&
          runtime.freed == 0x8000u && points.Size() == 0u &&
          state.words[24u] == 0u && state.words[25u] == 0u &&
          state.words[26u] == 0u && state.words[0u] == 0x84b030u &&
          runtime.calls == std::vector<std::string>{"free", "destroy"},
          "destruction frees point storage before base cleanup");
}
