#include "pal4/actor_direction_tasks.h"
#include "pal4/run_task.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kTask = 0x1000u;

struct FaceRuntime final : pal4::IFaceTaskCreationRuntime {
    std::uint32_t allocation = kTask;
    std::uint32_t size = 0u;
    bool throw_on_base = false;
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint32_t> words;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("allocate");
        size = bytes;
        return allocation;
    }
    void Free(std::uint32_t task) override {
        Check(task == kTask, "FaceTask exception frees allocation");
        calls.push_back("free");
    }
    void ConstructBase(std::uint32_t task) override {
        Check(task == kTask, "FaceTask base uses allocation");
        calls.push_back("base");
        if (throw_on_base) throw std::runtime_error("base");
    }
    std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    float ReadFloat(std::uint32_t) override { return 0.0f; }
    pal4::ActorDirectionVector ChildPosition(std::uint32_t) override {
        return {};
    }
    void Normalize(pal4::ActorDirectionVector&) override {}
    double AngleRadians(float) override { return 0.0; }
    void SetPosition(std::uint32_t, std::uint32_t,
                     std::uint32_t) override {}
    void SetFacing(std::uint32_t,
                   const pal4::ActorDirectionVector&,
                   std::uint32_t) override {}
    void SetPrepareState(std::uint32_t,
                         std::uint32_t) override {}
};

struct RunRuntime final : pal4::IRunTaskCreationRuntime {
    std::uint32_t allocation = kTask;
    std::uint32_t size = 0u;
    bool throw_on_base = false;
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint32_t> words;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("allocate");
        size = bytes;
        return allocation;
    }
    void Free(std::uint32_t task) override {
        Check(task == kTask, "RunTask exception frees allocation");
        calls.push_back("free");
    }
    void ConstructBase(std::uint32_t task) override {
        Check(task == kTask, "RunTask base uses allocation");
        calls.push_back("base");
        if (throw_on_base) throw std::runtime_error("base");
    }
    std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    float ReadFloat(std::uint32_t) override { return 0.0f; }
    void WriteFloat(std::uint32_t, float) override {}
    pal4::RunTaskPosition ChildPosition(std::uint32_t) override {
        return {};
    }
    double CalculateMagnitude(const pal4::RunTaskPosition&) override {
        return 0.0f;
    }
    void Normalize(pal4::RunTaskPosition&) override {}
    void NotifyRun(std::uint32_t, std::uint32_t,
                   std::uint32_t) override {}
    int SetChildPosition(std::uint32_t,
                         const pal4::RunTaskPosition&,
                         std::uint32_t) override { return 0; }
};

void FaceFactory() {
    FaceRuntime runtime;
    Check(pal4::FaceTask::Create(runtime) == kTask &&
              runtime.size == 76u &&
              runtime.words.at(kTask + 72u) == 0u &&
              runtime.words.at(kTask) == pal4::FaceTask::kVtable &&
              runtime.calls == std::vector<std::string>{
                  "allocate", "base", "write:4168", "write:4096"},
          "FaceTask factory composes existing constructor");
    runtime = FaceRuntime{};
    runtime.allocation = 0u;
    Check(pal4::FaceTask::Create(runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"allocate"},
          "FaceTask allocation failure skips constructor");
    runtime = FaceRuntime{};
    runtime.throw_on_base = true;
    bool caught = false;
    try {
        (void)pal4::FaceTask::Create(runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls == std::vector<std::string>{
              "allocate", "base", "free"},
          "FaceTask exception releases allocation");
}

void RunFactory() {
    RunRuntime runtime;
    Check(pal4::RunTask::Create(runtime) == kTask &&
              runtime.size == 88u &&
              runtime.words.at(kTask + 72u) == 0u &&
              runtime.words.at(kTask) == pal4::RunTask::kVtable &&
              runtime.calls == std::vector<std::string>{
                  "allocate", "base", "write:4168", "write:4096"},
          "RunTask factory composes existing constructor");
    runtime = RunRuntime{};
    runtime.allocation = 0u;
    Check(pal4::RunTask::Create(runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"allocate"},
          "RunTask allocation failure skips constructor");
    runtime = RunRuntime{};
    runtime.throw_on_base = true;
    bool caught = false;
    try {
        (void)pal4::RunTask::Create(runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls == std::vector<std::string>{
              "allocate", "base", "free"},
          "RunTask exception releases allocation");
}

}  // namespace

int main() {
    FaceFactory();
    RunFactory();
    std::cout << "Script action child factory tests passed\n";
}
