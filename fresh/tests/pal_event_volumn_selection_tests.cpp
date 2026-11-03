#include "pal4/pal_event_volumn_manager.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool good, const char* message) {
    if (!good) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct SelectionRuntime final : pal4::IPalEventVolumnSelectionRuntime,
                                pal4::ISceneEvfColorRuntime,
                                pal4::ISceneEvfIntersectionRuntime {
    std::map<std::uint32_t, pal4::SceneEvfRecordState> records;
    std::map<std::uint32_t, std::uint32_t> slots;
    std::map<std::uint32_t, float> hit_distance;
    std::vector<std::string> calls;
    bool miss_all = false;

    std::uint32_t ReadRecordPointer(std::uint32_t slot) override {
        return slots.at(slot);
    }
    pal4::SceneEvfRecordState& RecordAt(std::uint32_t address) override {
        return records.at(address);
    }
    pal4::ISceneEvfColorRuntime& Colors() override { return *this; }
    pal4::ISceneEvfIntersectionRuntime& Intersections() override {
        return *this;
    }
    void WriteVertexColor(std::uint32_t address, std::uint32_t index,
                          std::uint32_t argb) override {
        Check(index == 0u, "one vertex per test record");
        calls.push_back("color:" + std::to_string(address) + ":" +
                        std::to_string(argb));
    }
    void CalculatePixelRay(std::uint32_t camera, std::uint32_t pixel,
                           pal4::SceneEvfVec3& start,
                           pal4::SceneEvfVec3& end) override {
        Check(camera == 0x100u && pixel == 0x200u,
              "camera and pixel forwarded");
        start = {0.0f, 0.0f, 0.0f};
        end = {1.0f, 0.0f, 0.0f};
    }
    void TransformCameraRay(std::uint32_t,
                            pal4::SceneEvfVec3&,
                            pal4::SceneEvfVec3&) override {}
    std::array<std::uint32_t, 3u> ReadTriangleIndices(
        std::uint32_t address, std::uint32_t) override {
        calls.push_back("trace:" + std::to_string(address));
        return {address, address, address};
    }
    pal4::SceneEvfVec3 ReadPropertyPosition(
        std::uint32_t, std::uint32_t index) override {
        return {static_cast<float>(index), 0.0f, 0.0f};
    }
    void TransformTriangle(std::uint32_t,
                           std::array<pal4::SceneEvfVec3, 3u>&) override {}
    bool IntersectLine(const pal4::SceneEvfVec3&,
                       const pal4::SceneEvfVec3&,
                       const std::array<pal4::SceneEvfVec3, 3u>& points,
                       pal4::SceneEvfVec3& hit) override {
        const auto address = static_cast<std::uint32_t>(points[0u][0u]);
        if (miss_all) return false;
        hit = {hit_distance.at(address), 0.0f, 0.0f};
        return true;
    }
    float Length(const pal4::SceneEvfVec3& vector) override {
        return std::abs(vector[0u]);
    }
};

}  // namespace

int main() {
    pal4::PalEventVolumnState state;
    state.words[1u] = 0x4000u;
    state.words[2u] = 0x400cu;
    state.words[4u] = 0xdeadu;
    SelectionRuntime runtime;
    for (std::uint32_t i = 0u; i < 3u; ++i) {
        const auto address = 0x5000u + i * 0x1000u;
        runtime.slots[0x4000u + 4u * i] = address;
        auto& record = runtime.records[address];
        record.words[19u] = i == 0u ? 0u : 1u;
        record.words[32u] = 1u;
        record.words[31u] = 0x9000u + 0x1000u * i;
        record.words[41u] = address;
        record.words[42u] = 1u;
        record.words[34u] = 0x44332211u;
        record.words[35u] = 0xddccbbaau;
    }
    runtime.hit_distance = {{0x5000u, 1.0f},
                            {0x6000u, 5.0f},
                            {0x7000u, 2.0f}};
    Check(pal4::PalEventVolumnManager::PickAtPixel(
              state, 0x100u, 0x200u, false, runtime) == 0x7000u &&
              state.words[4u] == 0x7000u &&
              runtime.calls == std::vector<std::string>{
                  "color:36864:1141973555", "color:40960:1141973555",
                  "trace:24576", "color:45056:1141973555",
                  "trace:28672", "color:45056:3718953932"},
          "inactive record is recolored but skipped and nearest active hit wins");
    runtime.calls.clear();
    Check(pal4::PalEventVolumnManager::PickAtPixel(
              state, 0x100u, 0x200u, true, runtime) == 0x5000u &&
              state.words[4u] == 0x5000u &&
              runtime.calls.size() == 7u,
          "include-inactive mode can choose closest inactive record");
    runtime.calls.clear();
    runtime.miss_all = true;
    Check(pal4::PalEventVolumnManager::PickAtPixel(
              state, 0x100u, 0x200u, true, runtime) == 0u &&
              state.words[4u] == 0x5000u &&
              runtime.calls.size() == 6u,
          "no hit preserves previously selected record");
}
