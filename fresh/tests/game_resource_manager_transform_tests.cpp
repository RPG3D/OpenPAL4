#include "pal4/game_resource_manager.h"

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

float Word(const pal4::GameResourceManager& manager, std::size_t index) {
    return std::bit_cast<float>(manager.words[index]);
}

void Set(pal4::GameResourceManager& manager, std::size_t index, float value) {
    manager.words[index] = std::bit_cast<std::uint32_t>(value);
}

class Runtime final : public pal4::GameResourceManagerRuntimeStub {
public:
    std::uint32_t ClumpFrame(std::uint32_t clump) override {
        Check(clump == 0x700 || clump == 0x701, "clump frame query");
        return clump == 0x700 ? 0x800 : 0x801;
    }
    void FrameSetIdentity(std::uint32_t frame) override {
        Check(frame == 0x800, "main frame identity"); calls.push_back("identity");
    }
    void FrameScale(std::uint32_t frame, const float* value,
                    std::uint32_t combine) override {
        Check(frame == 0x800 && combine <= 2, "frame scale arguments");
        scale_combine = combine;
        scale = {value[0], value[1], value[2]}; calls.push_back("scale");
    }
    void FrameRotate(std::uint32_t frame, std::uint32_t axis,
                     float angle, std::uint32_t combine) override {
        Check(frame == 0x800 && combine <= 2, "frame rotation arguments");
        rotation_combines.push_back(combine);
        axes.push_back(axis); angles.push_back(angle); calls.push_back("rotate");
    }
    void FrameTranslate(std::uint32_t frame, const float* value,
                        std::uint32_t combine) override {
        Check(frame == 0x800 && combine == 2, "frame translation arguments");
        translation = {value[0], value[1], value[2]}; calls.push_back("translate");
    }
    void FrameUpdateObjects(std::uint32_t frame) override {
        Check(frame == 0x800 || frame == 0x801, "frame update target");
        calls.push_back(frame == 0x800 ? "update_main" : "update_secondary");
    }
    std::uint32_t ObjectTransform(std::uint32_t frame) override {
        Check(frame == 0x800, "object transform main frame");
        calls.push_back("object_transform"); return 0x900;
    }
    void ApplyDialogTransform(std::uint32_t frame,
                              std::uint32_t transform,
                              std::uint32_t combine) override {
        Check(frame == 0x801 && transform == 0x900 && combine == 0,
              "secondary transform arguments");
        calls.push_back("secondary");
    }
    void TransformPoints(pal4::ResourceCorners& points,
                         std::uint32_t transform) override {
        Check(transform == 0x900, "corner transform");
        corners = points;
        for (auto& point : points) point.x += 1.0f;
        calls.push_back("points");
    }
    void CalculateBBox(std::uint32_t* bbox,
                       const pal4::ResourceCorners& points) override {
        Check(bbox == manager + 66 && points[0].x == corners[0].x + 1.0f,
              "bbox receives transformed corners");
        bbox[0] = 0x1234;
        calls.push_back("bbox");
    }
    std::uint32_t CalculateCameraPosition(std::uint32_t* dest,
        std::uint32_t* source, std::uint32_t frame) override {
        Check(dest == manager + 56 && source == manager + 52 && frame == 0x800,
              "camera position offsets and frame");
        calls.push_back("camera_position"); return 0xA00;
    }
    void ProcessAtomicBounds(std::uint32_t clump, std::uint32_t mode,
                             std::uint32_t* bbox) override {
        Check(clump == 0x700 &&
              bbox == manager + (mode == 1 ? 66 : 60), "bounds target");
        for (int i = 0; i < 3; ++i)
            Check(bbox[i] == 0xff7fffffu && bbox[i + 3] == 0x7f7fffffu,
                  "bounds finite-extreme initialization");
        calls.push_back(mode == 1 ? "bounds_world" : "bounds_local");
    }
    std::uint32_t ProcessAtomicBounds2(std::uint32_t clump,
                               std::uint32_t first,
                               std::uint32_t second,
                               std::uint32_t* output) override {
        Check(clump == 0x700 && first == 0 && second == 1 &&
              output == manager + 52, "secondary bounds arguments");
        output[3] = 0xABCD;
        calls.push_back("bounds_extra");
        return 0xD00;
    }
    std::uint32_t SetClumpMaterialFlagsForAllAtomics(
        std::uint32_t clump, std::uint32_t mode) override {
        Check(clump == 0x700 && mode == 5, "atomic flags arguments");
        calls.push_back("all_flags"); return 0xE00;
    }

    std::uint32_t* manager{};
    pal4::ResourceVec3 scale, translation;
    pal4::ResourceCorners corners{};
    std::vector<std::uint32_t> axes;
    std::vector<std::uint32_t> rotation_combines;
    std::uint32_t scale_combine{};
    std::vector<float> angles;
    std::vector<std::string> calls;
};

void TransformBranches() {
    pal4::GameResourceManager manager;
    (void)manager.Construct();
    Runtime runtime;
    runtime.manager = manager.words.data();
    Check(manager.UpdateClumpTransform({1, 2, 3}, false, runtime) == 0 &&
          runtime.calls.empty(), "absent clump gates transform");
    manager.words[5] = 0x700;
    Check(manager.UpdateClumpTransform({1, 2, 3}, false, runtime) == 0xA00 &&
          Word(manager, 43) == 1 && Word(manager, 44) == 2 &&
          Word(manager, 45) == 3 &&
          runtime.calls == std::vector<std::string>{
              "identity", "scale", "rotate", "rotate", "rotate",
              "translate", "camera_position"},
          "absolute transform order without optional paths");
    Check(runtime.axes == std::vector<std::uint32_t>{
              0x84b870u, 0x84b87cu, 0x84b888u} &&
          runtime.translation.x == 1 && runtime.translation.y == 2 &&
          runtime.translation.z == 3, "rotation axes and translation");
    runtime.calls.clear();
    manager.words[12] = 0x701;
    manager.words[8] = 1;
    Set(manager, 60, 10); Set(manager, 61, 20); Set(manager, 62, 30);
    Set(manager, 63, 1); Set(manager, 64, 2); Set(manager, 65, 3);
    Check(manager.UpdateClumpTransform({2, -1, 4}, true, runtime) == 0xA00 &&
          Word(manager, 43) == 3 && Word(manager, 44) == 1 &&
          Word(manager, 45) == 7 && manager.words[66] == 0x1234 &&
          runtime.corners[0].x == 1 && runtime.corners[0].y == 2 &&
          runtime.corners[0].z == 3 && runtime.corners[7].x == 10 &&
          runtime.corners[7].y == 20 && runtime.corners[7].z == 30,
          "relative translation and all eight bbox corners");
    Check(runtime.calls == std::vector<std::string>{
              "identity", "scale", "rotate", "rotate", "rotate",
              "translate", "object_transform", "secondary",
              "update_secondary", "object_transform", "points", "bbox",
              "camera_position"}, "secondary and bbox call order");
}

void RotationBranches() {
    pal4::GameResourceManager manager;
    (void)manager.Construct();
    Runtime runtime;
    runtime.manager = manager.words.data();
    manager.UpdateClumpRotation({370, -10, -370}, false, runtime);
    Check(runtime.calls.empty(), "absent clump gates rotation");
    manager.words[5] = 0x700;
    manager.UpdateClumpRotation({370, -10, -370}, false, runtime);
    Check(Word(manager, 40) == 10 && Word(manager, 41) == 350 &&
          Word(manager, 42) == 350 &&
          runtime.calls == std::vector<std::string>{
              "identity", "scale", "rotate", "rotate", "rotate",
              "translate", "update_main"},
          "absolute angles wrap into a turn and update main frame");
    runtime.calls.clear();
    manager.UpdateClumpRotation({360, 10, 20}, true, runtime);
    Check(Word(manager, 40) == 10 && Word(manager, 41) == 360 &&
          Word(manager, 42) == 10, "relative angles wrap independently");
    manager.UpdateClumpRotation({360, 360, 360}, false, runtime);
    Check(manager.words[40] == 0x43b40000u &&
          manager.words[41] == 0x43b40000u &&
          manager.words[42] == 0x43b40000u,
          "exact positive turn preserves original serialized float bits");
}

void ScaleBranches() {
    pal4::GameResourceManager manager;
    (void)manager.Construct();
    Runtime runtime;
    runtime.manager = manager.words.data();
    manager.UpdateClumpPosition({1, 2, 3}, false, runtime);
    Check(runtime.calls.empty(), "absent clump gates scale");
    manager.words[5] = 0x700;
    manager.UpdateClumpPosition({-1, 0, -2}, false, runtime);
    Check(runtime.calls.empty() && Word(manager, 46) == -1 &&
          Word(manager, 47) == 0 && Word(manager, 48) == -2,
          "nonpositive scale stores values but skips frame and bounds");
    manager.words[8] = 1;
    manager.UpdateClumpPosition({2, 3, 4}, true, runtime);
    Check(Word(manager, 46) == 1 && Word(manager, 47) == 3 &&
          Word(manager, 48) == 2 && manager.words[59] == 0xABCD &&
          runtime.scale.x == 1 && runtime.scale.y == 3 &&
          runtime.scale.z == 2 &&
          runtime.calls == std::vector<std::string>{
              "identity", "scale", "rotate", "rotate", "rotate",
              "translate", "bounds_world", "bounds_local", "bounds_extra"},
          "positive relative scale rebuilds frame and bounds in order");
}

void UniformScaleAndReset() {
    pal4::GameResourceManager manager;
    (void)manager.Construct();
    Runtime runtime;
    runtime.manager = manager.words.data();
    Check(manager.UpdateClumpScale(2, false, runtime) == 0 &&
          runtime.calls.empty(), "uniform scale requires clump");
    manager.words[5] = 0x700;
    Check(manager.UpdateClumpScale(0, false, runtime) == 0 &&
          manager.UpdateClumpScale(-1, false, runtime) == 0 &&
          runtime.calls.empty(), "uniform scale rejects nonpositive input");
    Set(manager, 46, -10); Set(manager, 47, -20); Set(manager, 48, -30);
    Check(manager.UpdateClumpScale(2, true, runtime) == 0xABCD &&
          Word(manager, 46) == -8 && Word(manager, 47) == -18 &&
          Word(manager, 48) == -28 && !runtime.calls.empty(),
          "positive increment refreshes even when resulting scale is negative");
    runtime.calls.clear();
    runtime.rotation_combines.clear();
    manager.words[8] = 1;
    manager.words[12] = 0x701;
    Check(manager.ResetClumpTransform(runtime) == 0xD00 &&
          Word(manager, 46) == 1 && Word(manager, 47) == 1 &&
          Word(manager, 48) == 1 && runtime.scale_combine == 2 &&
          runtime.rotation_combines ==
              std::vector<std::uint32_t>{0, 2, 2} &&
          runtime.calls == std::vector<std::string>{
              "identity", "rotate", "rotate", "rotate", "scale",
              "translate", "object_transform", "secondary",
              "update_secondary", "bounds_world", "bounds_local",
              "bounds_extra"},
          "reset uses rotation-before-scale and returns bounds callback");
}

void Setters() {
    pal4::GameResourceManager manager;
    (void)manager.Construct();
    Runtime runtime;
    Check(manager.SetClumpMaterialFlagsForAllAtomics(5, runtime) == 0 &&
          runtime.calls.empty(), "atomic flags gate absent clump");
    manager.words[5] = 0x700;
    Check(manager.SetClumpMaterialFlagsForAllAtomics(5, runtime) == 0xE00 &&
          runtime.calls == std::vector<std::string>{"all_flags"},
          "atomic flags forward return");
    Check(manager.SetCurrentClump(0x1234) == 0x1234 &&
          manager.words[72] == 0x1234 &&
          manager.SetCurrentClump(0) == 0 && manager.words[72] == 0,
          "current clump setter writes word 72 and forwards value");
}

}  // namespace

int main() {
    TransformBranches();
    RotationBranches();
    ScaleBranches();
    UniformScaleAndReset();
    Setters();
}
