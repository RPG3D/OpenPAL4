#include "pal4/magic_window_stunt_selection.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::MagicWindowStuntSelectionRuntimeStub {
    enum class DialogAction { Keep, Clear, Replace, Throw };

    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint32_t> words;
    pal4::MagicDbStuntRecord record;
    bool exists{true};
    std::uint32_t player{0x9000u};
    std::int32_t value{5};
    std::uint32_t allocation{0xa000u};
    DialogAction dialog_action{DialogAction::Keep};

    Runtime() {
        record.key = 71u;
        record.required_stat = 5;
    }

    bool HasStunt(std::uint32_t key) override {
        Check(key == 71u, "existence lookup key");
        calls.push_back("has-stunt");
        return exists;
    }
    std::uint32_t GetPlayer(std::int32_t index) override {
        Check(index == 3, "selected character index");
        calls.push_back("player");
        return player;
    }
    std::optional<pal4::MagicDbStuntRecord> FindStunt(
        std::uint32_t key) override {
        Check(key == 71u, "record lookup key");
        calls.push_back("find-stunt");
        return record;
    }
    std::int32_t PlayerValue(std::uint32_t selected,
                             std::uint32_t stat) override {
        Check(selected == player && stat == 8u, "stat lookup arguments");
        calls.push_back("value");
        return value;
    }
    std::string_view TriggerText(std::uint32_t trigger_id) override {
        Check(trigger_id == 0x6cu, "failure trigger ID");
        calls.push_back("trigger");
        return "Insufficient stat";
    }
    void ShowStatFailure(std::string_view text, float x, float y,
                         std::uint32_t style,
                         std::uint32_t flags) override {
        Check(text == "Insufficient stat" && x == 0.5f && y == 0.4f &&
                  style == 4u && flags == 0u,
              "failure dialog arguments");
        calls.push_back("failure-dialog");
    }
    std::uint32_t AllocateCallback(std::uint32_t bytes) override {
        Check(bytes == 0x18u, "callback allocation size");
        calls.push_back("allocate");
        return allocation;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value_to_write) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value_to_write;
    }
    void ShowCastDialog(bool visible, std::uint32_t& callback,
                        const pal4::MagicDbStuntRecord& stunt) override {
        Check(visible && stunt.key == record.key &&
                  stunt.required_stat == record.required_stat,
              "cast dialog receives record and visible flag");
        calls.push_back("cast-dialog");
        switch (dialog_action) {
        case DialogAction::Keep: break;
        case DialogAction::Clear: callback = 0; break;
        case DialogAction::Replace: callback = 0xb000u; break;
        case DialogAction::Throw: throw std::runtime_error("dialog failure");
        }
    }
    void DeleteCallback(std::uint32_t callback,
                        std::uint32_t flags) override {
        Check(flags == 1u, "deleting destructor flags");
        calls.push_back("delete:" + std::to_string(callback));
    }
};

pal4::MagicWindowState State() {
    pal4::MagicWindowState state;
    state.selected_character_index = 3;
    return state;
}

bool Handle(Runtime& runtime) {
    return pal4::MagicWindowStuntSelection::Handle(
        0x4000u, 71u, State(), runtime);
}

void TestEarlyExits() {
    Runtime absent;
    absent.exists = false;
    Check(Handle(absent) &&
              absent.calls == std::vector<std::string>{"has-stunt"},
          "missing key exits before player lookup");
    Runtime missing_player;
    missing_player.player = 0;
    Check(Handle(missing_player) &&
              missing_player.calls ==
                  std::vector<std::string>{"has-stunt", "player"},
          "missing player exits before second record lookup");
}

void TestSignedStatFailure() {
    Runtime runtime;
    runtime.value = -2;
    runtime.record.required_stat = -1;
    Check(Handle(runtime), "stat failure returns true");
    Check(runtime.calls == std::vector<std::string>{
                               "has-stunt", "player", "find-stunt", "value",
                               "trigger", "failure-dialog"},
          "signed stat failure shows trigger and avoids callback allocation");
}

void TestCallbackLayoutAndOwnership() {
    Runtime runtime;
    Check(Handle(runtime), "equal stat permits cast dialog");
    Check(runtime.calls == std::vector<std::string>{
                               "has-stunt", "player", "find-stunt", "value",
                               "allocate", "write:40968", "write:40972",
                               "write:40976", "write:40960", "cast-dialog",
                               "delete:40960"},
          "callback initialization and cleanup order");
    Check(runtime.words == std::map<std::uint32_t, std::uint32_t>{
                               {0xa000u,
                                pal4::MagicWindowStuntSelection::kCastCallbackVtable},
                               {0xa008u,
                                pal4::MagicWindowStuntSelection::kCastCallbackHandler},
                               {0xa00cu, 0u}, {0xa010u, 0x4000u}},
          "24-byte callback fields");
}

void TestAllocationAndDialogMutation() {
    Runtime failed_allocation;
    failed_allocation.allocation = 0;
    Check(Handle(failed_allocation) &&
              failed_allocation.calls.back() == "cast-dialog" &&
              failed_allocation.words.empty(),
          "null allocation still calls dialog without destructor");

    Runtime cleared;
    cleared.dialog_action = Runtime::DialogAction::Clear;
    Check(Handle(cleared) && cleared.calls.back() == "cast-dialog",
          "dialog clearing callback suppresses destructor");

    Runtime replaced;
    replaced.dialog_action = Runtime::DialogAction::Replace;
    Check(Handle(replaced) && replaced.calls.back() == "delete:45056",
          "dialog replacement controls which callback is deleted");
}

void TestExceptionCleanup() {
    Runtime runtime;
    runtime.dialog_action = Runtime::DialogAction::Throw;
    bool caught = false;
    try {
        Handle(runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls.back() == "delete:40960",
          "exception cleanup releases callback before rethrow");
}

}  // namespace

int main() {
    TestEarlyExits();
    TestSignedStatFailure();
    TestCallbackLayoutAndOwnership();
    TestAllocationAndDialogMutation();
    TestExceptionCleanup();
}
