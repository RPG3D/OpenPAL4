#include "pal4/track_task.h"

#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
struct Runtime final : pal4::ITrackTaskConfigRuntime {
    std::vector<std::string> keys;
    std::vector<std::uint32_t> children{0x101u, 0x102u, 0x103u, 0x104u};
    int inherit = 0;
    int result = 0;
    int sample = 0;
    int coefficient = 4;
    int type = 2;
    int loop = 0;
    int autoface = 0;
    float track_time = 5.0f;
    float min_time = 0.0f;
    std::vector<float> sampled_times;
    unsigned spline_creates = 0u;
    unsigned spline_destroys = 0u;
    unsigned synchronized = 0u;
    bool fail_sample = false;

    std::vector<std::uint32_t> Children(std::uint32_t config,
                                       std::string_view name) override {
        Check(config == 0x5000u && name == "Attr_Task_Track_Position",
              "ordered point children");
        keys.emplace_back("children:" + std::string(name));
        return children;
    }
    int ReadInt(std::uint32_t node, std::string_view key,
                int fallback) override {
        Check(node == 0x5000u, "integer root attribute");
        keys.emplace_back(key);
        if (key == "Attr_Inherit_Pre_Task") {
            Check(fallback == 0, "inherit fallback"); return inherit;
        }
        if (key == "Attr_Task_Track_Result") {
            Check(fallback == 0, "result fallback"); return result;
        }
        if (key == "Attr_Task_Track_Sample") {
            Check(fallback == 1, "sample fallback"); return sample;
        }
        if (key == "Attr_Task_Track_Sample_Coeff") {
            Check(fallback == 4, "coefficient fallback"); return coefficient;
        }
        if (key == "Attr_Task_Track_Type") {
            Check(fallback == 2, "type fallback"); return type;
        }
        Check(key == "Attr_Task_Track_Autoface" && fallback == 0,
              "autoface fallback");
        return autoface;
    }
    int ReadTreeInt(std::uint32_t node, std::string_view key,
                    int fallback) override {
        Check(node == 0x5000u && key == "Attr_Task_Track_Loop" &&
              fallback == 0, "loop uses tree lookup with zero fallback");
        keys.emplace_back("tree:" + std::string(key)); return loop;
    }
    float ReadFloat(std::uint32_t node, std::string_view key,
                    float fallback) override {
        keys.emplace_back(key);
        if (node != 0x5000u) {
            Check(node >= 0x101u && node <= 0x104u && fallback == 0.0f,
                  "point attribute and zero fallback");
            const auto index = float(node - 0x101u);
            if (key == "Attr_Task_Track_Position_x") return index;
            if (key == "Attr_Task_Track_Position_y") return index + 10.0f;
            Check(key == "Attr_Task_Track_Position_z", "point z key");
            return index + 20.0f;
        }
        if (key == "Attr_Task_Track_Time") {
            Check(fallback == 5.0f, "track time fallback");
            return track_time;
        }
        Check(key == "Attr_Task_Track_Min_Time" && fallback == 0.0f,
              "minimum time fallback");
        return min_time;
    }
    std::uint32_t CreateSpline(const pal4::TrackPointArray& points,
                                int spline_type) override {
        Check(points.Size() == 4u && spline_type == type,
              "RenderWare spline creation parameters");
        keys.emplace_back("spline-create"); ++spline_creates;
        return 0x9000u;
    }
    pal4::TrackPoint SampleSpline(std::uint32_t spline, int mode,
                                  float time) override {
        Check(spline == 0x9000u && mode == 10,
              "RenderWare spline sample mode");
        keys.emplace_back("spline-sample");
        if (fail_sample && sampled_times.size() == 2u)
            throw std::runtime_error("sample failed");
        sampled_times.push_back(time);
        return {time, 2.0f * time, 3.0f * time};
    }
    void DestroySpline(std::uint32_t spline) override {
        Check(spline == 0x9000u, "RenderWare spline destruction");
        keys.emplace_back("spline-destroy"); ++spline_destroys;
    }
    void SynchronizePointStorage(pal4::TrackTaskState& state,
                                 const pal4::TrackPointArray& points) override {
        keys.emplace_back("synchronize"); ++synchronized;
        state.words[24u] = 0x1000u;
        state.words[25u] = 0x1000u +
            static_cast<std::uint32_t>(points.Size() * 12u);
        state.words[26u] = 0x1000u +
            static_cast<std::uint32_t>(points.Capacity() * 12u);
    }
};
const std::vector<std::string> kRootSuffix{
    "Attr_Task_Track_Time", "Attr_Task_Track_Min_Time",
    "Attr_Task_Track_Result", "Attr_Task_Track_Sample",
    "Attr_Task_Track_Sample_Coeff", "Attr_Task_Track_Type"};
}  // namespace

int main() {
    pal4::TrackTaskState state{};
    state.words.fill(0xabcdef01u);
    pal4::TrackPointArray points;
    Runtime runtime;
    Check(!pal4::TrackTask::LoadConfig(state, 0u, points, runtime) &&
          state.words[15u] == 0xabcdef01u && runtime.keys.empty(),
          "null config preserves state and points");

    runtime.children.resize(3u);
    bool caught = false;
    try { static_cast<void>(pal4::TrackTask::LoadConfig(
        state, 0x5000u, points, runtime)); }
    catch (const std::runtime_error& error) {
        caught = std::string(error.what()) ==
            "create track task failed,because the track points is less than 4...";
    }
    Check(caught && points.Size() == 0u &&
          ((state.words[15u] >> 8u) & 0xffu) == 0u &&
          state.words[33u] == 0xabcdef01u,
          "fewer than four points throws before timing fields");

    runtime = Runtime{};
    state.words.fill(0xabcdef01u);
    Check(pal4::TrackTask::LoadConfig(state, 0x5000u, points, runtime) &&
          points.Size() == 5u && points.At(0u) == points.At(4u) &&
          points.At(3u) == pal4::TrackPoint{3.0f, 13.0f, 23.0f},
          "unsampled type two appends the first point");
    Check(std::bit_cast<float>(state.words[33u]) == 5.0f &&
          state.words[34u] == 0u &&
          ((state.words[18u] >> 16u) & 0xffu) == 0u &&
          (state.words[18u] & 0xffffu) == 0u &&
          runtime.synchronized == 1u && state.words[24u] == 0x1000u &&
          state.words[25u] == 0x1000u + 60u,
          "timing defaults, flags and structured point storage");
    Check(runtime.keys.front() == "Attr_Inherit_Pre_Task" &&
          runtime.keys[1] == "children:Attr_Task_Track_Position" &&
          runtime.keys[runtime.keys.size() - 2u] ==
              "tree:Attr_Task_Track_Loop" &&
          runtime.keys.back() == "Attr_Task_Track_Autoface",
          "root keys and tree lookup follow IDA order");

    runtime = Runtime{};
    runtime.sample = 1;
    runtime.result = 2;
    runtime.inherit = 3;
    runtime.loop = 4;
    runtime.autoface = 5;
    runtime.min_time = 2.0f;
    runtime.track_time = 4.0f;
    points.ClearAndRelease();
    state.words.fill(0u);
    Check(pal4::TrackTask::LoadConfig(state, 0x5000u, points, runtime) &&
          points.Size() == 20u && runtime.spline_creates == 1u &&
          runtime.spline_destroys == 1u &&
          runtime.sampled_times.size() == 20u &&
          runtime.sampled_times.front() == 0.0f &&
          runtime.sampled_times.back() == 1.0f &&
          points.At(0u) == pal4::TrackPoint{0.0f, 0.0f, 0.0f} &&
          points.At(19u) == pal4::TrackPoint{1.0f, 2.0f, 3.0f},
          "sampled path uses count times coefficient plus one");
    Check(((state.words[15u] >> 8u) & 0xffu) == 1u &&
          (state.words[18u] & 0xffffffu) == 0x10101u &&
          std::bit_cast<float>(state.words[33u]) == 4.0f &&
          std::bit_cast<float>(state.words[34u]) == 2.0f,
          "positive inherit, loop, face and result flags");

    runtime = Runtime{};
    runtime.sample = 1;
    runtime.fail_sample = true;
    points.ClearAndRelease();
    caught = false;
    try { static_cast<void>(pal4::TrackTask::LoadConfig(
        state, 0x5000u, points, runtime)); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && runtime.spline_creates == 1u &&
          runtime.spline_destroys == 1u && points.Size() == 2u,
          "sample failure destroys spline and preserves inserted prefix");

    runtime = Runtime{};
    runtime.type = 1;
    points.ClearAndRelease();
    Check(pal4::TrackTask::LoadConfig(state, 0x5000u, points, runtime) &&
          points.Size() == 4u && points.At(3u)[0] == 3.0f,
          "unsampled non-closed type keeps only source points");
}
