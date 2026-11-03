#include "pal4/magic_window.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(EXIT_FAILURE); }
}

struct Runtime final : pal4::MagicWindowRuntimeStub {
    std::vector<std::string> calls;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::uint32_t allocation{0x5000u};
    bool fail_insert{};
    void LoadLayout(std::uint32_t object, std::uint32_t name) override {
        Check(object == 0x3000u && name == 0x7000u, "layout inputs");
        calls.push_back("layout");
    }
    void ConstructMember(std::uint32_t address) override {
        Check(address == 0x3028u, "member offset"); calls.push_back("member");
    }
    void ConstructTextArray(std::uint32_t address, std::uint32_t count) override {
        Check(address == 0x3170u && count == 7u, "text array offset and count");
        calls.push_back("texts");
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x18u, "event callback allocation size");
        calls.push_back("allocate"); return allocation;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value; calls.push_back("write");
    }
    std::uint32_t FrameManager() override { calls.push_back("manager"); return 0x8000u; }
    void InsertOrReplace(std::uint32_t tree, std::string_view command,
                         std::uint32_t& callback) override {
        Check(tree == 0x8104u && command == "use_stunt" &&
                  callback == allocation, "stunt command registration");
        calls.push_back("insert");
        if (fail_insert) throw std::runtime_error("insert");
    }
    void DeleteCallback(std::uint32_t address, std::uint32_t flags) override {
        Check(address == 0x5000u && flags == 1u, "event callback cleanup");
        calls.push_back("delete");
    }
    bool HasStuntText(std::uint32_t index) override {
        Check(index < 7u, "stunt index bounded");
        calls.push_back("has:" + std::to_string(index));
        return (index % 2u) == 0u;
    }
    std::string ReadStuntText(std::uint32_t index) override {
        calls.push_back("read:" + std::to_string(index));
        return "stunt-" + std::to_string(index);
    }
    void InitializePanel(std::uint32_t object) override {
        Check(object == 0x3000u, "panel object"); calls.push_back("panel");
    }
    void DestroyTextArray(std::uint32_t object, std::uint32_t count) override {
        Check(object == 0x3170u && count == 7u, "text array destruction");
        calls.push_back("destroy-texts");
    }
    void FreeMemberStorage(std::uint32_t storage) override {
        Check(storage == 0x9000u, "member storage release");
        calls.push_back("free-member");
    }
    std::uint32_t DestroyBase(std::uint32_t object) override {
        Check(object == 0x3000u, "base destruction target");
        calls.push_back("destroy-base");
        return 0xabcdu;
    }
};

void TestConstruct() {
    pal4::MagicWindowState state;
    Runtime runtime;
    Check(&pal4::MagicWindow::Construct(0x3000u, 0x7000u, state, runtime) == &state,
          "returns constructed state");
    Check(state.vtable == 0x843920u && state.member_vtable == 0x843938u &&
              state.selection_vtable == 0x84397cu && state.list_vtable == 0x843964u &&
              state.event_vtable == 0x843960u, "vtable identities");
    Check(state.category_position == 1u &&
              state.slot_order == std::array<std::uint32_t, 6>{5u, 0u, 1u, 2u, 3u, 4u} &&
              state.list_owner == 0x3000u && state.selection_owner == 0x3000u,
          "selection fields and self links");
    Check(runtime.words.at(0x5000u) == 0x843914u &&
              runtime.words.at(0x5008u) == 0x45a500u &&
              runtime.words.at(0x500cu) == 0u &&
              runtime.words.at(0x5010u) == 0x3000u, "callback layout");
    for (std::uint32_t index = 0; index < 7u; ++index)
        Check(state.stunt_text[index] == (index % 2u == 0u ?
              "stunt-" + std::to_string(index) : ""), "optional stunt text");
    Check(runtime.calls.front() == "layout" && runtime.calls[1] == "member" &&
              runtime.calls[2] == "texts" && runtime.calls[3] == "allocate" &&
              runtime.calls[9] == "insert" && runtime.calls[10] == "delete" &&
              runtime.calls.back() == "panel", "constructor stage order");
}

void TestCallbackBranches() {
    pal4::MagicWindowState state;
    Runtime runtime;
    runtime.allocation = 0;
    (void)pal4::MagicWindow::Construct(0x3000u, 0x7000u, state, runtime);
    Check(runtime.words.empty() &&
              std::find(runtime.calls.begin(), runtime.calls.end(), "delete") == runtime.calls.end(),
          "null callback does not write or delete");
    runtime = Runtime{};
    runtime.fail_insert = true;
    try {
        (void)pal4::MagicWindow::Construct(0x3000u, 0x7000u, state, runtime);
        Check(false, "expected insertion failure");
    } catch (const std::runtime_error&) {
        Check(runtime.calls.back() == "delete", "exception releases callback");
    }
}

void TestDestroy() {
    pal4::MagicWindowState state;
    state.member_storage = 0x9000u;
    state.member_size = 3u;
    state.member_capacity = 8u;
    Runtime runtime;
    Check(pal4::MagicWindow::Destroy(0x3000u, state, runtime) == 0xabcdu &&
              state.member_storage == 0u && state.member_size == 0u &&
              state.member_capacity == 0u &&
              runtime.calls == std::vector<std::string>{
                  "destroy-texts", "free-member", "destroy-base"},
          "destructor cleanup and order");
}

}  // namespace

int main() { TestConstruct(); TestCallbackBranches(); TestDestroy(); }
