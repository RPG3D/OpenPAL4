#include "pal4/camera_sequence_task.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <deque>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime : virtual pal4::ICameraSequenceTaskRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> freed;

    void BaseConstruct(std::uint32_t address) override {
        Check(address == object, "camera task base construct address");
        calls.emplace_back("base construct");
    }
    std::uint32_t BaseDestruct(std::uint32_t address) override {
        Check(address == object, "camera task base destruct address");
        calls.emplace_back("base destruct");
        return 0x1234u;
    }
    std::uint8_t TransientInitialByte() override {
        calls.emplace_back("transient");
        return 0x5au;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back("byte " + std::to_string(address));
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("word " + std::to_string(address));
    }
    void FreeMemory(std::uint32_t address) override {
        freed.push_back(address);
        calls.emplace_back("free");
    }
};

struct PlaybackRuntime final : Runtime, pal4::ICameraSequencePlaybackRuntime {
    static constexpr std::uint32_t active = 0x4000u;
    static constexpr std::uint32_t property = 0x5000u;
    static constexpr std::uint32_t control = 0x6000u;
    std::deque<std::uint32_t> records;
    std::deque<std::uint32_t> created_tasks;
    std::vector<pal4::CameraSequenceTiming> timings;
    std::vector<std::uint32_t> added_tasks;
    bool load_result = true;
    std::uint32_t loaded_filename = 0u;

    PlaybackRuntime() {
        words[object + 0x40u] = 1u;
        words[object + 0x48u] = 0x7000u;
        words[0x800cu] = 0x8100u;
        words[0x8008u] = 0x8200u;
        words[control] = 0x8300u;
        words[property + 0x1a8u] = 0x8400u;
        words[object + 0x54u] = 0u;
        words[object + 0x58u] = 0x8300u;
        words[object + 0x5cu] = 0x8400u;
        words[0x90c0u] = 0x8500u;
        bytes[object + 0x3cu] = 0u;
        bytes[active + 0x1cu] = 0u;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("read byte " + std::to_string(address));
        return Runtime::ReadByte(address);
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read word " + std::to_string(address));
        return Runtime::ReadWord(address);
    }
    std::uint32_t EmptyStringAddress() override {
        calls.emplace_back("empty string");
        return 0x8600u;
    }
    std::uint32_t GetArenaManager() override {
        calls.emplace_back("arena");
        return 0x8000u;
    }
    std::uint32_t GetScriptCloudManager() override {
        calls.emplace_back("cloud");
        return 0x8700u;
    }
    bool LoadCameraFile(std::uint32_t manager, std::uint32_t world,
                        std::uint32_t scene,
                        std::uint32_t filename) override {
        Check(manager == 0x8700u && world == 0x8200u &&
                  scene == 0x8100u, "camera file source fields");
        calls.emplace_back("load");
        loaded_filename = filename;
        return load_result;
    }
    std::uint32_t GetCameraSubsystem() override {
        calls.emplace_back("subsystem");
        return 0x9000u;
    }
    std::uint32_t GetActiveCamera(std::uint32_t subsystem) override {
        Check(subsystem == 0x9000u, "camera subsystem");
        calls.emplace_back("active");
        return active;
    }
    void CleanupCameraTasks(std::uint32_t collection) override {
        Check(collection == active + 0x10u, "camera cleanup address");
        calls.emplace_back("cleanup");
    }
    std::uint32_t FirstCamera(std::uint32_t manager) override {
        Check(manager == 0x8700u, "first cloud manager");
        calls.emplace_back("first");
        return records.empty() ? 0u : records.front();
    }
    std::uint32_t NextCamera(std::uint32_t manager) override {
        Check(manager == 0x8700u, "next cloud manager");
        calls.emplace_back("next");
        records.pop_front();
        return records.empty() ? 0u : records.front();
    }
    std::uint32_t CreateCameraTask(std::uint32_t descriptor_vtable) override {
        Check(descriptor_vtable == 0x848770u, "camera descriptor vtable");
        calls.emplace_back("create");
        const auto task = created_tasks.front();
        created_tasks.pop_front();
        return task;
    }
    void AddCameraTask(std::uint32_t container, std::uint32_t task,
                       pal4::CameraSequenceTiming timing) override {
        Check(container == active + 0x10u, "camera task container");
        calls.emplace_back("add");
        added_tasks.push_back(task);
        timings.push_back(timing);
    }
    std::uint32_t GetInternalId(std::uint32_t camera) override {
        Check(camera == active, "camera internal id owner");
        calls.emplace_back("internal");
        return property;
    }
    void MarkCameraProcessed(std::uint32_t collection) override {
        Check(collection == active + 0x10u, "camera processed address");
        calls.emplace_back("processed");
    }
    std::uint32_t GetActorControl() override {
        calls.emplace_back("control");
        return control;
    }
    void ResetActorControl(std::uint32_t source,
                           std::uint32_t value) override {
        Check(source == control, "actor control source");
        calls.push_back("reset " + std::to_string(value));
    }
    void SetCameraMode(std::uint32_t subsystem,
                       std::uint32_t mode) override {
        Check(subsystem == 0x9000u && mode == 0x8500u,
              "camera restore mode");
        calls.emplace_back("mode");
    }
    void SetLookAtFromObject(std::uint32_t camera,
                             std::uint32_t flag) override {
        Check(camera == active && flag == 1u, "camera restore look-at");
        calls.emplace_back("look-at");
    }
};

struct SpawnRuntime : Runtime, virtual pal4::ICameraSequenceSpawnRuntime {
    static constexpr std::uint32_t owner = 0x8000u;
    static constexpr std::uint32_t collection = owner + 0x2a0u;
    std::deque<std::uint32_t> allocations{0xb000u};
    std::vector<std::uint32_t> allocation_sizes;
    std::uint32_t added = 0u;
    std::uint32_t helper_owner = 0u;

    SpawnRuntime() {
        words[object + 0x60u] = 0x7000u;
        words[object + 0x40u] = owner;
        words[object + 0x30u] = 0u;
        words[collection + 4u] = 0x9000u;
        words[collection + 8u] = 0x9004u;
        words[collection + 12u] = 0x9008u;
        words[0x9000u] = 0xaaaa0000u;
    }
    std::uint32_t AllocateMemory(std::uint32_t bytes) override {
        calls.emplace_back("allocate");
        allocation_sizes.push_back(bytes);
        const auto result = allocations.front();
        allocations.pop_front();
        return result;
    }
    std::uint32_t ConstructHelperTask(std::uint32_t address) override {
        Check(address == 0xb000u, "camera helper allocation");
        calls.emplace_back("construct");
        return address;
    }
    void AddHelperTask(std::uint32_t context, std::uint32_t helper,
                       std::uint32_t first, std::uint32_t second,
                       std::uint32_t third) override {
        Check(context == 0x7000u && first == 1u && second == 0u &&
                  third == 0u, "camera helper timing");
        calls.emplace_back("add");
        added = helper;
    }
    void SetHelperOwner(std::uint32_t helper,
                        std::uint32_t object_owner) override {
        calls.emplace_back("set owner");
        helper_owner = object_owner;
        Check(helper == 0xb000u || helper == 0u, "camera helper address");
    }
    void NotifyPreviousTask(std::uint32_t task) override {
        Check(task == 0xc000u, "camera previous task");
        calls.emplace_back("previous");
    }
    void MarkProcessed(std::uint32_t context) override {
        Check(context == 0x7000u, "camera context processed");
        calls.emplace_back("processed");
    }
};

struct QuakeSpawnRuntime final : SpawnRuntime, pal4::ICameraQuakeSpawnRuntime {
    std::uint32_t inserted_task = 0u;
    bool throw_in_construct = false;

    QuakeSpawnRuntime() {
        allocations = {0xd000u, 0xb000u};
        words[object + 0x4cu] = 0x7000u;
        words[object + 0x48u] = owner;
        words[object + 0x3cu] = 11u;
        words[object + 0x40u] = 22u;
        words[object + 0x44u] = 33u;
    }
    void BaseConstruct(std::uint32_t address) override {
        Check(address == 0xd000u, "quake child allocation");
        calls.emplace_back("base construct quake");
        if (throw_in_construct) throw std::runtime_error("quake construct");
    }
    void AddQuakeTask(std::uint32_t context, std::uint32_t task,
                      std::uint32_t timing_address) override {
        Check(context == 0x7000u && task == 0xd000u &&
                  timing_address == object + 0x0cu,
              "quake task timing and context");
        calls.emplace_back("add quake");
    }
    void InsertOwnerTask(std::uint32_t address,
                         std::uint32_t task) override {
        Check(address == collection, "quake owner collection");
        calls.emplace_back("insert quake");
        inserted_task = task;
    }
};

struct InitializeRuntime final : Runtime, pal4::ICameraSequenceInitializeRuntime {
    std::uint32_t length = 0u;
    std::uint32_t copy_destination = 0u;
    bool reset = false;
    bool reallocated = false;

    InitializeRuntime() {
        words[object + 0x48u] = 0x3000u;
        words[object + 0x50u] = 8u;
        bytes[0x2fffu] = 0u;
    }
    std::uint32_t CStringLength(std::uint32_t source) override {
        Check(source == 0x7000u, "camera filename source");
        calls.emplace_back("length");
        return length;
    }
    [[noreturn]] void ThrowLength() override {
        calls.emplace_back("throw length");
        throw std::length_error("camera filename too long");
    }
    void SetErrorFlag(std::uint32_t record,
                      std::uint8_t flag) override {
        Check(record == object + 0x44u && flag == 1u,
              "camera filename reset");
        reset = true;
        calls.emplace_back("reset text");
        words[object + 0x48u] = 0u;
        words[object + 0x4cu] = 0u;
        words[object + 0x50u] = 0u;
    }
    void ReallocateText(std::uint32_t record,
                        std::uint32_t size) override {
        Check(record == object + 0x44u && size == length,
              "camera filename allocation");
        reallocated = true;
        calls.emplace_back("reallocate text");
        words[object + 0x48u] = 0xa000u;
        words[object + 0x50u] = size;
    }
    void CopyBytes(std::uint32_t destination,
                   std::uint32_t source,
                   std::uint32_t size) override {
        Check(source == 0x7000u && size == length,
              "camera filename copy");
        calls.emplace_back("copy text");
        copy_destination = destination;
    }
};

struct QuakeRuntime final : Runtime, pal4::ICameraQuakeRuntime {
    std::uint32_t applied_time = 0u;
    std::uint32_t applied_amplitude = 0u;
    std::uint32_t applied_frequency = 0u;
    std::uint32_t active_state = 2u;

    QuakeRuntime() {
        words[object + 0x48u] = 0x2000u;
        words[object + 0x3cu] = 0x3000u;
        words[object + 0x40u] = 0x4000u;
        words[object + 0x44u] = 0x5000u;
    }
    std::uint32_t GetCameraSubsystem() override {
        calls.emplace_back("subsystem");
        return 0x6000u;
    }
    std::uint32_t GetActiveCameraInternalId(
        std::uint32_t subsystem) override {
        Check(subsystem == 0x6000u, "quake subsystem");
        calls.emplace_back("internal camera");
        return 0x7000u;
    }
    void InitializeCameraParameters(std::uint32_t camera,
                                    std::uint32_t time,
                                    std::uint32_t amplitude,
                                    std::uint32_t frequency) override {
        Check(camera == 0x7000u, "quake active camera");
        calls.emplace_back("initialize quake");
        applied_time = time;
        applied_amplitude = amplitude;
        applied_frequency = frequency;
    }
    void SetCameraActive(std::uint32_t camera,
                         std::uint32_t enabled) override {
        Check(camera == 0x7000u && enabled <= 1u,
              "quake active state");
        active_state = enabled;
        calls.emplace_back(enabled == 0u ? "deactivate" : "activate");
    }
};

struct TerminateSpawnRuntime final : SpawnRuntime,
                                     pal4::ICameraQuakeTerminateSpawnRuntime {
    std::uint32_t inserted_task = 0u;
    bool throw_in_construct = false;

    TerminateSpawnRuntime() {
        allocations = {0xd000u, 0xb000u};
        words[object + 0x40u] = 0x7000u;
        words[object + 0x3cu] = owner;
    }
    void BaseConstruct(std::uint32_t address) override {
        Check(address == 0xd000u, "termination child allocation");
        calls.emplace_back("base construct terminal");
        if (throw_in_construct) throw std::runtime_error("terminal construct");
    }
    void AddTerminateTask(std::uint32_t context, std::uint32_t task,
                          std::uint32_t timing_address) override {
        Check(context == 0x7000u && task == 0xd000u &&
                  timing_address == object + 0x0cu,
              "quake termination timing");
        calls.emplace_back("add terminal");
    }
    void InsertOwnerTask(std::uint32_t address,
                         std::uint32_t task) override {
        Check(address == collection, "termination owner collection");
        inserted_task = task;
        calls.emplace_back("insert terminal");
    }
};

std::size_t IndexOf(const std::vector<std::string>& values,
                    const std::string& needle) {
    for (std::size_t index = 0; index < values.size(); ++index)
        if (values[index] == needle) return index;
    throw std::runtime_error("missing call: " + needle);
}

}  // namespace

int main() {
    try {
        Runtime constructed;
        Check(pal4::CameraSequenceTask::Construct(Runtime::object,
                                                   constructed) ==
                  Runtime::object &&
                  constructed.calls.front() == "base construct" &&
                  constructed.calls[1] == "transient" &&
                  constructed.bytes.at(Runtime::object + 0x44u) == 0x5au &&
                  constructed.bytes.at(Runtime::object + 0x3cu) == 0u &&
                  constructed.words.at(Runtime::object + 0x48u) == 0u &&
                  constructed.words.at(Runtime::object + 0x4cu) == 0u &&
                  constructed.words.at(Runtime::object + 0x50u) == 0u &&
                  constructed.words.at(Runtime::object) == 0x84874cu,
              "camera task constructor uses its distinct vtable");

        for (std::uint8_t count : {std::uint8_t{0u},
                                    std::uint8_t{0xffu},
                                    std::uint8_t{2u}}) {
            Runtime destroyed;
            destroyed.words[Runtime::object + 0x48u] = 0x3000u;
            destroyed.bytes[0x2fffu] = count;
            Check(pal4::CameraSequenceTask::Destruct(Runtime::object,
                                                      destroyed) == 0x1234u &&
                      destroyed.words.at(Runtime::object) == 0x8486fcu &&
                      destroyed.words.at(Runtime::object + 0x48u) == 0u &&
                      destroyed.words.at(Runtime::object + 0x4cu) == 0u &&
                      destroyed.words.at(Runtime::object + 0x50u) == 0u &&
                      destroyed.calls.back() == "base destruct" &&
                      (count == 2u ?
                           destroyed.bytes.at(0x2fffu) == 1u &&
                               destroyed.freed.empty() :
                           destroyed.freed ==
                               (std::vector<std::uint32_t>{0x2fffu})),
                  "camera task destructor respects text reference count");
        }
        Runtime empty;
        empty.words[Runtime::object + 0x48u] = 0u;
        Check(pal4::CameraSequenceTask::Destruct(Runtime::object,
                                                  empty) == 0x1234u &&
                  empty.freed.empty(),
              "camera task destructor skips absent text");

        Runtime descriptor;
        Check(pal4::CameraSequenceTaskFactory::ConstructDescriptor(
                  0x2000u, descriptor) == 0x2000u &&
                  descriptor.words.at(0x2000u) == 0x848770u,
              "camera task factory descriptor");
        Check(pal4::CameraActiveObject::TaskCollection(0x4000u) == 0x4010u,
              "active camera collection offset");
        Runtime autoseek_construct;
        Check(pal4::CameraAutoSeekTask::Construct(Runtime::object,
                                                  autoseek_construct) ==
                  Runtime::object &&
                  autoseek_construct.calls.front() == "base construct" &&
                  autoseek_construct.words.at(Runtime::object) == 0x848774u &&
                  autoseek_construct.bytes.at(Runtime::object + 0x3cu) == 0u,
              "camera autoseek constructor");
        Runtime autoseek_destruct;
        Check(pal4::CameraAutoSeekTask::Destruct(Runtime::object,
                                                 autoseek_destruct) ==
                  0x1234u &&
                  autoseek_destruct.words.at(Runtime::object) == 0x8486fcu &&
                  autoseek_destruct.bytes.at(Runtime::object + 0x3cu) == 0u &&
                  autoseek_destruct.calls.back() == "base destruct",
              "camera autoseek destructor");
        for (auto flags : {std::uint8_t{0u}, std::uint8_t{1u},
                           std::uint8_t{2u}, std::uint8_t{3u}}) {
            Runtime deleting;
            Check(pal4::CameraAutoSeekTask::Delete(Runtime::object,
                                                    flags, deleting) ==
                      Runtime::object &&
                      deleting.freed.size() == (flags & 1u) &&
                      ((flags & 1u) == 0u ?
                           deleting.calls.back() == "base destruct" :
                           deleting.calls.back() == "free"),
                  "camera autoseek deleting destructor flags");
        }

        PlaybackRuntime no_owner;
        no_owner.words[Runtime::object + 0x40u] = 0u;
        Check(pal4::CameraSequenceTask::Enter(Runtime::object,
                                               no_owner) == 0u &&
                  no_owner.calls.size() == 1u,
              "camera enter skips absent owner");

        PlaybackRuntime load_failure;
        load_failure.load_result = false;
        load_failure.words[Runtime::object + 0x48u] = 0u;
        Check(pal4::CameraSequenceTask::Enter(Runtime::object,
                                               load_failure) == 0u &&
                  load_failure.loaded_filename == 0x8600u &&
                  load_failure.bytes.at(Runtime::object + 0x3cu) == 1u &&
                  IndexOf(load_failure.calls, "empty string") <
                      IndexOf(load_failure.calls, "load") &&
                  IndexOf(load_failure.calls, "load") <
                      load_failure.calls.size() - 1u,
              "camera enter load failure and empty filename");
        Check(std::find(load_failure.calls.begin(),
                        load_failure.calls.end(), "subsystem") ==
                  load_failure.calls.end(),
              "camera load failure does not initialize camera");

        PlaybackRuntime no_records;
        Check(pal4::CameraSequenceTask::Enter(Runtime::object,
                                               no_records) == 1u &&
                  no_records.added_tasks.empty() &&
                  no_records.words.at(Runtime::object + 0x58u) == 0x8300u &&
                  no_records.words.at(Runtime::object + 0x5cu) == 0x8400u &&
                  no_records.words.at(PlaybackRuntime::property + 0x1a8u) ==
                      0u &&
                  no_records.bytes.at(Runtime::object + 0x3cu) == 0u &&
                  IndexOf(no_records.calls, "cleanup") <
                      IndexOf(no_records.calls, "first") &&
                  IndexOf(no_records.calls, "first") <
                      IndexOf(no_records.calls, "processed") &&
                  IndexOf(no_records.calls, "processed") <
                      IndexOf(no_records.calls, "reset 0"),
              "camera enter with no records saves state");

        PlaybackRuntime multiple;
        multiple.records = {0xa000u, 0xa100u};
        multiple.created_tasks = {0xb000u, 0xb100u};
        multiple.words[0xa000u + 0x34u] = 21u;
        multiple.words[0xa000u + 0x38u] = 0u;
        multiple.words[0xa100u + 0x34u] = 37u;
        multiple.words[0xa100u + 0x38u] = 1u;
        Check(pal4::CameraSequenceTask::Enter(Runtime::object,
                                               multiple) == 1u &&
                  multiple.added_tasks ==
                      (std::vector<std::uint32_t>{0xb000u, 0xb100u}) &&
                  multiple.timings.size() == 2u &&
                  multiple.timings[0].first == 1u &&
                  multiple.timings[0].second == 0u &&
                  multiple.timings[0].duration == 21u &&
                  multiple.timings[1].duration == 0u &&
                  multiple.words.at(0xb000u + 0x48u) ==
                      PlaybackRuntime::property &&
                  multiple.words.at(0xb000u + 0x4cu) == 0xa000u &&
                  multiple.words.at(0xb000u + 0x44u) == 21u &&
                  multiple.words.at(0xb100u + 0x4cu) == 0xa100u &&
                  multiple.words.at(0xb100u + 0x44u) == 37u &&
                  IndexOf(multiple.calls, "create") <
                      IndexOf(multiple.calls, "add") &&
                  IndexOf(multiple.calls, "add") <
                      IndexOf(multiple.calls, "next"),
              "camera enter creates and configures every record");

        PlaybackRuntime no_update_owner;
        no_update_owner.words[Runtime::object + 0x40u] = 0u;
        no_update_owner.bytes[Runtime::object + 0x3cu] = 1u;
        Check(pal4::CameraSequenceTask::Update(Runtime::object,
                                                no_update_owner) == 0u &&
                  no_update_owner.bytes.at(Runtime::object + 0x3cu) == 0u,
              "camera update clears completion without owner");
        PlaybackRuntime already_complete;
        already_complete.bytes[Runtime::object + 0x3cu] = 1u;
        Check(pal4::CameraSequenceTask::Update(Runtime::object,
                                                already_complete) == 0u &&
                  std::find(already_complete.calls.begin(),
                            already_complete.calls.end(), "subsystem") ==
                      already_complete.calls.end(),
              "camera update skips completed task");
        PlaybackRuntime still_running;
        Check(pal4::CameraSequenceTask::Update(Runtime::object,
                                                still_running) == 1u &&
                  still_running.bytes.at(Runtime::object + 0x3cu) == 0u,
              "camera update waits for active camera");
        PlaybackRuntime finished;
        finished.bytes[PlaybackRuntime::active + 0x1cu] = 1u;
        Check(pal4::CameraSequenceTask::Update(Runtime::object,
                                                finished) == 1u &&
                  finished.bytes.at(Runtime::object + 0x3cu) == 1u &&
                  finished.words.at(PlaybackRuntime::property + 0x1a8u) ==
                      0x8400u &&
                  IndexOf(finished.calls, "reset 33536") <
                      IndexOf(finished.calls, "internal"),
              "camera update restores actor and camera state");
        PlaybackRuntime autoseek;
        autoseek.bytes[PlaybackRuntime::active + 0x1cu] = 1u;
        autoseek.words[Runtime::object + 0x54u] = 1u;
        Check(pal4::CameraSequenceTask::Update(Runtime::object,
                                                autoseek) == 1u &&
                  IndexOf(autoseek.calls, "mode") <
                      IndexOf(autoseek.calls, "look-at"),
              "camera update restores mode and look-at for autoseek");
        PlaybackRuntime no_autoseek_owner;
        no_autoseek_owner.words[Runtime::object + 0x40u] = 0u;
        Check(pal4::CameraAutoSeekTask::Enter(Runtime::object,
                                               no_autoseek_owner) == 0u &&
                  no_autoseek_owner.calls.size() == 1u,
              "camera autoseek enter requires owner");
        PlaybackRuntime activate_autoseek;
        Check(pal4::CameraAutoSeekTask::Enter(Runtime::object,
                                               activate_autoseek) == 1u &&
                  activate_autoseek.bytes.at(Runtime::object + 0x3cu) == 1u &&
                  std::count(activate_autoseek.calls.begin(),
                             activate_autoseek.calls.end(),
                             "subsystem") == 3 &&
                  IndexOf(activate_autoseek.calls, "active") <
                      IndexOf(activate_autoseek.calls, "mode") &&
                  IndexOf(activate_autoseek.calls, "mode") <
                      IndexOf(activate_autoseek.calls, "look-at"),
              "camera autoseek activates mode and look-at in order");

        SpawnRuntime no_context;
        no_context.words[Runtime::object + 0x60u] = 0u;
        Check(pal4::CameraSequenceTask::SpawnFollowup(Runtime::object,
                                                       no_context) == 0u &&
                  no_context.calls.empty(),
              "camera follow-up requires context");
        SpawnRuntime no_spawn_owner;
        no_spawn_owner.words[Runtime::object + 0x40u] = 0u;
        Check(pal4::CameraSequenceTask::SpawnFollowup(Runtime::object,
                                                       no_spawn_owner) == 0u &&
                  no_spawn_owner.calls.empty(),
              "camera follow-up requires owner");
        SpawnRuntime previous;
        previous.words[Runtime::object + 0x30u] = 0xc000u;
        Check(pal4::CameraSequenceTask::SpawnFollowup(Runtime::object,
                                                       previous) == 1u &&
                  previous.calls ==
                      (std::vector<std::string>{"previous"}),
              "camera follow-up notifies previous task");
        SpawnRuntime spare;
        Check(pal4::CameraSequenceTask::SpawnFollowup(Runtime::object,
                                                       spare) == 1u &&
                  spare.allocation_sizes ==
                      (std::vector<std::uint32_t>{0x40u}) &&
                  spare.added == 0xb000u &&
                  spare.helper_owner == SpawnRuntime::owner &&
                  spare.words.at(0x9004u) == 0xb000u &&
                  spare.words.at(SpawnRuntime::collection + 8u) == 0x9008u &&
                  spare.freed.empty() &&
                  IndexOf(spare.calls, "add") <
                      IndexOf(spare.calls, "set owner") &&
                  IndexOf(spare.calls, "set owner") <
                      IndexOf(spare.calls, "processed"),
              "camera follow-up appends with spare capacity");
        SpawnRuntime growth;
        growth.words[SpawnRuntime::collection + 8u] = 0x9008u;
        growth.words[SpawnRuntime::collection + 12u] = 0x9008u;
        growth.words[0x9004u] = 0xbbbb0000u;
        growth.allocations.push_back(0xa000u);
        Check(pal4::CameraSequenceTask::SpawnFollowup(Runtime::object,
                                                       growth) == 1u &&
                  growth.allocation_sizes ==
                      (std::vector<std::uint32_t>{0x40u, 16u}) &&
                  growth.words.at(0xa000u) == 0xaaaa0000u &&
                  growth.words.at(0xa004u) == 0xbbbb0000u &&
                  growth.words.at(0xa008u) == 0xb000u &&
                  growth.words.at(SpawnRuntime::collection + 4u) == 0xa000u &&
                  growth.words.at(SpawnRuntime::collection + 8u) == 0xa00cu &&
                  growth.words.at(SpawnRuntime::collection + 12u) == 0xa010u &&
                  growth.freed ==
                      (std::vector<std::uint32_t>{0x9000u}),
              "camera follow-up grows owner pointer collection");
        SpawnRuntime auto_no_context;
        auto_no_context.words[Runtime::object + 0x44u] = 0u;
        Check(pal4::CameraAutoSeekTask::SpawnFollowup(
                  Runtime::object, auto_no_context) == 0u &&
                  auto_no_context.calls.empty(),
              "camera autoseek follow-up checks context at +0x44");
        SpawnRuntime auto_previous;
        auto_previous.words[Runtime::object + 0x44u] = 0x7000u;
        auto_previous.words[Runtime::object + 0x60u] = 0u;
        auto_previous.words[Runtime::object + 0x30u] = 0xc000u;
        Check(pal4::CameraAutoSeekTask::SpawnFollowup(
                  Runtime::object, auto_previous) == 1u &&
                  auto_previous.calls ==
                      (std::vector<std::string>{"previous"}),
              "camera autoseek follow-up notifies previous task");
        SpawnRuntime auto_growth;
        auto_growth.words[Runtime::object + 0x44u] = 0x7000u;
        auto_growth.words[Runtime::object + 0x60u] = 0u;
        auto_growth.words[SpawnRuntime::collection + 8u] = 0x9008u;
        auto_growth.words[SpawnRuntime::collection + 12u] = 0x9008u;
        auto_growth.words[0x9004u] = 0xbbbb0000u;
        auto_growth.allocations.push_back(0xa000u);
        Check(pal4::CameraAutoSeekTask::SpawnFollowup(
                  Runtime::object, auto_growth) == 1u &&
                  auto_growth.words.at(0xa008u) == 0xb000u &&
                  auto_growth.freed ==
                      (std::vector<std::uint32_t>{0x9000u}) &&
                  auto_growth.calls.back() == "processed",
              "camera autoseek follow-up shares collection growth");

        InitializeRuntime shared_empty;
        shared_empty.bytes[0x2fffu] = 2u;
        Check(pal4::CameraSequenceTask::Initialize(
                  Runtime::object, 0x2000u, 0x7000u, 0x4000u,
                  0x6000u, shared_empty) == 0x3000u &&
                  shared_empty.bytes.at(0x2fffu) == 1u &&
                  shared_empty.words.at(Runtime::object + 0x48u) == 0u &&
                  shared_empty.words.at(Runtime::object + 0x50u) == 0u &&
                  shared_empty.words.at(Runtime::object + 0x54u) == 0x4000u &&
                  shared_empty.words.at(Runtime::object + 0x60u) == 0x6000u &&
                  shared_empty.words.count(Runtime::object + 0x58u) == 0u,
              "camera init clears shared empty filename and writes context");
        for (auto count : {std::uint8_t{0u}, std::uint8_t{0xffu}}) {
            InitializeRuntime empty_filename;
            empty_filename.bytes[0x2fffu] = count;
            Check(pal4::CameraSequenceTask::Initialize(
                      Runtime::object, 0x2000u, 0x7000u, 0x4000u,
                      0x6000u, empty_filename) == 0x4000u &&
                      empty_filename.reset &&
                      empty_filename.words.at(Runtime::object + 0x60u) ==
                          0x6000u,
                  "camera init resets unique empty filename");
        }
        InitializeRuntime reused;
        reused.length = 3u;
        Check(pal4::CameraSequenceTask::Initialize(
                  Runtime::object, 0x2000u, 0x7000u, 0x4000u,
                  0x6000u, reused) == 0x6000u &&
                  !reused.reset && !reused.reallocated &&
                  reused.copy_destination == 0x3000u &&
                  reused.bytes.at(0x3003u) == 0u &&
                  reused.words.at(Runtime::object + 0x4cu) == 3u &&
                  reused.words.at(Runtime::object + 0x60u) == 0x6000u,
              "camera init reuses filename storage");
        InitializeRuntime shared_name;
        shared_name.length = 3u;
        shared_name.bytes[0x2fffu] = 2u;
        Check(pal4::CameraSequenceTask::Initialize(
                  Runtime::object, 0x2000u, 0x7000u, 0x4000u,
                  0x6000u, shared_name) == 0x6000u &&
                  !shared_name.reset && shared_name.reallocated &&
                  shared_name.copy_destination == 0xa000u,
              "camera init reallocates shared filename");
        for (auto capacity : {2u, 32u}) {
            InitializeRuntime small;
            small.length = 3u;
            small.words[Runtime::object + 0x50u] = capacity;
            Check(pal4::CameraSequenceTask::Initialize(
                      Runtime::object, 0x2000u, 0x7000u, 0x4000u,
                      0x6000u, small) == 0x6000u &&
                      small.reset && small.reallocated &&
                      small.copy_destination == 0xa000u,
                  "camera init resets invalid filename capacity");
        }
        InitializeRuntime oversized;
        oversized.length = 0xfffffffeu;
        bool length_thrown = false;
        try {
            (void)pal4::CameraSequenceTask::Initialize(
                Runtime::object, 0x2000u, 0x7000u, 0x4000u,
                0x6000u, oversized);
        } catch (const std::length_error&) {
            length_thrown = true;
        }
        Check(length_thrown &&
                  oversized.calls.back() == "throw length" &&
                  oversized.words.at(Runtime::object + 0x40u) == 0x2000u &&
                  oversized.bytes.at(Runtime::object + 0x3cu) == 0u,
              "camera init checks filename length after owner reset");

        Runtime owner_context;
        Check(pal4::GomTaskOwnerContext::Initialize(
                  Runtime::object, 0x2000u, 0x3000u,
                  owner_context) == 0x2000u &&
                  owner_context.words.at(Runtime::object + 0x40u) == 0x2000u &&
                  owner_context.bytes.at(Runtime::object + 0x3cu) == 0u &&
                  owner_context.words.at(Runtime::object + 0x44u) == 0x3000u &&
                  owner_context.calls ==
                      (std::vector<std::string>{"word 4160", "byte 4156",
                                                "word 4164"}),
              "generic owner-context task initializer");
        Runtime plain_owner_context;
        Check(pal4::GomTaskOwnerContext::InitializePlain(
                  Runtime::object, 0x2000u, 0x3000u,
                  plain_owner_context) == 0x2000u &&
                  plain_owner_context.words.at(Runtime::object + 0x3cu) ==
                      0x2000u &&
                  plain_owner_context.words.at(Runtime::object + 0x40u) ==
                      0x3000u &&
                  plain_owner_context.calls ==
                      (std::vector<std::string>{"word 4156", "word 4160"}),
              "plain owner-context task initializer");
        Runtime quake_construct;
        Check(pal4::CameraQuakeTask::Construct(Runtime::object,
                                               quake_construct) ==
                  Runtime::object &&
                  quake_construct.calls.front() == "base construct" &&
                  quake_construct.words.at(Runtime::object) == 0x848798u &&
                  quake_construct.words.at(Runtime::object + 0x3cu) == 0u &&
                  quake_construct.words.at(Runtime::object + 0x48u) == 0u &&
                  quake_construct.words.at(Runtime::object + 0x4cu) == 0u,
              "camera quake constructor");
        Runtime quake_destruct;
        Check(pal4::CameraQuakeTask::Destruct(Runtime::object,
                                              quake_destruct) == 0x1234u &&
                  quake_destruct.words.at(Runtime::object) == 0x8486fcu &&
                  quake_destruct.words.at(Runtime::object + 0x3cu) == 0u &&
                  quake_destruct.words.at(Runtime::object + 0x48u) == 0u &&
                  quake_destruct.words.at(Runtime::object + 0x4cu) == 0u &&
                  quake_destruct.calls.back() == "base destruct",
              "camera quake destructor");
        QuakeRuntime quake_missing_owner;
        quake_missing_owner.words[Runtime::object + 0x48u] = 0u;
        Check(!pal4::CameraQuakeTask::ValidateOwner(
                  Runtime::object, quake_missing_owner) &&
                  pal4::CameraQuakeTask::Enter(
                      Runtime::object, quake_missing_owner) == 0u &&
                  quake_missing_owner.calls.empty(),
              "camera quake requires owner");
        QuakeRuntime quake;
        Check(pal4::CameraQuakeTask::ValidateOwner(Runtime::object, quake) &&
                  pal4::CameraQuakeTask::Enter(Runtime::object,
                                                quake) == 1u &&
                  quake.applied_time == 0x3000u &&
                  quake.applied_amplitude == 0x4000u &&
                  quake.applied_frequency == 0x5000u &&
                  quake.calls ==
                      (std::vector<std::string>{"subsystem", "internal camera",
                                                "initialize quake", "subsystem",
                                                "internal camera", "activate"}),
              "camera quake initializes parameters then activates");
        QuakeSpawnRuntime quake_no_context;
        quake_no_context.words[Runtime::object + 0x4cu] = 0u;
        Check(pal4::CameraQuakeTask::SpawnFollowup(
                  Runtime::object, quake_no_context) == 0u &&
                  quake_no_context.calls.empty(),
              "quake follow-up requires context");
        QuakeSpawnRuntime quake_no_owner;
        quake_no_owner.words[Runtime::object + 0x48u] = 0u;
        Check(pal4::CameraQuakeTask::SpawnFollowup(
                  Runtime::object, quake_no_owner) == 0u &&
                  quake_no_owner.calls.empty(),
              "quake follow-up requires owner");
        QuakeSpawnRuntime quake_previous;
        quake_previous.words[Runtime::object + 0x30u] = 0xc000u;
        Check(pal4::CameraQuakeTask::SpawnFollowup(
                  Runtime::object, quake_previous) == 1u &&
                  quake_previous.inserted_task == 0xd000u &&
                  quake_previous.words.at(0xd000u + 0x48u) ==
                      QuakeSpawnRuntime::owner &&
                  quake_previous.words.at(0xd000u + 0x3cu) == 11u &&
                  quake_previous.words.at(0xd000u + 0x40u) == 22u &&
                  quake_previous.words.at(0xd000u + 0x44u) == 33u &&
                  quake_previous.words.at(0xd000u + 0x4cu) == 0u &&
                  IndexOf(quake_previous.calls, "add quake") <
                      IndexOf(quake_previous.calls, "insert quake") &&
                  IndexOf(quake_previous.calls, "insert quake") <
                      IndexOf(quake_previous.calls, "previous"),
              "quake child is inserted before previous callback");
        QuakeSpawnRuntime quake_helper;
        Check(pal4::CameraQuakeTask::SpawnFollowup(
                  Runtime::object, quake_helper) == 1u &&
                  quake_helper.inserted_task == 0xd000u &&
                  quake_helper.added == 0xb000u &&
                  quake_helper.words.at(0x9004u) == 0xb000u &&
                  quake_helper.allocation_sizes ==
                      (std::vector<std::uint32_t>{0x50u, 0x40u}) &&
                  IndexOf(quake_helper.calls, "insert quake") <
                      IndexOf(quake_helper.calls, "add") &&
                  quake_helper.calls.back() == "processed",
              "quake follow-up creates child then helper");
        QuakeSpawnRuntime quake_null_child;
        quake_null_child.allocations = {0u, 0xb000u};
        Check(pal4::CameraQuakeTask::SpawnFollowup(
                  Runtime::object, quake_null_child) == 1u &&
                  quake_null_child.inserted_task == 0u &&
                  std::find(quake_null_child.calls.begin(),
                            quake_null_child.calls.end(), "add quake") ==
                      quake_null_child.calls.end(),
              "quake follow-up skips registration for null child");
        Runtime quake_descriptor;
        Check(pal4::CameraQuakeTaskFactory::ConstructDescriptor(
                  0x2000u, quake_descriptor) == 0x2000u &&
                  quake_descriptor.words.at(0x2000u) == 0x8487bcu,
              "quake factory descriptor");
        Runtime quake_init;
        Check(pal4::CameraQuakeTask::Initialize(
                  Runtime::object, 0x2000u, 11u, 22u, 33u, 0x7000u,
                  quake_init) == 0x7000u &&
                  quake_init.words.at(Runtime::object + 0x48u) == 0x2000u &&
                  quake_init.words.at(Runtime::object + 0x3cu) == 11u &&
                  quake_init.words.at(Runtime::object + 0x40u) == 22u &&
                  quake_init.words.at(Runtime::object + 0x44u) == 33u &&
                  quake_init.words.at(Runtime::object + 0x4cu) == 0x7000u,
              "quake runtime initializer");
        QuakeSpawnRuntime factory_success;
        Check(pal4::CameraQuakeTaskFactory::Create(factory_success) ==
                  0xd000u &&
                  factory_success.allocation_sizes ==
                      (std::vector<std::uint32_t>{0x50u}) &&
                  factory_success.words.at(0xd000u) == 0x848798u,
              "quake factory allocation and construction");
        QuakeSpawnRuntime factory_null;
        factory_null.allocations = {0u};
        Check(pal4::CameraQuakeTaskFactory::Create(factory_null) == 0u &&
                  factory_null.calls ==
                      (std::vector<std::string>{"allocate"}),
              "quake factory null allocation");
        QuakeSpawnRuntime factory_failure;
        factory_failure.throw_in_construct = true;
        bool construct_thrown = false;
        try {
            (void)pal4::CameraQuakeTaskFactory::Create(factory_failure);
        } catch (const std::runtime_error&) {
            construct_thrown = true;
        }
        Check(construct_thrown && factory_failure.freed ==
                                      (std::vector<std::uint32_t>{0xd000u}) &&
                  factory_failure.calls.back() == "free",
              "quake factory releases failed construction");

        Runtime terminal_construct;
        Check(pal4::CameraQuakeTerminateTask::Construct(
                  Runtime::object, terminal_construct) == Runtime::object &&
                  terminal_construct.calls.front() == "base construct" &&
                  terminal_construct.words.at(Runtime::object) == 0x8487c0u &&
                  terminal_construct.words.at(Runtime::object + 0x3cu) == 0u &&
                  terminal_construct.words.at(Runtime::object + 0x40u) == 0u,
              "quake termination constructor");
        Runtime terminal_destruct;
        Check(pal4::CameraQuakeTerminateTask::Destruct(
                  Runtime::object, terminal_destruct) == 0x1234u &&
                  terminal_destruct.words.at(Runtime::object) == 0x8486fcu &&
                  terminal_destruct.words.at(Runtime::object + 0x3cu) == 0u &&
                  terminal_destruct.words.at(Runtime::object + 0x40u) == 0u &&
                  terminal_destruct.calls.back() == "base destruct",
              "quake termination destructor");
        for (auto flags : {std::uint8_t{0u}, std::uint8_t{1u},
                           std::uint8_t{2u}, std::uint8_t{3u}}) {
            Runtime deleting_terminal;
            Check(pal4::CameraQuakeTerminateTask::Delete(
                      Runtime::object, flags, deleting_terminal) ==
                      Runtime::object &&
                      deleting_terminal.freed.size() == (flags & 1u),
                  "quake termination deleting destructor flag");
        }
        QuakeRuntime deactivate;
        Check(pal4::CameraQuakeTerminateTask::Enter(deactivate) == 1u &&
                  deactivate.active_state == 0u &&
                  deactivate.calls ==
                      (std::vector<std::string>{"subsystem",
                                                "internal camera",
                                                "deactivate"}),
              "quake termination deactivates active camera");
        Runtime terminal_owner;
        terminal_owner.words[Runtime::object + 0x3cu] = 0u;
        Check(!pal4::CameraQuakeTerminateTask::ValidateOwner(
                  Runtime::object, terminal_owner),
              "quake termination missing owner");
        terminal_owner.words[Runtime::object + 0x3cu] = 0x8000u;
        Check(pal4::CameraQuakeTerminateTask::ValidateOwner(
                  Runtime::object, terminal_owner),
              "quake termination present owner");
        TerminateSpawnRuntime terminal_no_context;
        terminal_no_context.words[Runtime::object + 0x40u] = 0u;
        Check(pal4::CameraQuakeTerminateTask::SpawnFollowup(
                  Runtime::object, terminal_no_context) == 0u &&
                  terminal_no_context.calls.empty(),
              "quake termination requires context");
        TerminateSpawnRuntime terminal_no_owner;
        terminal_no_owner.words[Runtime::object + 0x3cu] = 0u;
        Check(pal4::CameraQuakeTerminateTask::SpawnFollowup(
                  Runtime::object, terminal_no_owner) == 0u &&
                  terminal_no_owner.calls.empty(),
              "quake termination requires owner");
        TerminateSpawnRuntime terminal_previous;
        terminal_previous.words[Runtime::object + 0x30u] = 0xc000u;
        Check(pal4::CameraQuakeTerminateTask::SpawnFollowup(
                  Runtime::object, terminal_previous) == 1u &&
                  terminal_previous.inserted_task == 0xd000u &&
                  terminal_previous.words.at(0xd000u) == 0x8487c0u &&
                  IndexOf(terminal_previous.calls, "add terminal") <
                      IndexOf(terminal_previous.calls,
                              "insert terminal") &&
                  IndexOf(terminal_previous.calls, "insert terminal") <
                      IndexOf(terminal_previous.calls, "previous"),
              "quake termination child precedes previous callback");
        TerminateSpawnRuntime terminal_helper;
        Check(pal4::CameraQuakeTerminateTask::SpawnFollowup(
                  Runtime::object, terminal_helper) == 1u &&
                  terminal_helper.inserted_task == 0xd000u &&
                  terminal_helper.added == 0xb000u &&
                  terminal_helper.words.at(0x9004u) == 0xb000u &&
                  terminal_helper.allocation_sizes ==
                      (std::vector<std::uint32_t>{0x44u, 0x40u}) &&
                  terminal_helper.calls.back() == "processed",
              "quake termination spawns helper after child");
        TerminateSpawnRuntime terminal_null_child;
        terminal_null_child.allocations = {0u, 0xb000u};
        Check(pal4::CameraQuakeTerminateTask::SpawnFollowup(
                  Runtime::object, terminal_null_child) == 1u &&
                  terminal_null_child.inserted_task == 0u &&
                  std::find(terminal_null_child.calls.begin(),
                            terminal_null_child.calls.end(),
                            "add terminal") ==
                      terminal_null_child.calls.end(),
              "quake termination skips null child registration");
        Runtime terminal_descriptor;
        Check(pal4::CameraQuakeTerminateTaskFactory::ConstructDescriptor(
                  0x2000u, terminal_descriptor) == 0x2000u &&
                  terminal_descriptor.words.at(0x2000u) == 0x8487e4u,
              "quake termination factory descriptor");
        TerminateSpawnRuntime terminal_factory_success;
        Check(pal4::CameraQuakeTerminateTaskFactory::Create(
                  terminal_factory_success) == 0xd000u &&
                  terminal_factory_success.allocation_sizes ==
                      (std::vector<std::uint32_t>{0x44u}) &&
                  terminal_factory_success.words.at(0xd000u) == 0x8487c0u,
              "quake termination factory construction");
        TerminateSpawnRuntime terminal_factory_null;
        terminal_factory_null.allocations = {0u};
        Check(pal4::CameraQuakeTerminateTaskFactory::Create(
                  terminal_factory_null) == 0u &&
                  terminal_factory_null.calls ==
                      (std::vector<std::string>{"allocate"}),
              "quake termination factory null allocation");
        TerminateSpawnRuntime terminal_factory_failure;
        terminal_factory_failure.throw_in_construct = true;
        bool terminal_thrown = false;
        try {
            (void)pal4::CameraQuakeTerminateTaskFactory::Create(
                terminal_factory_failure);
        } catch (const std::runtime_error&) {
            terminal_thrown = true;
        }
        Check(terminal_thrown && terminal_factory_failure.freed ==
                                     (std::vector<std::uint32_t>{0xd000u}) &&
                  terminal_factory_failure.calls.back() == "free",
              "quake termination factory frees failed construction");
        std::cout << "camera_sequence_task_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
