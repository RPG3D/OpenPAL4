#include "pal4/action_task_object.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime : pal4::IActionTaskSpawnRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    static constexpr std::uint32_t owner = 0x2000u;
    static constexpr std::uint32_t animation = owner + 0x1dcu;

    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::string current_name = "ATTACK";
    std::string action_name = "attack";
    std::string played_path;
    std::uint32_t played_mode = 0u;
    float played_time = 0.0f;
    bool animation_flag = false;
    bool fail_suffix = false;
    bool fail_string_init = false;
    bool fail_string_cleanup = false;
    int set_animation_count = 0;
    std::uint32_t active_task = object;

    Runtime() {
        words[object + 0x54u] = owner;
        words[object + 0x40u] = 0x6000u;
        words[object + 0x50u] = 2u;
        words[owner + 0x1b0u] = 0u;
        bytes[object + 0x3cu] = 0u;
    }
    void BaseConstruct(std::uint32_t address) override {
        Check(address == active_task, "base construct address");
        calls.emplace_back("base construct");
    }
    std::uint32_t BaseDestruct(std::uint32_t address) override {
        Check(address == active_task, "base destruct address");
        calls.emplace_back("base destruct");
        return 0x4321u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("word " + std::to_string(address));
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back("byte " + std::to_string(address));
    }
    void InitializeEmptyString(std::uint32_t record) override {
        Check(record == active_task + 0x40u, "action string record");
        calls.emplace_back("string init");
        if (fail_string_init) throw std::runtime_error("string init failed");
    }
    void InitializeStringFromCString(std::uint32_t record,
                                      std::uint32_t source) override {
        Check(record == active_task + 0x40u && source == 0x6000u,
              "action string initialization arguments");
        calls.emplace_back("string assign");
        words[record] = source;
    }
    void CleanupString(std::uint32_t record) override {
        Check(record == active_task + 0x40u, "action string cleanup");
        calls.emplace_back("string cleanup");
        if (fail_string_cleanup)
            throw std::runtime_error("string cleanup failed");
    }
    std::string CurrentAnimationName(std::uint32_t address) override {
        Check(address == animation, "animation name address");
        calls.emplace_back("current name");
        return current_name;
    }
    std::int32_t CompareCaseInsensitive(std::uint32_t record,
                                         std::string_view name) override {
        Check(record == active_task + 0x40u, "name comparison record");
        calls.emplace_back("compare");
        std::string first = action_name;
        std::string second(name);
        std::transform(first.begin(), first.end(), first.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        std::transform(second.begin(), second.end(), second.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return first == second ? 0 : 1;
    }
    void SetAnimation(std::uint32_t address,
                      std::uint32_t action_name_pointer) override {
        Check(address == animation && action_name_pointer == 0x6000u,
              "animation setter arguments");
        ++set_animation_count;
        calls.emplace_back("set animation");
    }
    std::uint32_t ClearAnimationFlag(std::uint32_t address,
                                     std::uint32_t flag) override {
        Check(address == animation && flag <= 1u,
              "clear animation flag arguments");
        calls.push_back("clear " + std::to_string(flag));
        return 0x55u;
    }
    void SetAnimationFlag(std::uint32_t address,
                          std::uint32_t flag) override {
        Check(address == animation && flag == 0u,
              "set animation flag arguments");
        calls.emplace_back("set flag 0");
    }
    bool AnimationFlag(std::uint32_t address) override {
        Check(address == animation, "finished animation flag address");
        calls.emplace_back("animation flag");
        return animation_flag;
    }
    float AnimationLength(std::uint32_t address) override {
        Check(address == animation, "animation length address");
        calls.emplace_back("length");
        return 2.5f;
    }
    void SetAnimationTime(std::uint32_t address, float value) override {
        Check(address == animation, "animation time address");
        played_time = value;
        calls.emplace_back("time");
    }
    std::string ReadStringRecord(std::uint32_t record) override {
        calls.push_back("prefix " + std::to_string(record));
        if (record == 0x94d560u) return "audio/";
        Check(record == 0x94d524u, "second sound prefix record");
        return "voice/";
    }
    bool AppendSoundSuffix(std::string& path,
                            std::string_view suffix) override {
        Check(suffix == "WC003.wav", "action sound suffix");
        calls.emplace_back("append sound");
        if (fail_suffix) return false;
        path += suffix;
        return true;
    }
    void PlayMusic(std::string_view path, std::uint32_t mode) override {
        played_path = path;
        played_mode = mode;
        calls.emplace_back("music");
    }
    std::uint32_t AllocateMemory(std::uint32_t) override {
        throw std::runtime_error("unexpected allocation");
    }
    void FreeMemory(std::uint32_t) override {
        throw std::runtime_error("unexpected free");
    }
    void AddTask(std::uint32_t, std::uint32_t,
                 std::uint32_t) override {
        throw std::runtime_error("unexpected task registration");
    }
    void AddHelperTask(std::uint32_t, std::uint32_t,
                       pal4::ActionTaskTiming) override {
        throw std::runtime_error("unexpected helper registration");
    }
    void InsertCopiedTask(std::uint32_t, std::uint32_t) override {
        throw std::runtime_error("unexpected task insertion");
    }
    void NotifyPreviousTask(std::uint32_t) override {
        throw std::runtime_error("unexpected previous task");
    }
    std::uint32_t ConstructHelperTask(std::uint32_t) override {
        throw std::runtime_error("unexpected helper construction");
    }
    void SetHelperOwner(std::uint32_t, std::uint32_t) override {
        throw std::runtime_error("unexpected helper owner");
    }
    void MarkProcessed(std::uint32_t) override {
        throw std::runtime_error("unexpected processed flag");
    }
};

struct SpawnRuntime final : Runtime {
    static constexpr std::uint32_t context = 0x5000u;
    static constexpr std::uint32_t vector = owner + 0x2a0u;
    bool spare_capacity = false;
    bool fail_action_allocation = false;
    bool fail_helper_allocation = false;
    bool fail_helper_constructor = false;
    bool processed = false;
    std::vector<std::uint32_t> freed;

    SpawnRuntime() {
        words[object + 0x58u] = context;
        words[object + 0x4cu] = 2u;
        words[object + 0x30u] = 0u;
        words[vector + 4u] = 0x9000u;
        words[vector + 8u] = 0x9004u;
        words[vector + 12u] = 0x9008u;
        words[0x9000u] = 0x1111u;
    }
    std::uint32_t AllocateMemory(std::uint32_t size) override {
        calls.push_back("allocate " + std::to_string(size));
        if (size == 0x5cu) {
            active_task = 0x7000u;
            return fail_action_allocation ? 0u : 0x7000u;
        }
        if (size == 0x40u)
            return fail_helper_allocation ? 0u : 0x8000u;
        Check(size == 16u, "helper pointer vector grows to four slots");
        return 0xa000u;
    }
    void FreeMemory(std::uint32_t address) override {
        freed.push_back(address);
        calls.push_back("free " + std::to_string(address));
    }
    void AddTask(std::uint32_t selected_context,
                 std::uint32_t task,
                 std::uint32_t timing_address) override {
        Check(selected_context == context && task == 0x7000u &&
                  timing_address == object + 0x0cu,
              "copied action task container arguments");
        calls.emplace_back("add action");
    }
    void AddHelperTask(std::uint32_t selected_context,
                       std::uint32_t helper,
                       pal4::ActionTaskTiming timing) override {
        Check(selected_context == context && helper == 0x8000u &&
                  timing.first == 1u && timing.second == 0u &&
                  timing.third == 0u,
              "helper task container timing");
        calls.emplace_back("add helper");
    }
    void InsertCopiedTask(std::uint32_t collection,
                          std::uint32_t task) override {
        Check(collection == vector && task == 0x7000u,
              "first owner task insertion");
        words[0x9004u] = task;
        words[vector + 8u] = 0x9008u;
        if (spare_capacity) words[vector + 12u] = 0x900cu;
        calls.emplace_back("insert action");
    }
    void NotifyPreviousTask(std::uint32_t task) override {
        Check(task == 0xabcdu, "previous task address");
        calls.emplace_back("notify previous");
    }
    std::uint32_t ConstructHelperTask(std::uint32_t address) override {
        Check(address == 0x8000u, "helper constructor address");
        calls.emplace_back("construct helper");
        if (fail_helper_constructor)
            throw std::runtime_error("helper construction failed");
        return address;
    }
    void SetHelperOwner(std::uint32_t helper,
                        std::uint32_t selected_owner) override {
        Check(helper == 0x8000u && selected_owner == owner,
              "helper owner binding");
        calls.emplace_back("bind helper");
    }
    void MarkProcessed(std::uint32_t selected_context) override {
        Check(selected_context == context, "processed context");
        processed = true;
        calls.emplace_back("processed");
    }
};

void CheckLifecycle() {
    Runtime runtime;
    Check(pal4::ActionTaskObject::Construct(Runtime::object, runtime) ==
                  Runtime::object &&
              runtime.words.at(Runtime::object) == 0x8486d8u &&
              runtime.bytes.at(Runtime::object + 0x3cu) == 0u &&
              runtime.words.at(Runtime::object + 0x4cu) == 1u &&
              runtime.words.at(Runtime::object + 0x50u) == 0u &&
              runtime.calls ==
                  (std::vector<std::string>{
                      "base construct", "word 4096", "string init",
                      "word 4096", "byte 4156", "word 4172",
                      "word 4176"}),
          "constructor uses base and derived vtables around text init");
    runtime.calls.clear();
    Check(pal4::ActionTaskObject::Destruct(Runtime::object, runtime) ==
                  0x4321u &&
              runtime.words.at(Runtime::object) == 0x8486fcu &&
              runtime.calls ==
                  (std::vector<std::string>{
                      "word 4096", "byte 4156", "string cleanup",
                      "word 4096", "base destruct"}),
          "destructor releases text before base teardown");

    Runtime failed_init;
    failed_init.fail_string_init = true;
    bool thrown = false;
    try {
        static_cast<void>(pal4::ActionTaskObject::Construct(
            Runtime::object, failed_init));
    } catch (const std::runtime_error&) { thrown = true; }
    Check(thrown && failed_init.words.at(Runtime::object) == 0x8486fcu &&
              failed_init.calls.back() == "base destruct",
          "failed text initialization destroys the base object");

    Runtime failed_cleanup;
    failed_cleanup.fail_string_cleanup = true;
    thrown = false;
    try {
        static_cast<void>(pal4::ActionTaskObject::Destruct(
            Runtime::object, failed_cleanup));
    } catch (const std::runtime_error&) { thrown = true; }
    Check(thrown && failed_cleanup.words.at(Runtime::object) ==
                        0x8486fcu &&
              failed_cleanup.calls.back() == "base destruct",
          "failed text cleanup still destroys the base object");
}

void CheckEnter() {
    Runtime no_owner;
    no_owner.words[Runtime::object + 0x54u] = 0u;
    Check(pal4::ActionTaskObject::Enter(Runtime::object, no_owner) == 0u &&
              no_owner.calls.empty(),
          "missing owner returns before animation and sound");

    Runtime matched;
    Check(pal4::ActionTaskObject::Enter(Runtime::object, matched) == 1u &&
              matched.set_animation_count == 0 &&
              matched.calls ==
                  (std::vector<std::string>{
                      "current name", "compare", "clear 0", "set flag 0"}) &&
              matched.played_path.empty(),
          "case-insensitive match preserves animation and resets flags");

    Runtime mismatch;
    mismatch.current_name = "idle";
    mismatch.words[Runtime::owner + 0x1b0u] = 4u;
    Check(pal4::ActionTaskObject::Enter(Runtime::object, mismatch) == 1u &&
              mismatch.set_animation_count == 1 &&
              mismatch.played_path == "audio/voice/WC003.wav" &&
              mismatch.played_mode == 1u &&
              mismatch.calls ==
                  (std::vector<std::string>{
                      "current name", "compare", "set animation",
                      "clear 0", "set flag 0", "prefix 9753952",
                      "prefix 9753892", "append sound", "music"}),
          "mismatch restarts action and owner type four plays sound");

    Runtime failed_append;
    failed_append.words[Runtime::owner + 0x1b0u] = 4u;
    failed_append.fail_suffix = true;
    Check(pal4::ActionTaskObject::Enter(Runtime::object,
                                         failed_append) == 1u &&
              failed_append.played_path == "audio/voice/",
          "failed sound suffix append plays retained prefix");
}

void CheckUpdate() {
    Runtime no_owner;
    no_owner.words[Runtime::object + 0x54u] = 0u;
    no_owner.bytes[Runtime::object + 0x3cu] = 1u;
    Check(pal4::ActionTaskObject::Update(Runtime::object, 123,
                                          no_owner) == 0u &&
              no_owner.bytes.at(Runtime::object + 0x3cu) == 0u,
          "missing owner clears completion flag");

    Runtime completed;
    completed.bytes[Runtime::object + 0x3cu] = 1u;
    Check(pal4::ActionTaskObject::Update(Runtime::object, 123,
                                          completed) == 0u &&
              completed.calls.empty(),
          "completed action short circuits");

    Runtime mismatch;
    mismatch.current_name = "idle";
    Check(pal4::ActionTaskObject::Update(Runtime::object, 123,
                                          mismatch) == 0u &&
              mismatch.bytes.at(Runtime::object + 0x3cu) == 1u &&
              std::find(mismatch.calls.begin(), mismatch.calls.end(),
                        "animation flag") == mismatch.calls.end(),
          "changed animation marks action complete without cycle check");

    Runtime waiting;
    Check(pal4::ActionTaskObject::Update(Runtime::object, 123,
                                          waiting) == 1u &&
              waiting.words.at(Runtime::object + 0x50u) == 2u &&
              waiting.set_animation_count == 0,
          "unfinished animation leaves cycle count unchanged");

    Runtime again;
    again.animation_flag = true;
    again.words[Runtime::object + 0x50u] = 3u;
    Check(pal4::ActionTaskObject::Update(Runtime::object, 123,
                                          again) == 1u &&
              again.words.at(Runtime::object + 0x50u) == 2u &&
              again.set_animation_count == 1 &&
              again.bytes.at(Runtime::object + 0x3cu) == 0u,
          "finished cycle decrements and restarts when count remains positive");

    Runtime last;
    last.animation_flag = true;
    last.words[Runtime::object + 0x50u] = 1u;
    Check(pal4::ActionTaskObject::Update(Runtime::object, 123,
                                          last) == 1u &&
              last.words.at(Runtime::object + 0x50u) == 0u &&
              last.bytes.at(Runtime::object + 0x3cu) == 1u &&
              last.played_time == 2.5f &&
              last.set_animation_count == 0 &&
              last.calls.size() >= 4u &&
              last.calls[last.calls.size() - 3u] == "length" &&
              last.calls[last.calls.size() - 2u] == "time" &&
              last.calls.back() == "clear 1",
          "final cycle clamps animation time and clears flag one");
}

std::size_t IndexOf(const std::vector<std::string>& calls,
                    const std::string& value) {
    const auto found = std::find(calls.begin(), calls.end(), value);
    Check(found != calls.end(), "missing expected service call");
    return static_cast<std::size_t>(found - calls.begin());
}

void CheckInitializationAndFactories() {
    Runtime initialized;
    Check(pal4::ActionTaskObject::Initialize(
              Runtime::object, Runtime::owner, 3u, 0x6000u,
              0x5000u, initialized) == 0x55u &&
              initialized.words.at(Runtime::object + 0x54u) ==
                  Runtime::owner &&
              initialized.words.at(Runtime::object + 0x58u) == 0x5000u &&
              initialized.words.at(Runtime::object + 0x4cu) == 3u &&
              initialized.words.at(Runtime::object + 0x50u) == 3u &&
              initialized.bytes.at(Runtime::object + 0x3cu) == 0u &&
              initialized.played_time == 0.0f &&
              initialized.set_animation_count == 0 &&
              initialized.calls.back() == "clear 1",
          "runtime initialization copies owner/count and resets matched animation");

    Runtime mismatch;
    mismatch.current_name = "idle";
    Check(pal4::ActionTaskObject::Initialize(
              Runtime::object, Runtime::owner, 2u, 0x6000u,
              0u, mismatch) == 0x55u &&
              mismatch.set_animation_count == 1 &&
              IndexOf(mismatch.calls, "set animation") <
                  IndexOf(mismatch.calls, "clear 1"),
          "runtime initialization switches mismatched animation");

    Runtime descriptors;
    Check(pal4::ActionTaskFactory::ConstructActionDescriptor(
              0xb000u, descriptors) == 0xb000u &&
              pal4::ActionTaskFactory::ConstructHelperDescriptor(
                  0xb004u, descriptors) == 0xb004u &&
              descriptors.words.at(0xb000u) == 0x848724u &&
              descriptors.words.at(0xb004u) == 0x848720u,
          "factory descriptor constructors write their distinct vtables");

    SpawnRuntime action_factory;
    Check(pal4::ActionTaskFactory::CreateAction(action_factory) == 0x7000u &&
              action_factory.words.at(0x7000u) == 0x8486d8u &&
              action_factory.freed.empty(),
          "action factory allocates 0x5c and constructs action object");
    SpawnRuntime null_action;
    null_action.fail_action_allocation = true;
    Check(pal4::ActionTaskFactory::CreateAction(null_action) == 0u &&
              null_action.freed.empty(),
          "action factory returns zero on null allocation");
    SpawnRuntime failed_action;
    failed_action.fail_string_init = true;
    bool thrown = false;
    try {
        static_cast<void>(pal4::ActionTaskFactory::CreateAction(failed_action));
    } catch (const std::runtime_error&) { thrown = true; }
    Check(thrown && failed_action.freed ==
                        (std::vector<std::uint32_t>{0x7000u}),
          "action factory frees allocation after constructor failure");

    SpawnRuntime helper_factory;
    Check(pal4::ActionTaskFactory::CreateHelper(helper_factory) == 0x8000u &&
              helper_factory.freed.empty(),
          "helper factory allocates 0x40 and calls native constructor");
    SpawnRuntime null_helper;
    null_helper.fail_helper_allocation = true;
    Check(pal4::ActionTaskFactory::CreateHelper(null_helper) == 0u,
          "helper factory returns zero on null allocation");
    SpawnRuntime failed_helper;
    failed_helper.fail_helper_constructor = true;
    thrown = false;
    try {
        static_cast<void>(pal4::ActionTaskFactory::CreateHelper(failed_helper));
    } catch (const std::runtime_error&) { thrown = true; }
    Check(thrown && failed_helper.freed ==
                        (std::vector<std::uint32_t>{0x8000u}),
          "helper factory frees allocation after constructor failure");
}

void CheckSpawn() {
    SpawnRuntime no_context;
    no_context.words[Runtime::object + 0x58u] = 0u;
    Check(pal4::ActionTaskObject::SpawnFollowup(
              Runtime::object, no_context) == 0u &&
              no_context.calls.empty(),
          "spawn requires context");
    SpawnRuntime no_owner;
    no_owner.words[Runtime::object + 0x54u] = 0u;
    Check(pal4::ActionTaskObject::SpawnFollowup(
              Runtime::object, no_owner) == 0u &&
              no_owner.calls.empty(),
          "spawn requires owner");

    SpawnRuntime growing;
    Check(pal4::ActionTaskObject::SpawnFollowup(
              Runtime::object, growing) == 1u &&
              growing.words.at(0x7000u + 0x54u) == Runtime::owner &&
              growing.words.at(0x7000u + 0x4cu) == 2u &&
              growing.words.at(SpawnRuntime::vector + 4u) == 0xa000u &&
              growing.words.at(SpawnRuntime::vector + 8u) == 0xa00cu &&
              growing.words.at(SpawnRuntime::vector + 12u) == 0xa010u &&
              growing.words.at(0xa000u) == 0x1111u &&
              growing.words.at(0xa004u) == 0x7000u &&
              growing.words.at(0xa008u) == 0x8000u &&
              growing.freed == (std::vector<std::uint32_t>{0x9000u}) &&
              growing.processed &&
              IndexOf(growing.calls, "add action") <
                  IndexOf(growing.calls, "string assign") &&
              IndexOf(growing.calls, "insert action") <
                  IndexOf(growing.calls, "construct helper") &&
              IndexOf(growing.calls, "bind helper") <
                  IndexOf(growing.calls, "processed"),
          "spawn copies action and grows owner vector for helper");

    SpawnRuntime spare;
    spare.spare_capacity = true;
    Check(pal4::ActionTaskObject::SpawnFollowup(
              Runtime::object, spare) == 1u &&
              spare.words.at(SpawnRuntime::vector + 4u) == 0x9000u &&
              spare.words.at(SpawnRuntime::vector + 8u) == 0x900cu &&
              spare.words.at(0x9008u) == 0x8000u &&
              spare.freed.empty() && spare.processed &&
              std::find(spare.calls.begin(), spare.calls.end(),
                        "allocate 16") == spare.calls.end(),
          "spare capacity appends helper without reallocating");

    SpawnRuntime previous;
    previous.words[Runtime::object + 0x30u] = 0xabcdu;
    Check(pal4::ActionTaskObject::SpawnFollowup(
              Runtime::object, previous) == 1u &&
              IndexOf(previous.calls, "notify previous") >
                  IndexOf(previous.calls, "insert action") &&
              !previous.processed &&
              std::find(previous.calls.begin(), previous.calls.end(),
                        "construct helper") == previous.calls.end(),
          "previous task path notifies and skips helper creation");
}

}  // namespace

int main() {
    try {
        CheckLifecycle();
        CheckEnter();
        CheckUpdate();
        CheckInitializationAndFactories();
        CheckSpawn();
        std::cout << "action_task_object_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
