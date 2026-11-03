#include "pal4/check_protect_task.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kTask = 0x1000u;
constexpr std::uint32_t kSource = 0x2000u;
constexpr std::uint32_t kProtector = 0x3000u;
constexpr std::uint32_t kTarget = 0x4000u;
constexpr std::uint32_t kFirst = 0x5000u;
constexpr std::uint32_t kSecond = 0x6000u;

struct Actors final : pal4::IScriptActorEventRuntime {
    std::int32_t source_state = 2;
    std::vector<std::string> calls;
    std::size_t position = 0u;
    bool has_target = true;

    std::int32_t ActorState(std::uint32_t actor) override {
        calls.push_back("state");
        return actor == kSource ? source_state : 1;
    }
    bool SourceBlocksEvent(std::uint32_t actor) override {
        Check(actor == kSource, "gate receives source");
        calls.push_back("blocked");
        return false;
    }
    std::uint32_t OpenTargetIterator(std::uint32_t actor) override {
        Check(actor == kProtector, "protector owns iterator");
        calls.push_back("open");
        return 0x7000u;
    }
    bool IteratorAtEnd(std::uint32_t iterator) override {
        Check(iterator == 0x7000u, "iterator identity");
        calls.push_back("end");
        return !has_target || position != 0u;
    }
    std::uint32_t IteratorCurrent(std::uint32_t iterator) override {
        Check(iterator == 0x7000u, "iterator current identity");
        calls.push_back("current");
        return kTarget;
    }
    void IteratorNext(std::uint32_t iterator) override {
        Check(iterator == 0x7000u, "iterator next identity");
        calls.push_back("next");
        ++position;
    }
    bool CandidateStateNotThree(std::uint32_t actor) override {
        Check(actor == kTarget, "candidate state receives target");
        calls.push_back("eligible");
        return true;
    }
    bool CandidateCanAffect(std::uint32_t actor,
                            std::uint32_t target) override {
        Check(actor == kTarget && target == kProtector,
              "candidate affect receives protector");
        calls.push_back("affect");
        return true;
    }
    std::uint32_t GenerateRandomNumber() override {
        calls.push_back("random");
        return 37u;
    }
    void DeleteIterator(std::uint32_t iterator,
                        std::uint32_t flags) override {
        Check(iterator == 0x7000u && flags == 1u,
              "iterator cleanup uses deleting flag");
        calls.push_back("delete");
    }
};

struct Runtime final : pal4::ICheckProtectStartRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 44u, kSource}, {kTask + 48u, kProtector}};
    std::map<std::uint32_t, float> floats;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> factories;
    std::vector<std::string> texts;
    std::uint32_t first = kFirst;
    std::uint32_t second = kSecond;
    std::size_t created = 0u;
    std::array<float, 3> source_position{13.0f, 24.0f, 35.0f};
    std::array<float, 3> protector_position{3.0f, 4.0f, 5.0f};
    std::array<float, 3> normalized_direction{0.2f, -0.4f, 0.8f};
    std::array<float, 6> bounds{30.0f, 0.0f, 0.0f,
                                10.0f, 0.0f, 0.0f};

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    void WriteFloat(std::uint32_t address, float value) override {
        calls.push_back("float:" + std::to_string(address));
        floats[address] = value;
    }
    void NotifySource(std::uint32_t actor) override {
        Check(actor == kSource, "notify source virtual receives source");
        calls.push_back("notify");
    }
    std::array<float, 3> ActorChildPosition(
        std::uint32_t actor) override {
        calls.push_back("child_position:" + std::to_string(actor));
        if (actor == kSource) return source_position;
        Check(actor == kProtector, "position reads protector child");
        return protector_position;
    }
    void NormalizeVector(std::array<float, 3>& vector) override {
        Check(vector == std::array<float, 3>{10.0f, 20.0f, 30.0f},
              "RenderWare receives source minus protector");
        calls.push_back("normalize");
        vector = normalized_direction;
    }
    std::array<float, 6> ActorChildBoundingBox(
        std::uint32_t actor) override {
        Check(actor == kSource, "bounds come from source actor");
        calls.push_back("bbox");
        return bounds;
    }
    std::string_view GlobalText(std::uint32_t address) override {
        calls.push_back("text:" + std::to_string(address));
        if (address == 0x8f9798u) return "first-label";
        if (address == 0x8f9634u) return {};
        throw std::runtime_error("unexpected global text address");
    }
    std::uint32_t CreateChildTask(
        const pal4::CheckProtectSubtaskFactorySlot& factory) override {
        factories.push_back(factory.vtable);
        calls.push_back("create:" + std::to_string(factory.vtable));
        return created++ == 0u ? first : second;
    }
    void AddTaskAndInitialize(
        std::uint32_t container, std::uint32_t child,
        std::string_view source,
        const std::array<std::uint32_t, 3>& timing) override {
        Check(container == kTask + 4u &&
                  (child == kFirst || child == kSecond) &&
                  timing == std::array<std::uint32_t, 3>{1u, 0u, 0u},
              "child is added with original container and timing");
        texts.emplace_back(source);
        calls.push_back("add:" + std::to_string(child));
    }
};

std::size_t Index(const Runtime& runtime, const std::string& call) {
    const auto it = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "expected call occurred");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

void FactoryWrappers() {
    pal4::CheckProtectSubtaskFactorySlot slot;
    Check(pal4::CheckProtectSubtaskFactory::InitializePosition(&slot) ==
              &slot &&
          slot.vtable ==
              pal4::CheckProtectSubtaskFactory::kPositionVtable,
          "first factory writes vtable and returns its slot");
    Check(pal4::CheckProtectSubtaskFactory::InitializeResponse(&slot) ==
              &slot &&
          slot.vtable ==
              pal4::CheckProtectSubtaskFactory::kResponseVtable,
          "second factory reuses slot with its vtable");
}

void StartTwoTasks() {
    Runtime runtime;
    Actors actors;
    Check(pal4::CheckProtectTask::Start(kTask, runtime, actors) ==
              kProtector &&
          runtime.words[kTask + 52u] == kTarget &&
          runtime.words[kFirst + 72u] == kTarget &&
          runtime.floats[kFirst + 60u] == 15.0f &&
          runtime.floats[kFirst + 64u] == -20.0f &&
          runtime.floats[kFirst + 68u] == 53.0f &&
          runtime.words[kSecond + 60u] == kSource &&
          runtime.words[kSecond + 64u] == kTarget &&
          runtime.words[kSecond + 68u] == kProtector,
          "start wires selected actor, position and response task fields");
    Check(runtime.factories ==
              std::vector<std::uint32_t>{
                  pal4::CheckProtectSubtaskFactory::kPositionVtable,
                  pal4::CheckProtectSubtaskFactory::kResponseVtable} &&
          runtime.texts == std::vector<std::string>{"first-label", ""} &&
          Index(runtime, "notify") < Index(runtime, "write:4148") &&
          Index(runtime, "bbox") < Index(runtime, "add:20480") &&
          Index(runtime, "add:20480") < Index(runtime, "float:20540") &&
          Index(runtime, "float:20540") < Index(runtime, "add:24576"),
          "two factories, global text fallback and child order match IDA");
    Check(actors.calls.back() == "delete",
          "target selector releases actor iterator");
}

void PositionFormula() {
    Runtime runtime;
    std::array<float, 3> output{};
    Check(pal4::CheckProtectTask::CalculatePosition(kTask, output,
                                                     runtime) == &output &&
          output == std::array<float, 3>{15.0f, -20.0f, 53.0f},
          "position uses half bbox extent plus 50 and returns output slot");
    Check(runtime.calls == std::vector<std::string>{
              "read:4140", "child_position:8192", "read:4144",
              "child_position:12288", "normalize", "bbox"},
          "position calls actor children, RenderWare and bounds in order");
}

void NoTargetAndChildFailures() {
    Runtime runtime;
    Actors actors;
    actors.source_state = 1;
    Check(pal4::CheckProtectTask::Start(kTask, runtime, actors) == 0u &&
          runtime.words[kTask + 52u] == 0u &&
          runtime.created == 0u &&
          runtime.calls.back() == "write:4148",
          "no target exits after storing zero");

    runtime = Runtime{};
    actors = Actors{};
    runtime.first = 0u;
    Check(pal4::CheckProtectTask::Start(kTask, runtime, actors) == 0u &&
          runtime.created == 1u && runtime.texts.empty(),
          "null first child safely exits before field writes");

    runtime = Runtime{};
    actors = Actors{};
    runtime.second = 0u;
    Check(pal4::CheckProtectTask::Start(kTask, runtime, actors) == 0u &&
          runtime.created == 2u && runtime.texts.size() == 1u &&
          runtime.words[kFirst + 72u] == kTarget,
          "null second child retains first child without invalid writes");
}

}  // namespace

int main() {
    FactoryWrappers();
    StartTwoTasks();
    PositionFormula();
    NoTargetAndChildFailures();
    std::cout << "CheckProtect start tests passed\n";
}
