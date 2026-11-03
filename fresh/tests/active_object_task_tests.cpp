#include "pal4/active_object_task.h"

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

struct Runtime : pal4::IActiveObjectTaskSpawnRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    static constexpr std::uint32_t target = 0x5000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> freed;
    bool target_present = true;
    std::uint32_t string_length = 0u;
    std::uint32_t copy_destination = 0u;
    std::uint32_t copy_length = 0u;

    Runtime() {
        words[object + 0x40u] = 0x2000u;
        words[object + 0x48u] = 0x3000u;
        words[object + 0x54u] = 0x4000u;
        bytes[object + 0x3cu] = 0u;
    }
    void BaseConstruct(std::uint32_t address) override {
        Check(address == object, "base construct address");
        calls.emplace_back("base construct");
    }
    std::uint32_t BaseDestruct(std::uint32_t address) override {
        Check(address == object, "base destruct address");
        calls.emplace_back("base destruct");
        return 0x1234u;
    }
    std::uint8_t TransientInitialByte() override {
        calls.emplace_back("transient byte");
        return 0x7au;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back("byte " + std::to_string(address));
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back("word " + std::to_string(address));
    }
    void FreeMemory(std::uint32_t address) override {
        freed.push_back(address);
        calls.emplace_back("free");
    }
    std::uint32_t EmptyStringAddress() override {
        calls.emplace_back("empty string");
        return 0x8000u;
    }
    std::uint32_t GameObjectManager() override {
        calls.emplace_back("object manager");
        return 0x6000u;
    }
    std::uint32_t FindObjectByName(std::uint32_t manager,
                                   std::uint32_t name) override {
        Check(manager == 0x6000u &&
                  (name == 0x3000u || name == 0x8000u),
              "named-object lookup arguments");
        calls.emplace_back("find object");
        return target_present ? target : 0u;
    }
    std::uint32_t ArenaManager() override {
        calls.emplace_back("arena manager");
        return 0x7000u;
    }
    void RemoveAnimatedObject(std::uint32_t manager,
                              std::uint32_t found) override {
        Check(manager == 0x7000u && found == target,
              "remove animated object arguments");
        calls.emplace_back("remove object");
    }
    void InvokeObjectControl(std::uint32_t found,
                             std::uint32_t control) override {
        Check(found == target && control == 0x4000u,
              "object virtual +0x9c control arguments");
        calls.emplace_back("control");
    }
    void AddAnimatedObject(std::uint32_t manager,
                           std::uint32_t found) override {
        Check(manager == 0x7000u && found == target,
              "add animated object arguments");
        calls.emplace_back("add object");
    }
    std::uint32_t CStringLength(std::uint32_t source) override {
        Check(source == 0x8000u, "active-object name source");
        calls.emplace_back("length");
        return string_length;
    }
    [[noreturn]] void ThrowLength() override {
        calls.emplace_back("throw length");
        throw std::length_error("string too long");
    }
    void SetErrorFlag(std::uint32_t record,
                      std::uint8_t flag) override {
        Check(record == object + 0x44u && flag == 1u,
              "active-object string reset arguments");
        calls.emplace_back("reset text");
        words[object + 0x48u] = 0u;
        words[object + 0x4cu] = 0u;
        words[object + 0x50u] = 0u;
    }
    void ReallocateText(std::uint32_t record,
                        std::uint32_t length) override {
        Check(record == object + 0x44u && length == string_length,
              "active-object text allocation arguments");
        calls.emplace_back("reallocate text");
        words[object + 0x48u] = 0xa000u;
        words[object + 0x50u] = length;
    }
    void CopyBytes(std::uint32_t destination,
                   std::uint32_t source,
                   std::uint32_t length) override {
        Check(source == 0x8000u, "active-object text copy source");
        copy_destination = destination;
        copy_length = length;
        calls.emplace_back("copy text");
    }
    std::uint32_t AllocateMemory(std::uint32_t) override {
        throw std::runtime_error("unexpected allocation");
    }
    std::uint32_t ConstructHelperTask(std::uint32_t) override {
        throw std::runtime_error("unexpected helper construction");
    }
    void AddHelperTask(std::uint32_t, std::uint32_t,
                       std::uint32_t, std::uint32_t,
                       std::uint32_t) override {
        throw std::runtime_error("unexpected helper registration");
    }
    void SetHelperOwner(std::uint32_t, std::uint32_t) override {
        throw std::runtime_error("unexpected helper owner");
    }
    void NotifyPreviousTask(std::uint32_t) override {
        throw std::runtime_error("unexpected previous task");
    }
    void MarkProcessed(std::uint32_t) override {
        throw std::runtime_error("unexpected processed flag");
    }
};

struct SpawnRuntime final : Runtime {
    static constexpr std::uint32_t context = 0x6000u;
    static constexpr std::uint32_t vector = 0x2000u + 0x2a0u;
    bool spare_capacity = false;
    bool processed = false;

    SpawnRuntime() {
        words[object + 0x58u] = context;
        words[object + 0x30u] = 0u;
        words[vector + 4u] = 0x9000u;
        words[vector + 8u] = 0x9004u;
        words[vector + 12u] = 0x9004u;
        words[0x9000u] = 0x1111u;
    }
    std::uint32_t AllocateMemory(std::uint32_t bytes) override {
        calls.push_back("allocate " + std::to_string(bytes));
        if (bytes == 0x40u) return 0x8000u;
        Check(bytes == 8u, "one-element owner vector doubles capacity");
        return 0xa000u;
    }
    std::uint32_t ConstructHelperTask(std::uint32_t address) override {
        Check(address == 0x8000u, "helper constructor address");
        calls.emplace_back("construct helper");
        return address;
    }
    void AddHelperTask(std::uint32_t selected_context,
                       std::uint32_t helper,
                       std::uint32_t first,
                       std::uint32_t second,
                       std::uint32_t third) override {
        Check(selected_context == context && helper == 0x8000u &&
                  first == 1u && second == 0u && third == 0u,
              "helper timing and context");
        calls.emplace_back("add helper");
    }
    void SetHelperOwner(std::uint32_t helper,
                        std::uint32_t owner) override {
        Check(helper == 0x8000u && owner == 0x2000u,
              "helper owner binding");
        calls.emplace_back("bind helper");
        if (spare_capacity) words[vector + 12u] = 0x9008u;
    }
    void NotifyPreviousTask(std::uint32_t task) override {
        Check(task == 0xabcdu, "previous task address");
        calls.emplace_back("notify previous");
    }
    void MarkProcessed(std::uint32_t selected_context) override {
        Check(selected_context == context, "processed context");
        processed = true;
        calls.emplace_back("processed");
    }
};

void CheckLifecycle() {
    Runtime constructed;
    Check(pal4::ActiveObjectTask::Construct(Runtime::object,
                                             constructed) ==
                  Runtime::object &&
              constructed.bytes.at(Runtime::object + 0x44u) == 0x7au &&
              constructed.words.at(Runtime::object + 0x48u) == 0u &&
              constructed.words.at(Runtime::object + 0x4cu) == 0u &&
              constructed.words.at(Runtime::object + 0x50u) == 0u &&
              constructed.bytes.at(Runtime::object + 0x3cu) == 0u &&
              constructed.words.at(Runtime::object) == 0x848728u &&
              constructed.calls.front() == "base construct" &&
              constructed.calls[1] == "transient byte",
          "constructor preserves transient byte source and initializes fields");

    for (std::uint32_t count : {0u, 0xffu, 2u}) {
        Runtime runtime;
        runtime.bytes[0x2fffu] = static_cast<std::uint8_t>(count);
        Check(pal4::ActiveObjectTask::Destruct(Runtime::object,
                                                 runtime) == 0x1234u &&
                  runtime.words.at(Runtime::object + 0x48u) == 0u &&
                  runtime.words.at(Runtime::object + 0x4cu) == 0u &&
                  runtime.words.at(Runtime::object + 0x50u) == 0u &&
                  runtime.words.at(Runtime::object) == 0x8486fcu &&
                  runtime.calls.back() == "base destruct" &&
                  (count == 2u ?
                       runtime.bytes.at(0x2fffu) == 1u &&
                           runtime.freed.empty() :
                       runtime.freed ==
                           (std::vector<std::uint32_t>{0x2fffu})),
              "destructor applies reference byte release rules");
    }
    Runtime empty;
    empty.words[Runtime::object + 0x48u] = 0u;
    Check(pal4::ActiveObjectTask::Destruct(Runtime::object, empty) ==
                  0x1234u && empty.freed.empty(),
          "null text pointer skips release");
}

void CheckEnterAndStatus() {
    Runtime absent_owner;
    absent_owner.words[Runtime::object + 0x40u] = 0u;
    Check(pal4::ActiveObjectTask::Enter(Runtime::object,
                                         absent_owner) == 0u &&
              absent_owner.calls.empty(),
          "enter requires active object owner");

    Runtime full;
    Check(pal4::ActiveObjectTask::Enter(Runtime::object, full) == 1u &&
              full.bytes.at(Runtime::object + 0x3cu) == 1u &&
              full.calls ==
                  (std::vector<std::string>{
                      "object manager", "find object", "arena manager",
                      "remove object", "control", "arena manager",
                      "add object", "byte 4156"}),
          "control branch removes, calls virtual slot, re-adds and completes");

    Runtime no_control;
    no_control.words[Runtime::object + 0x54u] = 0u;
    Check(pal4::ActiveObjectTask::Enter(Runtime::object,
                                         no_control) == 1u &&
              no_control.calls ==
                  (std::vector<std::string>{
                      "object manager", "find object", "arena manager",
                      "remove object", "byte 4156"}),
          "zero control removes without re-adding");

    Runtime missing;
    missing.target_present = false;
    missing.words[Runtime::object + 0x48u] = 0u;
    Check(pal4::ActiveObjectTask::Enter(Runtime::object,
                                         missing) == 1u &&
              missing.calls ==
                  (std::vector<std::string>{
                      "empty string", "object manager", "find object",
                      "byte 4156"}),
          "null name uses empty string and missing target still completes");

    Runtime status;
    Check(pal4::ActiveObjectTask::CheckStatus(Runtime::object, 99,
                                               status) == 1u,
          "owner with clear completion remains active");
    status.bytes[Runtime::object + 0x3cu] = 1u;
    Check(pal4::ActiveObjectTask::CheckStatus(Runtime::object, 99,
                                               status) == 0u,
          "owner with complete flag is inactive");
    status.words[Runtime::object + 0x40u] = 0u;
    Check(pal4::ActiveObjectTask::CheckStatus(Runtime::object, 99,
                                               status) == 0u &&
              status.bytes.at(Runtime::object + 0x3cu) == 0u,
          "missing owner clears completion flag");
}

void CheckSpawn() {
    SpawnRuntime no_context;
    no_context.words[Runtime::object + 0x58u] = 0u;
    Check(pal4::ActiveObjectTask::SpawnFollowup(
              Runtime::object, no_context) == 0u &&
              no_context.calls.empty(),
          "active-object follow-up requires context");
    SpawnRuntime no_owner;
    no_owner.words[Runtime::object + 0x40u] = 0u;
    Check(pal4::ActiveObjectTask::SpawnFollowup(
              Runtime::object, no_owner) == 0u &&
              no_owner.calls.empty(),
          "active-object follow-up requires owner");

    SpawnRuntime previous;
    previous.words[Runtime::object + 0x30u] = 0xabcdu;
    Check(pal4::ActiveObjectTask::SpawnFollowup(
              Runtime::object, previous) == 1u &&
              previous.calls ==
                  (std::vector<std::string>{"notify previous"}) &&
              !previous.processed,
          "previous task gets notified without helper creation");

    SpawnRuntime growing;
    Check(pal4::ActiveObjectTask::SpawnFollowup(
              Runtime::object, growing) == 1u &&
              growing.words.at(SpawnRuntime::vector + 4u) == 0xa000u &&
              growing.words.at(SpawnRuntime::vector + 8u) == 0xa008u &&
              growing.words.at(SpawnRuntime::vector + 12u) == 0xa008u &&
              growing.words.at(0xa000u) == 0x1111u &&
              growing.words.at(0xa004u) == 0x8000u &&
              growing.freed ==
                  (std::vector<std::uint32_t>{0x9000u}) &&
              growing.processed &&
              growing.calls.front() == "allocate 64" &&
              growing.calls.back() == "processed",
          "active-object follow-up grows owner pointer range");

    SpawnRuntime spare;
    spare.spare_capacity = true;
    Check(pal4::ActiveObjectTask::SpawnFollowup(
              Runtime::object, spare) == 1u &&
              spare.words.at(SpawnRuntime::vector + 4u) == 0x9000u &&
              spare.words.at(SpawnRuntime::vector + 8u) == 0x9008u &&
              spare.words.at(0x9004u) == 0x8000u &&
              spare.freed.empty() && spare.processed,
          "active-object follow-up appends into spare capacity");
}

void CheckInitialize() {
    Runtime shared_empty;
    shared_empty.bytes[0x2fffu] = 2u;
    Check(pal4::ActiveObjectTask::Initialize(
              Runtime::object, 0x2000u, 0x8000u, 0x4000u,
              0x6000u, shared_empty) == 0x3000u &&
              shared_empty.bytes.at(0x2fffu) == 1u &&
              shared_empty.words.at(Runtime::object + 0x48u) == 0u &&
              shared_empty.words.at(Runtime::object + 0x4cu) == 0u &&
              shared_empty.words.at(Runtime::object + 0x50u) == 0u &&
              shared_empty.words.at(Runtime::object + 0x54u) == 0x4000u &&
              shared_empty.words.at(Runtime::object + 0x58u) == 0x6000u &&
              shared_empty.calls.back() == "word 4184",
          "empty shared text decrements refcount and returns old data pointer");

    for (const std::uint8_t count : {std::uint8_t{0u},
                                     std::uint8_t{0xffu}}) {
        Runtime unique_empty;
        unique_empty.bytes[0x2fffu] = count;
        Check(pal4::ActiveObjectTask::Initialize(
                  Runtime::object, 0x2000u, 0x8000u, 0x4000u,
                  0x6000u, unique_empty) == 0x4000u &&
                  unique_empty.words.at(Runtime::object + 0x48u) == 0u &&
                  unique_empty.calls[3] == "reset text",
              "empty nonshared text resets buffer and returns control");
    }

    Runtime reused;
    reused.string_length = 3u;
    reused.bytes[0x2fffu] = 0u;
    reused.words[Runtime::object + 0x50u] = 10u;
    Check(pal4::ActiveObjectTask::Initialize(
              Runtime::object, 0x2000u, 0x8000u, 0x4000u,
              0x6000u, reused) == 0x6000u &&
              reused.copy_destination == 0x3000u &&
              reused.copy_length == 3u &&
              reused.words.at(Runtime::object + 0x4cu) == 3u &&
              reused.bytes.at(0x3003u) == 0u &&
              reused.words.at(Runtime::object + 0x40u) == 0x2000u &&
              reused.bytes.at(Runtime::object + 0x3cu) == 0u &&
              std::find(reused.calls.begin(), reused.calls.end(),
                        "reallocate text") == reused.calls.end(),
          "small unique text reuses existing capacity");

    Runtime shared;
    shared.string_length = 3u;
    shared.bytes[0x2fffu] = 2u;
    shared.words[Runtime::object + 0x50u] = 10u;
    Check(pal4::ActiveObjectTask::Initialize(
              Runtime::object, 0x2000u, 0x8000u, 0x4000u,
              0x6000u, shared) == 0x6000u &&
              shared.copy_destination == 0xa000u &&
              std::find(shared.calls.begin(), shared.calls.end(),
                        "reset text") == shared.calls.end() &&
              std::find(shared.calls.begin(), shared.calls.end(),
                        "reallocate text") != shared.calls.end(),
          "nonempty shared text reallocates without prior reset");

    for (const std::uint32_t capacity : {2u, 40u}) {
        Runtime new_buffer;
        new_buffer.string_length = 3u;
        new_buffer.bytes[0x2fffu] = 0u;
        new_buffer.words[Runtime::object + 0x50u] = capacity;
        Check(pal4::ActiveObjectTask::Initialize(
                  Runtime::object, 0x2000u, 0x8000u, 0x4000u,
                  0x6000u, new_buffer) == 0x6000u &&
                  new_buffer.copy_destination == 0xa000u &&
                  std::find(new_buffer.calls.begin(), new_buffer.calls.end(),
                            "reset text") <
                      std::find(new_buffer.calls.begin(),
                                new_buffer.calls.end(),
                                "reallocate text"),
              "insufficient or large capacity resets then reallocates");
    }

    Runtime too_long;
    too_long.string_length = 0xfffffffeu;
    bool thrown = false;
    try {
        static_cast<void>(pal4::ActiveObjectTask::Initialize(
            Runtime::object, 0x2000u, 0x8000u, 0x4000u,
            0x6000u, too_long));
    } catch (const std::length_error&) { thrown = true; }
    Check(thrown && too_long.words.at(Runtime::object + 0x40u) == 0x2000u &&
              too_long.bytes.at(Runtime::object + 0x3cu) == 0u &&
              too_long.calls.back() == "throw length",
          "oversized string raises length before touching owned buffer");
}

}  // namespace

int main() {
    try {
        CheckLifecycle();
        CheckEnterAndStatus();
        CheckSpawn();
        CheckInitialize();
        std::cout << "active_object_task_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
