#include "pal4/get_item_task.h"

#include <algorithm>
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

struct Runtime : virtual pal4::IGetItemTaskRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t next_text = 0x2000u;
    std::uint32_t window = 0x3000u;
    bool item_exists = true;
    bool equipment_exists = true;
    bool visible = true;
    bool throw_on_format = false;
    std::string_view sound;
    std::string_view format;
    std::string_view name;
    std::uint32_t formatted_count = 0u;
    std::uint32_t granted_id = 0u;
    std::uint32_t granted_amount = 0u;

    Runtime(std::uint32_t kind = 0u) {
        words[object + 0x40u] = kind;
        words[object + 0x44u] = 42u;
        words[object + 0x48u] = 7u;
        words[object + 0x4cu] = 0x4000u;
    }
    void BaseConstruct(std::uint32_t address) override {
        Check(address == object, "base constructor address");
        calls.emplace_back("base construct");
    }
    std::uint32_t BaseDestruct(std::uint32_t address) override {
        Check(address == object, "base destructor address");
        calls.emplace_back("base destruct");
        return 0x55u;
    }
    void FreeMemory(std::uint32_t address) override {
        Check(address == object || address == 0xa000u,
              "free address");
        calls.emplace_back("free");
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
    std::uint32_t BeginText() override {
        const auto result = next_text++;
        calls.emplace_back("begin text");
        return result;
    }
    void ReleaseText(std::uint32_t text) override {
        Check(text == 0x2000u || text == 0x2001u,
              "released text address");
        calls.emplace_back(text == 0x2001u ? "release hint" :
                                               "release formatted");
    }
    bool HasItem(std::uint32_t id) override {
        Check(id == 42u, "item id");
        calls.emplace_back("has item");
        return item_exists;
    }
    bool HasEquipment(std::uint32_t id) override {
        Check(id == 42u, "equipment id");
        calls.emplace_back("has equipment");
        return equipment_exists;
    }
    std::string_view ItemName(std::uint32_t id) override {
        Check(id == 42u, "item name id");
        calls.emplace_back("item name");
        return "Herb";
    }
    std::string_view EquipmentName(std::uint32_t id) override {
        Check(id == 42u, "equipment name id");
        calls.emplace_back("equipment name");
        return "Sword";
    }
    void GrantItem(std::uint32_t id,
                   std::uint32_t count) override {
        calls.emplace_back("grant item");
        granted_id = id;
        granted_amount = count;
    }
    void GrantEquipment(std::uint32_t id) override {
        calls.emplace_back("grant equipment");
        granted_id = id;
    }
    void AddMoney(std::uint32_t amount) override {
        calls.emplace_back("add money");
        granted_amount = amount;
    }
    std::string_view GameString(std::uint32_t id) override {
        Check(id == 0xfbu || id == 0xc2u, "localized string id");
        calls.emplace_back("game string");
        return id == 0xfbu ? "received %s x%d" : "money %d";
    }
    void FormatText(std::uint32_t text, std::string_view pattern,
                    std::string_view value, std::uint32_t count) override {
        Check(text == 0x2000u, "formatted text address");
        calls.emplace_back("format");
        if (throw_on_format) throw std::runtime_error("format failed");
        format = pattern;
        name = value;
        formatted_count = count;
    }
    void AppendText(std::uint32_t destination,
                    std::uint32_t source) override {
        Check(destination == 0x2001u && source == 0x2000u,
              "append text addresses");
        calls.emplace_back("append");
    }
    void PlaySound(std::string_view path) override {
        calls.emplace_back("sound");
        sound = path;
    }
    std::uint32_t CreateHintWindow() override {
        calls.emplace_back("create hint");
        return window;
    }
    void ShowHint(std::uint32_t address, std::uint32_t text,
                  float x, float y, float opacity,
                  bool first, bool second) override {
        Check(address == window && text == 0x2001u &&
                  x == 0.5f && y == 0.15f && opacity == 1.0f &&
                  !first && second,
              "hint show arguments");
        calls.emplace_back("show hint");
    }
    bool HintVisible(std::uint32_t address) override {
        Check(address == window, "visible hint address");
        calls.emplace_back("hint visible");
        return visible;
    }
};

struct SpawnRuntime final : Runtime, pal4::IGetItemTaskSpawnRuntime {
    std::vector<std::uint32_t> allocation_sizes;
    std::uint32_t helper_memory = 0xb000u;
    std::uint32_t added_helper = 0u;
    std::uint32_t owner_assigned = 0u;
    std::uint32_t processed = 0u;

    SpawnRuntime() {
        words[object + 0x30u] = 0u;
        words[object + 0x50u] = 0x5000u;
        words[0x42a4u] = 0xa000u;
        words[0x42a8u] = 0xa004u;
        words[0x42acu] = 0xa008u;
        words[0xa000u] = 0xaaaa0000u;
    }
    std::uint32_t AllocateMemory(std::uint32_t bytes) override {
        allocation_sizes.push_back(bytes);
        calls.emplace_back("allocate");
        return allocation_sizes.size() == 1u ? helper_memory : 0xe000u;
    }
    std::uint32_t ConstructHelperTask(std::uint32_t memory) override {
        Check(memory == 0xb000u, "helper construction address");
        calls.emplace_back("construct helper");
        return memory;
    }
    void AddHelperTask(std::uint32_t context,
                       std::uint32_t helper,
                       std::uint32_t first,
                       std::uint32_t second,
                       std::uint32_t third) override {
        Check(context == 0x5000u && helper == 0xb000u &&
                  first == 1u && second == 0u && third == 0u,
              "helper timing");
        calls.emplace_back("add helper");
        added_helper = helper;
    }
    void SetHelperOwner(std::uint32_t helper,
                        std::uint32_t owner) override {
        Check((helper == 0xb000u || helper == 0u) &&
                  owner == 0x4000u, "helper owner");
        calls.emplace_back("set owner");
        owner_assigned = owner;
    }
    void NotifyPreviousTask(std::uint32_t previous) override {
        Check(previous == 0x6000u, "previous task");
        calls.emplace_back("previous");
    }
    void MarkProcessed(std::uint32_t context) override {
        Check(context == 0x5000u, "processed context");
        calls.emplace_back("processed");
        processed = context;
    }
};

void CheckLifecycle() {
    Runtime runtime;
    Check(pal4::GetItemTask::Construct(Runtime::object, runtime) ==
              Runtime::object &&
              runtime.words.at(Runtime::object) == 0x848810u &&
              runtime.bytes.at(Runtime::object + 0x3cu) == 0u &&
              runtime.words.at(Runtime::object + 0x40u) == 0u &&
              runtime.words.at(Runtime::object + 0x44u) == 0u &&
              runtime.words.at(Runtime::object + 0x48u) == 0u &&
              runtime.calls.front() == "base construct",
          "getitem constructor fields");
    for (const auto flags : {0u, 1u, 2u, 3u}) {
        Runtime deleting;
        Check(pal4::GetItemTask::Delete(Runtime::object,
                    static_cast<std::uint8_t>(flags), deleting) ==
                  Runtime::object &&
                  deleting.words.at(Runtime::object) == 0x8486fcu &&
                  deleting.calls[deleting.calls.size() -
                                 ((flags & 1u) ? 2u : 1u)] ==
                      "base destruct" &&
                  (deleting.calls.back() == "free") ==
                      ((flags & 1u) != 0u),
              "getitem deleting destructor flag gate");
    }
    Runtime destructing;
    Check(pal4::GetItemTask::Destruct(Runtime::object, destructing) ==
              0x55u && destructing.calls.back() == "base destruct",
          "getitem destructor forwards base result");
}

void CheckEnter() {
    Runtime no_context;
    no_context.words[Runtime::object + 0x4cu] = 0u;
    Check(pal4::GetItemTask::Enter(Runtime::object, no_context) ==
              0u && no_context.calls.empty(),
          "getitem enter context gate");
    for (const auto kind : {0u, 1u, 2u, 3u}) {
        Runtime runtime(kind);
        Check(pal4::GetItemTask::Enter(Runtime::object, runtime) ==
                  1u && runtime.calls[0] == "begin text" &&
                  runtime.calls[1] == "begin text" &&
                  runtime.calls[runtime.calls.size() - 2u] ==
                      "release hint" &&
                  runtime.calls.back() == "release formatted" &&
                  std::find(runtime.calls.begin(), runtime.calls.end(),
                            "show hint") != runtime.calls.end(),
              "getitem enter text and hint lifecycle");
        if (kind < 3u)
            Check(runtime.granted_amount == 7u || kind == 1u,
                  "getitem grant amount");
        if (kind == 0u)
            Check(runtime.granted_id == 42u &&
                      runtime.name == "Herb" &&
                      runtime.formatted_count == 7u &&
                      runtime.sound ==
                          "gamedata\\PALSound\\WA06.wav",
                  "getitem item reward");
        if (kind == 1u)
            Check(runtime.granted_id == 42u &&
                      runtime.name == "Sword" &&
                      runtime.formatted_count == 1u &&
                      runtime.sound ==
                          "gamedata\\PALSound\\WA06.wav",
                  "getitem equipment reward");
        if (kind == 2u)
            Check(runtime.format == "money %d" &&
                      runtime.sound ==
                          "gamedata\\PALSound\\WA40.wav",
                  "getitem money reward");
        if (kind == 3u)
            Check(runtime.sound.empty() && runtime.granted_id == 0u,
                  "getitem unknown kind still shows empty hint");
    }
    Runtime missing;
    missing.item_exists = false;
    missing.window = 0u;
    Check(pal4::GetItemTask::Enter(Runtime::object, missing) ==
              1u && missing.sound.empty() &&
              std::find(missing.calls.begin(), missing.calls.end(),
                        "show hint") == missing.calls.end() &&
              std::find(missing.calls.begin(), missing.calls.end(),
                        "create hint") != missing.calls.end(),
          "getitem missing database record and hint window");
    Runtime failed;
    failed.throw_on_format = true;
    bool thrown = false;
    try {
        (void)pal4::GetItemTask::Enter(Runtime::object, failed);
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    Check(thrown && failed.calls[failed.calls.size() - 2u] ==
                        "release hint" &&
              failed.calls.back() == "release formatted",
          "getitem enter exception cleans both texts");
}

void CheckUpdate() {
    Runtime no_context;
    no_context.words[Runtime::object + 0x4cu] = 0u;
    no_context.bytes[Runtime::object + 0x3cu] = 1u;
    Check(pal4::GetItemTask::Update(Runtime::object, no_context) ==
              0u && no_context.bytes.at(Runtime::object + 0x3cu) ==
                        0u,
          "getitem update clears completion without context");
    Runtime complete;
    complete.bytes[Runtime::object + 0x3cu] = 1u;
    Check(pal4::GetItemTask::Update(Runtime::object, complete) ==
              0u && complete.calls.empty(),
          "getitem update skips completed task");
    for (const bool visible : {false, true}) {
        Runtime runtime;
        runtime.bytes[Runtime::object + 0x3cu] = 0u;
        runtime.visible = visible;
        Check(pal4::GetItemTask::Update(Runtime::object, runtime) ==
                  1u && runtime.bytes.at(Runtime::object + 0x3cu) ==
                            (visible ? 0u : 1u),
              "getitem update waits for hint visibility");
    }
    Runtime missing;
    missing.bytes[Runtime::object + 0x3cu] = 0u;
    missing.window = 0u;
    Check(pal4::GetItemTask::Update(Runtime::object, missing) ==
              1u && missing.bytes.at(Runtime::object + 0x3cu) == 0u,
          "getitem update waits when hint window is missing");
}

void CheckFollowupAndInitialize() {
    Runtime init;
    init.bytes[Runtime::object + 0x3cu] = 1u;
    pal4::GetItemTask::Initialize(Runtime::object, 0x4000u, 2u,
                                  42u, 7u, 0x5000u, init);
    Check(init.words.at(Runtime::object + 0x4cu) == 0x4000u &&
              init.words.at(Runtime::object + 0x40u) == 2u &&
              init.words.at(Runtime::object + 0x44u) == 42u &&
              init.words.at(Runtime::object + 0x48u) == 7u &&
              init.words.at(Runtime::object + 0x50u) == 0x5000u &&
              pal4::GetItemTask::IsComplete(Runtime::object, init) == 0u,
          "getitem initialize fields and completion getter");
    init.bytes[Runtime::object + 0x3cu] = 0xa5u;
    Check(pal4::GetItemTask::IsComplete(Runtime::object, init) ==
              0xa5u,
          "getitem completion getter forwards raw byte");

    SpawnRuntime no_context;
    no_context.words[Runtime::object + 0x50u] = 0u;
    Check(pal4::GetItemTask::SpawnFollowup(
              Runtime::object, no_context) == 0u &&
              no_context.calls.empty(),
          "getitem follow-up requires context");
    SpawnRuntime no_owner;
    no_owner.words[Runtime::object + 0x4cu] = 0u;
    Check(pal4::GetItemTask::SpawnFollowup(
              Runtime::object, no_owner) == 0u &&
              no_owner.calls.empty(),
          "getitem follow-up requires owner");
    SpawnRuntime previous;
    previous.words[Runtime::object + 0x30u] = 0x6000u;
    Check(pal4::GetItemTask::SpawnFollowup(
              Runtime::object, previous) == 1u &&
              previous.calls ==
                  (std::vector<std::string>{"previous"}),
          "getitem follow-up notifies previous before creating helper");
    SpawnRuntime spare;
    Check(pal4::GetItemTask::SpawnFollowup(
              Runtime::object, spare) == 1u &&
              spare.allocation_sizes ==
                  (std::vector<std::uint32_t>{0x40u}) &&
              spare.added_helper == 0xb000u &&
              spare.owner_assigned == 0x4000u &&
              spare.words.at(0xa004u) == 0xb000u &&
              spare.words.at(0x42a8u) == 0xa008u &&
              spare.processed == 0x5000u &&
              spare.calls.back() == "processed",
          "getitem follow-up appends helper to spare capacity");
    SpawnRuntime growth;
    growth.words[0x42acu] = 0xa004u;
    Check(pal4::GetItemTask::SpawnFollowup(
              Runtime::object, growth) == 1u &&
              growth.allocation_sizes ==
                  (std::vector<std::uint32_t>{0x40u, 8u}) &&
              growth.words.at(0xe000u) == 0xaaaa0000u &&
              growth.words.at(0xe004u) == 0xb000u &&
              growth.words.at(0x42a8u) == 0xe008u &&
              std::find(growth.calls.begin(), growth.calls.end(),
                        "free") != growth.calls.end(),
          "getitem follow-up grows helper pointer collection");
    SpawnRuntime no_helper;
    no_helper.helper_memory = 0u;
    Check(pal4::GetItemTask::SpawnFollowup(
              Runtime::object, no_helper) == 1u &&
              no_helper.added_helper == 0u &&
              no_helper.words.at(0xa004u) == 0u &&
              no_helper.processed == 0x5000u,
          "getitem follow-up appends null helper after failed allocation");
}

}  // namespace

int main() {
    try {
        CheckLifecycle();
        CheckEnter();
        CheckUpdate();
        CheckFollowupAndInitialize();
        std::cout << "get_item_task_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
