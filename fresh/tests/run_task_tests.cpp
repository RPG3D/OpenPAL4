#include "pal4/run_task.h"

#include <cmath>
#include <bit>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kTask = 0x4000u;
constexpr std::uint32_t kActor = 0x5000u;

struct Runtime final : pal4::IRunTaskRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, float> floats;
    std::vector<std::string> order;
    std::vector<pal4::RunTaskPosition> positions;
    std::size_t position_index = 0;
    pal4::RunTaskPosition measured;
    pal4::RunTaskPosition written_position;
    float magnitude = 360.0f;
    int child_writes = 0;
    std::uint32_t notification_actor = 0;
    void ConstructBase(std::uint32_t task) override {
        Check(task == kTask, "base constructor task");
        order.emplace_back("base");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        order.emplace_back("write_word");
    }
    float ReadFloat(std::uint32_t address) override { return floats[address]; }
    void WriteFloat(std::uint32_t address, float value) override {
        floats[address] = value;
        order.emplace_back("write_float");
    }
    pal4::RunTaskPosition ChildPosition(std::uint32_t actor) override {
        Check(actor == kActor, "child position actor");
        order.emplace_back("position");
        return positions.at(position_index++);
    }
    double CalculateMagnitude(const pal4::RunTaskPosition& delta) override {
        measured = delta;
        order.emplace_back("magnitude");
        return magnitude;
    }
    void Normalize(pal4::RunTaskPosition& delta) override {
        delta.x *= 0.1f;
        delta.y *= 0.1f;
        delta.z *= 0.1f;
        order.emplace_back("normalize");
    }
    void NotifyRun(std::uint32_t actor, std::uint32_t payload,
                   std::uint32_t mode) override {
        Check(payload == pal4::RunTask::kRunPayload && mode == 1u,
              "run virtual slot 564 arguments");
        notification_actor = actor;
        order.emplace_back("notify");
    }
    int SetChildPosition(std::uint32_t actor,
                         const pal4::RunTaskPosition& position,
                         std::uint32_t mode) override {
        Check(actor == kActor && mode == 0u, "position virtual arguments");
        written_position = position;
        ++child_writes;
        order.emplace_back("set_position");
        return -8;
    }
};

void ConstructAndTarget() {
    Runtime runtime;
    Check(pal4::RunTask::Construct(kTask, runtime) == kTask &&
          runtime.words[kTask + 72u] == 0u &&
          runtime.words[kTask] == pal4::RunTask::kVtable &&
          runtime.order == std::vector<std::string>{
              "base", "write_word", "write_word"},
          "constructor initializes base, actor pointer and vtable");
    runtime.words[0x6000u] = 0x3f800000u;
    runtime.words[0x6004u] = 0x40000000u;
    runtime.words[0x6008u] = 0x40400000u;
    Check(pal4::RunTask::SetTarget(kTask, kActor, 0x6000u, runtime) ==
              0x40000000u &&
          runtime.words[kTask + 72u] == kActor &&
          runtime.words[kTask + 60u] == 0x3f800000u &&
          runtime.words[kTask + 64u] == 0x40000000u &&
          runtime.words[kTask + 68u] == 0x40400000u,
          "target setter copies three raw words and returns second word");
}

void Process() {
    Runtime absent;
    Check(pal4::RunTask::Process(kTask, absent) == 1u &&
          absent.order.empty(), "missing actor is handled without side effects");
    Runtime runtime;
    runtime.words[kTask + 72u] = kActor;
    runtime.floats[kTask + 60u] = 4.0f;
    runtime.floats[kTask + 64u] = 9.0f;
    runtime.floats[kTask + 68u] = 14.0f;
    runtime.positions = {{1.0f, 2.0f, 3.0f},
                         {4.0f, 5.0f, 6.0f},
                         {7.0f, 8.0f, 9.0f}};
    Check(pal4::RunTask::Process(kTask, runtime) == 1u &&
          runtime.position_index == 3u &&
          runtime.measured.x == 3.0f && runtime.measured.y == 4.0f &&
          runtime.measured.z == 5.0f &&
          runtime.floats[kTask + 20u] == 1.0f &&
          std::abs(runtime.floats[kTask + 76u] - 0.3f) < 1e-6f &&
          std::abs(runtime.floats[kTask + 80u] - 0.4f) < 1e-6f &&
          std::abs(runtime.floats[kTask + 84u] - 0.5f) < 1e-6f &&
          runtime.notification_actor == kActor,
          "process samples three positions, stores duration/direction and notifies actor");
    Check(runtime.order.back() == "notify", "run notification follows normalize");
}

void Update() {
    Runtime absent;
    Check(pal4::RunTask::UpdatePosition(kTask, 0.25f, absent) == 1u &&
          absent.child_writes == 0, "missing actor skips movement");
    Runtime runtime;
    runtime.words[kTask + 72u] = kActor;
    runtime.floats[kTask + 20u] = 2.0f;
    runtime.floats[kTask + 32u] = 0.5f;
    runtime.floats[kTask + 76u] = 1.0f;
    runtime.floats[kTask + 80u] = 0.0f;
    runtime.floats[kTask + 84u] = 0.0f;
    runtime.positions = {{1.0f, 2.0f, 3.0f},
                         {4.0f, 5.0f, 6.0f},
                         {7.0f, 8.0f, 9.0f}};
    Check(pal4::RunTask::UpdatePosition(kTask, 0.25f, runtime) == 1u &&
          runtime.child_writes == 1 &&
          runtime.written_position.x == 91.0f &&
          runtime.written_position.y == 5.0f &&
          runtime.written_position.z == 9.0f,
          "update uses three position samples and 360-scaled direction");
    runtime.position_index = 0;
    runtime.child_writes = 0;
    runtime.floats[kTask + 20u] = 1.0f;
    runtime.floats[kTask + 32u] = 0.75f;
    Check(pal4::RunTask::UpdatePosition(kTask, 1.0f, runtime) == 1u &&
          runtime.child_writes == 1 &&
          runtime.written_position.x == 91.0f,
          "update clamps step to remaining duration");
    for (const auto step : {0.0f, -1.0f,
                            std::numeric_limits<float>::quiet_NaN()}) {
        runtime.child_writes = 0;
        Check(pal4::RunTask::UpdatePosition(kTask, step, runtime) == 1u &&
              runtime.child_writes == 0,
              "zero, negative and NaN steps skip position write");
    }
}

void CapturedRetreatPrecision() {
    // Pair100 outer49489 ->49490: same task words and previous position.
    Runtime runtime;
    runtime.words[kTask + 72u] = kActor;
    runtime.floats[kTask + 20u] = std::bit_cast<float>(1057894813u);
    runtime.floats[kTask + 32u] = std::bit_cast<float>(1015759766u);
    runtime.floats[kTask + 76u] = std::bit_cast<float>(1039938836u);
    runtime.floats[kTask + 80u] = 0.0f;
    runtime.floats[kTask + 84u] = std::bit_cast<float>(1065226218u);
    const pal4::RunTaskPosition before{0.0f,0.0f,-26.44635581970215f};
    runtime.positions = {before,before,before};
    Check(pal4::RunTask::UpdatePosition(kTask,
              std::bit_cast<float>(1015759766u),runtime)==1u &&
          std::bit_cast<std::uint32_t>(runtime.written_position.z)==0xc1a2fb40u,
          "captured original retreat rounds step*360 before direction product");
}

}  // namespace

int main() {
    try {
        ConstructAndTarget();
        Process();
        Update();
        CapturedRetreatPrecision();
        std::cout << "run_task_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
