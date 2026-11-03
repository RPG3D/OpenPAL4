#include "pal4/touch_other_task.h"

#include <algorithm>
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

std::size_t IndexOf(const std::vector<std::string>& calls,
                    const std::string& name) {
    const auto found = std::find(calls.begin(), calls.end(), name);
    if (found == calls.end()) throw std::runtime_error("missing " + name);
    return static_cast<std::size_t>(found - calls.begin());
}

struct Runtime : virtual pal4::ITouchOtherTaskRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    static constexpr std::uint32_t owner = 0x5000u;
    static constexpr std::uint32_t target = 0x7000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    bool fail_empty_string = false;
    bool target_done = false;
    std::uint32_t found_target = target;
    std::uint32_t bound_target = 0u;
    std::uint32_t bound_position = 0u;

    void BaseConstruct(std::uint32_t address) override {
        Check(address == object, "touch base constructor address");
        calls.emplace_back("base construct");
    }
    std::uint32_t BaseDestruct(std::uint32_t address) override {
        Check(address == object, "touch base destructor address");
        calls.emplace_back("base destruct");
        return 0x76543210u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write word");
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("write byte");
    }
    void InitializeEmptyString(std::uint32_t record) override {
        Check(record == object + 0x44u, "touch empty string record");
        calls.emplace_back("initialize empty string");
        if (fail_empty_string)
            throw std::runtime_error("empty string failed");
        words[record] = 0u;
        words[record + 4u] = 0u;
        words[record + 8u] = 0u;
    }
    void CleanupString(std::uint32_t record) override {
        Check(record == object + 0x44u, "touch cleanup record");
        calls.emplace_back("cleanup string");
    }
    std::uint32_t CStringLength(std::uint32_t source) override {
        Check(source == 0x3000u, "touch name source");
        calls.emplace_back("string length");
        return 4u;
    }
    std::uint32_t InitializeString(
        std::uint32_t record, std::uint32_t source,
        std::uint32_t length) override {
        Check(record == object + 0x44u && source == 0x3000u &&
                  length == 4u,
              "touch initialized string arguments");
        calls.emplace_back("initialize string");
        words[record] = 0x9000u;
        words[record + 4u] = length;
        return 0x12345678u;
    }
    std::uint32_t FindObjectByName(std::uint32_t name) override {
        Check(name == 0x9000u, "touch lookup name");
        calls.emplace_back("find target");
        return found_target;
    }
    void TouchObject(std::uint32_t handle,
                    std::uint32_t position) override {
        Check(handle == target && position == owner + 0xacu,
              "touch target binding arguments");
        calls.emplace_back("bind target");
        bound_target = handle;
        bound_position = position;
    }
    bool ObjectDone(std::uint32_t handle) override {
        Check(handle == target, "touch target completion handle");
        calls.emplace_back("target done");
        return target_done;
    }
};

struct SpawnRuntime final : Runtime, pal4::ITouchOtherTaskSpawnRuntime {
    unsigned allocations{};
    SpawnRuntime() {
        words[object+0x58u]=0x7000u; words[object+0x54u]=owner;
        words[object+0x30u]=0u;
        words[owner+0x2a4u]=0xb000u; words[owner+0x2a8u]=0xb004u;
        words[owner+0x2acu]=0xb008u; words[0xb000u]=0xdeadu;
    }
    std::uint32_t AllocateMemory(std::uint32_t n) override {
        Check(n==0x40u,"touch followup allocates only the helper");
        ++allocations; calls.emplace_back("allocate helper"); return 0xa000u;
    }
    void FreeMemory(std::uint32_t) override {throw std::runtime_error("unexpected helper free");}
    std::uint32_t ConstructHelperTask(std::uint32_t p) override {
        Check(p==0xa000u,"helper allocation owner"); calls.emplace_back("construct helper"); return p;
    }
    void AddHelperTask(std::uint32_t c,std::uint32_t p,std::uint32_t m,
        std::uint32_t w,std::uint32_t d) override {
        Check(c==0x7000u && p==0xa000u && m==1u && !w && !d,"touch helper timing");
        calls.emplace_back("register helper");
    }
    void SetHelperOwner(std::uint32_t p,std::uint32_t o) override {
        Check(p==0xa000u && o==owner,"touch helper binding"); calls.emplace_back("bind helper");
    }
    void NotifyPreviousTask(std::uint32_t p) override {
        Check(p==0xc000u,"touch previous pointer"); calls.emplace_back("previous");
    }
    void MarkProcessed(std::uint32_t c) override {
        Check(c==0x7000u,"touch context owner"); calls.emplace_back("begin context");
    }
};

void CheckFollowup() {
    SpawnRuntime gated; gated.words[Runtime::object+0x58u]=0u;
    Check(!pal4::TouchOtherTask::SpawnFollowup(Runtime::object,gated) && gated.calls.empty(),"touch missing context has no effects");
    SpawnRuntime missing; missing.words[Runtime::object+0x54u]=0u;
    Check(!pal4::TouchOtherTask::SpawnFollowup(Runtime::object,missing) && missing.calls.empty(),"touch missing owner has no effects");
    SpawnRuntime previous; previous.words[Runtime::object+0x30u]=0xc000u;
    Check(pal4::TouchOtherTask::SpawnFollowup(Runtime::object,previous)==1u && !previous.allocations &&
        previous.calls==std::vector<std::string>{"previous"},"touch previous branch creates no copied task or helper");
    SpawnRuntime helper;
    Check(pal4::TouchOtherTask::SpawnFollowup(Runtime::object,helper)==1u && helper.allocations==1u &&
        helper.words.at(0xb004u)==0xa000u && helper.words.at(Runtime::owner+0x2a8u)==0xb008u &&
        IndexOf(helper.calls,"register helper")<IndexOf(helper.calls,"bind helper") &&
        helper.calls.back()=="begin context","touch helper is registered, bound and appended before context begins");
}

void CheckLifecycle() {
    Runtime runtime;
    Check(pal4::TouchOtherTask::Construct(Runtime::object, runtime) ==
              Runtime::object &&
              runtime.words.at(Runtime::object) == 0x8489b4u &&
              runtime.bytes.at(Runtime::object + 0x3cu) == 0u &&
              runtime.words.at(Runtime::object + 0x50u) == 0u &&
              runtime.words.at(Runtime::object + 0x54u) == 0u &&
              runtime.words.at(Runtime::object + 0x58u) == 0u &&
              IndexOf(runtime.calls, "base construct") <
                  IndexOf(runtime.calls, "initialize empty string"),
          "touch constructor layout and string order");
    Check(pal4::TouchOtherTask::Destruct(Runtime::object, runtime) ==
              0x76543210u &&
              runtime.words.at(Runtime::object) == 0x8486fcu &&
              IndexOf(runtime.calls, "cleanup string") <
                  IndexOf(runtime.calls, "base destruct"),
          "touch destructor string cleanup and return");
    Runtime failed;
    failed.fail_empty_string = true;
    bool thrown = false;
    try {
        (void)pal4::TouchOtherTask::Construct(Runtime::object, failed);
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    Check(thrown && failed.calls.back() == "base destruct",
          "touch failed string construction cleanup");
}

void CheckInitializeEnterUpdate() {
    Runtime runtime;
    Check(pal4::TouchOtherTask::Initialize(
              Runtime::object, Runtime::owner, 0x3000u,
              1u, 0x8000u, runtime) == 0x12345678u &&
              runtime.words.at(Runtime::object + 0x54u) ==
                  Runtime::owner &&
              runtime.words.at(Runtime::object + 0x40u) == 1u &&
              runtime.words.at(Runtime::object + 0x58u) == 0x8000u &&
              runtime.bytes.at(Runtime::object + 0x3cu) == 0u &&
              IndexOf(runtime.calls, "string length") <
                  IndexOf(runtime.calls, "initialize string"),
          "touch initializer arguments and raw return");
    runtime.words[Runtime::object + 0x50u] = 1u;
    Check(pal4::TouchOtherTask::Enter(Runtime::object, runtime) == 1u &&
              runtime.words.at(Runtime::object + 0x50u) == 0u,
          "touch entry resets binding");
    runtime.words[Runtime::target + 0x184u] = 1u;
    runtime.calls.clear();
    Check(pal4::TouchOtherTask::Update(Runtime::object, runtime) ==
              1u && runtime.bound_target == Runtime::target &&
              runtime.bound_position == Runtime::owner + 0xacu &&
              runtime.words.at(Runtime::object + 0x50u) == 1u &&
              runtime.bytes.at(Runtime::object + 0x3cu) == 0u &&
              IndexOf(runtime.calls, "find target") <
                  IndexOf(runtime.calls, "bind target") &&
              IndexOf(runtime.calls, "bind target") <
                  IndexOf(runtime.calls, "target done"),
          "touch first update binds target and waits");
    runtime.calls.clear();
    Check(pal4::TouchOtherTask::Update(Runtime::object, runtime) ==
              1u &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "bind target") == runtime.calls.end(),
          "touch does not rebind target");
    runtime.target_done = true;
    Check(pal4::TouchOtherTask::Update(Runtime::object, runtime) ==
              1u && runtime.bytes.at(Runtime::object + 0x3cu) == 1u,
          "touch completes when target done");
    runtime.calls.clear();
    Check(pal4::TouchOtherTask::Update(Runtime::object, runtime) ==
              0u && runtime.calls.empty(),
          "touch completed update gate");
    Runtime no_wait;
    no_wait.words[Runtime::object + 0x54u] = Runtime::owner;
    no_wait.words[Runtime::object + 0x40u] = 0u;
    no_wait.words[Runtime::object + 0x44u] = 0x9000u;
    no_wait.words[Runtime::object + 0x50u] = 0u;
    no_wait.bytes[Runtime::object + 0x3cu] = 0u;
    no_wait.found_target = 0u;
    Check(pal4::TouchOtherTask::Update(Runtime::object, no_wait) ==
              1u && no_wait.bytes.at(Runtime::object + 0x3cu) == 1u &&
              std::find(no_wait.calls.begin(), no_wait.calls.end(),
                        "target done") == no_wait.calls.end(),
          "touch no-wait completes without target");
    Runtime missing_owner;
    missing_owner.words[Runtime::object + 0x54u] = 0u;
    missing_owner.bytes[Runtime::object + 0x3cu] = 1u;
    Check(pal4::TouchOtherTask::Enter(
              Runtime::object, missing_owner) == 0u &&
              pal4::TouchOtherTask::Update(
                  Runtime::object, missing_owner) == 0u &&
              missing_owner.bytes.at(Runtime::object + 0x3cu) == 0u,
          "touch missing owner gates");
    Runtime missing_state;
    missing_state.words[Runtime::object + 0x54u] = Runtime::owner;
    missing_state.words[Runtime::object + 0x40u] = 1u;
    missing_state.words[Runtime::object + 0x44u] = 0x9000u;
    missing_state.words[Runtime::object + 0x50u] = 0u;
    missing_state.bytes[Runtime::object + 0x3cu] = 0u;
    missing_state.words[Runtime::target + 0x184u] = 0u;
    Check(pal4::TouchOtherTask::Update(
              Runtime::object, missing_state) == 1u &&
              missing_state.bytes.at(Runtime::object + 0x3cu) == 1u,
          "touch missing target state completes");
}

}  // namespace

int main() {
    try {
        CheckLifecycle();
        CheckInitializeEnterUpdate();
        CheckFollowup();
        std::cout << "touch_other_task_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
