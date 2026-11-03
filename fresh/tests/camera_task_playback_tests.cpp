#include "pal4/camera_task_object.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICameraTaskPlaybackRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    static constexpr std::uint32_t camera = 0x2000u;
    static constexpr std::uint32_t record = 0x3000u;
    static constexpr std::uint32_t child = 0x4000u;
    static constexpr std::uint32_t document = 0x5000u;

    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::string, float> config;
    std::vector<std::string> calls;
    std::vector<std::string> config_keys;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;
    pal4::CameraTaskVector look_at{};
    pal4::CameraTaskVector position{};
    pal4::CameraTaskVector eye{};
    float scalar_value{};
    float angle_value{};
    float second_angle_value{};
    std::vector<std::pair<float, float>> interpolations;

    Runtime() {
        words[object + 0x48u] = camera;
        words[object + 0x4cu] = record;
        words[record + 0x40u] = document;
        words[record + 0x30u] = 9u;
        words[record + 0x38u] = 0u;
        words[record + 0x24u] = 0u;
        for (std::uint32_t offset = 0u; offset < 12u; offset += 4u) {
            words[camera + 0x74u + offset] =
                std::bit_cast<std::uint32_t>(offset == 0u ? 10.0f : 0.0f);
            words[camera + 0x68u + offset] = 0u;
            words[record + 0x0cu + offset] =
                std::bit_cast<std::uint32_t>(1.0f + offset / 4u);
        }
        words[camera + 0x40u] = std::bit_cast<std::uint32_t>(10.0f);
        words[camera + 0x3cu] = std::bit_cast<std::uint32_t>(20.0f);
        words[camera + 0x44u] = std::bit_cast<std::uint32_t>(30.0f);
        words[camera + 0x48u] = std::bit_cast<std::uint32_t>(15.0f);
        words[record + 0x18u] = std::bit_cast<std::uint32_t>(40.0f);
        words[record + 0x1cu] = std::bit_cast<std::uint32_t>(50.0f);
        words[record + 0x20u] = std::bit_cast<std::uint32_t>(60.0f);
        words[record + 0x2cu] = std::bit_cast<std::uint32_t>(70.0f);
        words[object + 0x44u] = std::bit_cast<std::uint32_t>(2.0f);
        config = {{"SCLD_Extend_eye_x", 0.0f},
                  {"SCLD_Extend_eye_y", 0.0f},
                  {"SCLD_Extend_eye_z", 0.0f},
                  {"SCLD_Extend_lookat_x", 0.0f},
                  {"SCLD_Extend_lookat_y", 0.0f},
                  {"SCLD_Extend_lookat_z", 10.0f}};
    }

    void BaseConstruct(std::uint32_t) override {}
    std::uint32_t BaseDestruct(std::uint32_t) override { return 0u; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        writes.emplace_back(address, value);
        calls.emplace_back("write " + std::to_string(address));
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("byte " + std::to_string(address));
    }
    std::uint32_t FindXmlChild(std::uint32_t source,
                                std::string_view name) override {
        Check(source == document && name == "SCLD_Extend",
              "XML document and extension child");
        calls.emplace_back("xml child");
        return child;
    }
    std::optional<float> ConfigFloat(std::uint32_t source,
                                      std::string_view name) override {
        Check(source == child, "XML configuration child");
        config_keys.emplace_back(name);
        const auto found = config.find(std::string(name));
        if (found == config.end()) return std::nullopt;
        return found->second;
    }
    void SetLookAt(std::uint32_t target, pal4::CameraTaskVector position,
                   std::uint32_t flag) override {
        Check(target == camera && flag == 9u, "look-at call arguments");
        look_at = position;
        calls.emplace_back("look at");
    }
    void SetSecondAngle(std::uint32_t target, float angle,
                        std::uint32_t flag) override {
        Check(target == camera, "second angle camera");
        second_angle_value = angle;
        calls.emplace_back("second angle");
    }
    void SetAngle(std::uint32_t target, float angle,
                  std::uint32_t flag) override {
        Check(target == camera, "angle camera");
        angle_value = angle;
        calls.emplace_back("angle");
    }
    void SetThirdAngle(std::uint32_t target, float angle,
                       std::uint32_t flag) override {
        Check(target == camera && angle == 60.0f && flag == 9u,
              "third angle arguments");
        calls.emplace_back("third angle");
    }
    void SetScalar(std::uint32_t target, float scalar,
                   std::uint32_t flag) override {
        Check(target == camera, "scalar camera");
        scalar_value = scalar;
        calls.emplace_back("scalar");
    }
    float InterpolateFloat(float start, float end, float fraction,
                            std::uint32_t mode) override {
        Check(mode == 0u, "linear interpolation mode");
        interpolations.emplace_back(start, end);
        calls.emplace_back("interpolate");
        return start + (end - start) * fraction;
    }
    void SetPosition(std::uint32_t target,
                      pal4::CameraTaskVector value,
                      std::uint32_t flag) override {
        Check(target == camera && flag == 9u,
              "anchored position arguments");
        position = value;
        calls.emplace_back("position");
    }
    void SetEye(std::uint32_t target, pal4::CameraTaskVector value,
                 std::uint32_t flag) override {
        Check(target == camera && flag == 0u,
              "free eye arguments");
        eye = value;
        calls.emplace_back("eye");
    }
};

struct Geometry final : pal4::ICameraTaskGeometry {
    std::vector<std::string> calls;
    std::vector<pal4::CameraTaskVector> directions;
    std::vector<pal4::CameraTaskBasis> bases;
    pal4::CameraTaskVector initial_euler{};
    pal4::CameraTaskVector final_euler{};

    void ClampDirection(pal4::CameraTaskVector& direction) override {
        calls.emplace_back("clamp");
        directions.push_back(direction);
    }
    void Normalize(pal4::CameraTaskVector&) override {
        calls.emplace_back("normalize");
    }
    pal4::CameraTaskQuaternion ToQuaternion(
        const pal4::CameraTaskBasis& basis) override {
        calls.emplace_back("quaternion");
        bases.push_back(basis);
        return {static_cast<float>(bases.size()), 0.0f, 0.0f, 0.0f};
    }
    pal4::CameraTaskVector ToEuler(
        const pal4::CameraTaskQuaternion& quaternion) override {
        calls.emplace_back("euler");
        return quaternion.x == 1.0f ? initial_euler : final_euler;
    }
};

float WordAsFloat(const Runtime& runtime, std::uint32_t address) {
    return std::bit_cast<float>(runtime.words.at(address));
}

void CheckNoCamera() {
    Runtime runtime;
    Geometry geometry;
    runtime.words[Runtime::object + 0x48u] = 0u;
    Check(pal4::CameraTaskObject::Enter(Runtime::object, runtime, geometry) ==
                  0u &&
              runtime.calls.empty() && runtime.writes.empty() &&
              geometry.calls.empty(),
          "null camera exits before XML or field writes");
}

void CheckImmediateAndOtherModes() {
    Runtime runtime;
    Geometry geometry;
    runtime.words[Runtime::record + 0x38u] = 1u;
    runtime.config.erase("SCLD_Extend_eye_y");
    Check(pal4::CameraTaskObject::Enter(Runtime::object, runtime, geometry) ==
                  1u &&
              runtime.words.at(Runtime::object + 0x50u) ==
                  runtime.words.at(Runtime::camera + 0x74u) &&
              WordAsFloat(runtime, Runtime::object + 0x5cu) == 10.0f &&
              WordAsFloat(runtime, Runtime::object + 0x60u) == 20.0f &&
              WordAsFloat(runtime, Runtime::object + 0x64u) == 30.0f &&
              WordAsFloat(runtime, Runtime::object + 0x78u) == 0.0f &&
              WordAsFloat(runtime, Runtime::object + 0x80u) == 15.0f &&
              runtime.words.at(Runtime::object + 0x84u) == 9u &&
              runtime.look_at.x == 1.0f && runtime.look_at.y == 2.0f &&
              runtime.look_at.z == 3.0f &&
              runtime.second_angle_value == 40.0f &&
              runtime.angle_value == 50.0f &&
              runtime.scalar_value == 70.0f &&
              runtime.words.at(Runtime::object + 0x40u) ==
                  runtime.words.at(Runtime::object + 0x44u) &&
              runtime.bytes.at(Runtime::object + 0x3cu) == 1u &&
              geometry.calls.empty(),
          "immediate mode copies fields, applies camera and completes");
    const auto look_at = std::find(runtime.calls.begin(), runtime.calls.end(),
                                   "look at");
    Check(look_at != runtime.calls.end() &&
              std::vector<std::string>(look_at, look_at + 5) ==
                  (std::vector<std::string>{"look at", "second angle",
                                            "angle", "third angle", "scalar"}),
          "immediate camera calls retain IDA order");

    Runtime other;
    other.words[Runtime::record + 0x38u] = 2u;
    Check(pal4::CameraTaskObject::Enter(Runtime::object, other, geometry) ==
                  1u &&
              other.bytes.find(Runtime::object + 0x3cu) == other.bytes.end() &&
              std::find(other.calls.begin(), other.calls.end(), "look at") ==
                  other.calls.end() &&
              geometry.calls.empty(),
          "other nonzero mode returns without camera setters or completion");
}

void InitializeUpdateFields(Runtime& runtime, const std::uint32_t flag) {
    const auto put = [&](const std::uint32_t offset, const float value) {
        runtime.words[Runtime::object + offset] =
            std::bit_cast<std::uint32_t>(value);
    };
    runtime.bytes[Runtime::object + 0x3cu] = 0u;
    put(0x40u, 0.0f);
    put(0x44u, 2.0f);
    put(0x50u, 1.0f);
    put(0x54u, 2.0f);
    put(0x58u, 3.0f);
    put(0x68u, 4.0f);
    put(0x6cu, 5.0f);
    put(0x70u, 6.0f);
    put(0x74u, 8.0f);
    put(0x78u, 9.0f);
    put(0x7cu, 10.0f);
    put(0x80u, 20.0f);
    put(0x5cu, 30.0f);
    put(0x60u, 40.0f);
    put(0x88u, 20.0f);
    put(0x8cu, 10.0f);
    runtime.words[Runtime::object + 0x84u] = flag;
    runtime.words[Runtime::record + 0x0cu] =
        std::bit_cast<std::uint32_t>(11.0f);
    runtime.words[Runtime::record + 0x10u] =
        std::bit_cast<std::uint32_t>(12.0f);
    runtime.words[Runtime::record + 0x14u] =
        std::bit_cast<std::uint32_t>(13.0f);
}

void CheckUpdateBranches() {
    Runtime anchored;
    InitializeUpdateFields(anchored, 9u);
    Check(pal4::CameraTaskObject::Update(Runtime::object, 1.0f, anchored) ==
                  1u &&
              WordAsFloat(anchored, Runtime::object + 0x40u) == 1.0f &&
              anchored.bytes.at(Runtime::object + 0x3cu) == 0u &&
              anchored.position.x == 6.0f &&
              anchored.position.y == 7.0f &&
              anchored.position.z == 8.0f &&
              anchored.scalar_value == 45.0f &&
              anchored.angle_value == 50.0f &&
              anchored.second_angle_value == 35.0f &&
              anchored.interpolations ==
                  (std::vector<std::pair<float, float>>{
                      {3.0f, 13.0f}, {2.0f, 12.0f}, {1.0f, 11.0f},
                      {20.0f, 70.0f}}),
          "anchored update interpolates z/y/x, scalar and angles");

    const auto before = anchored.calls.size();
    Check(pal4::CameraTaskObject::Update(Runtime::object, 2.0f, anchored) ==
                  1u &&
              anchored.bytes.at(Runtime::object + 0x3cu) == 1u &&
              WordAsFloat(anchored, Runtime::object + 0x40u) == 2.0f &&
              anchored.position.x == 11.0f &&
              anchored.scalar_value == 70.0f &&
              anchored.calls.size() > before,
          "duration reached clamps elapsed and applies final endpoint");
    const auto final_calls = anchored.calls.size();
    Check(pal4::CameraTaskObject::Update(Runtime::object, 1.0f, anchored) ==
                  0u &&
              anchored.calls.size() == final_calls,
          "completed task short circuits before elapsed write");

    Runtime unanchored;
    InitializeUpdateFields(unanchored, 0u);
    Check(pal4::CameraTaskObject::Update(Runtime::object, 1.0f,
                                          unanchored) == 1u &&
              unanchored.eye.x == 6.0f && unanchored.eye.y == 7.0f &&
              unanchored.eye.z == 8.0f &&
              unanchored.interpolations.front() ==
                  std::pair{6.0f, 10.0f} &&
              std::find(unanchored.calls.begin(), unanchored.calls.end(),
                        "position") == unanchored.calls.end(),
          "zero anchor interpolates eye and skips position setter");

    Runtime other_mode;
    InitializeUpdateFields(other_mode, 9u);
    other_mode.words[Runtime::record + 0x38u] = 2u;
    Check(pal4::CameraTaskObject::Update(Runtime::object, 1.0f,
                                          other_mode) == 1u &&
              WordAsFloat(other_mode, Runtime::object + 0x40u) == 1.0f &&
              other_mode.interpolations.empty(),
          "nonzero mode advances time without camera interpolation");
}

void CheckInterpolatedAngles() {
    Runtime runtime;
    Geometry geometry;
    geometry.initial_euler = {170.0f, -170.0f, 0.0f};
    geometry.final_euler = {-170.0f, 170.0f, 0.0f};
    runtime.words[Runtime::record + 0x24u] = 1u;
    Check(pal4::CameraTaskObject::Enter(Runtime::object, runtime, geometry) ==
                  1u &&
              WordAsFloat(runtime, Runtime::object + 0x88u) == 340.0f &&
              WordAsFloat(runtime, Runtime::object + 0x8cu) == 20.0f &&
              geometry.directions.size() == 2u &&
              geometry.directions[0].x == 10.0f &&
              geometry.directions[1].z == 10.0f &&
              geometry.bases.size() == 2u &&
              geometry.bases[0].right.z == 10.0f &&
              geometry.bases[1].right.x == -10.0f &&
              geometry.bases[0].up.y == -100.0f &&
              geometry.bases[1].up.y == -100.0f &&
              geometry.calls ==
                  (std::vector<std::string>{
                      "clamp", "clamp", "normalize", "normalize",
                      "normalize", "normalize", "quaternion",
                      "quaternion", "euler", "euler"}),
          "interpolated mode builds both bases and applies revolution flag");
    Check(runtime.config_keys ==
              (std::vector<std::string>{
                  "SCLD_Extend_eye_x", "SCLD_Extend_eye_y",
                  "SCLD_Extend_eye_z", "SCLD_Extend_lookat_x",
                  "SCLD_Extend_lookat_y", "SCLD_Extend_lookat_z",
                  "SCLD_Extend_eye_x", "SCLD_Extend_eye_y",
                  "SCLD_Extend_eye_z"}),
          "interpolated mode rereads XML eye after look-at");

    Runtime boundary;
    Geometry boundary_geometry;
    boundary_geometry.initial_euler = {-90.0f, 90.0f, 0.0f};
    boundary_geometry.final_euler = {90.0f, -90.0f, 0.0f};
    Check(pal4::CameraTaskObject::Enter(Runtime::object, boundary,
                                         boundary_geometry) == 1u &&
              WordAsFloat(boundary, Runtime::object + 0x88u) == -180.0f &&
              WordAsFloat(boundary, Runtime::object + 0x8cu) == 180.0f,
          "180-degree boundary keeps direct signed difference");
}

void CheckCapturedQ07TurnRounding() {
    Runtime runtime;
    Geometry geometry;
    // Pair145 at42872: all6 actual quaternion/Euler input/output records
    // agree; this last173.999313 -> -169.001892 turn exposes reassociation.
    geometry.initial_euler = {0.f, std::bit_cast<float>(1127088083u), 0.f};
    geometry.final_euler = {0.f, std::bit_cast<float>(3274244220u), 0.f};
    Check(pal4::CameraTaskObject::Enter(Runtime::object,runtime,geometry)==1u &&
          runtime.words.at(Runtime::object+0x88u)==1099431304u,
          "captured Q07 wrap uses original PC24 (180-start)+(finish+180) order");
    // Independent operation-association witness: the two x87 registers at
    // 592DC0/592DC8 are summed by592DCE, not a left-associative C++ chain.
    Runtime association;
    Geometry two_registers;
    two_registers.initial_euler={0.f,std::bit_cast<float>(1126005999u),0.f};
    two_registers.final_euler={0.f,std::bit_cast<float>(3255731934u),0.f};
    Check(pal4::CameraTaskObject::Enter(Runtime::object,association,two_registers)==1u &&
          association.words.at(Runtime::object+0x88u)==1126621529u,
          "two original x87 half-turn expressions retain separate PC24 rounding");
    Runtime mirror;
    Geometry reversed;
    reversed.initial_euler={0.f,-geometry.initial_euler.y,0.f};
    reversed.final_euler={0.f,-geometry.final_euler.y,0.f};
    Check(pal4::CameraTaskObject::Enter(Runtime::object,mirror,reversed)==1u &&
          runtime.words.at(Runtime::object+0x88u)==(mirror.words.at(Runtime::object+0x88u)^0x80000000u),
          "opposite sign crossing follows the corresponding original negated operation chain");
}

}  // namespace

int main() {
    try {
        CheckNoCamera();
        CheckImmediateAndOtherModes();
        CheckInterpolatedAngles();
        CheckCapturedQ07TurnRounding();
        CheckUpdateBranches();
        std::cout << "camera_task_playback_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
