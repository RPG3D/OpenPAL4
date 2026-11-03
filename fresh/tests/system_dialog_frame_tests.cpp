#include "pal4/system_dialog_frame.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ISystemDialogFrameRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x5024u, 0x6000u}, {0x8411b8u, 0xa000u},
        {0x84103cu, 0xa001u}};
    std::vector<std::string> calls;
    std::string current_name;
    std::uint32_t allocations = 0u;
    std::uint32_t subscriptions = 0u;
    std::uint32_t releases = 0u;
    std::uint32_t null_allocation = 99u;
    std::uint32_t null_connection = 99u;
    std::string fail_at;

    void Fail(const char* stage) {
        if (fail_at == stage) throw std::runtime_error(stage);
    }
    void LoadLayout(const std::uint32_t frame,
                    const std::uint32_t name) override {
        Check(frame == 0x5000u && name == 0x7000u,
              "layout arguments");
        calls.emplace_back("layout");
        Fail("layout");
    }
    void DestroyBase(const std::uint32_t frame) override {
        Check(frame == 0x5000u, "base cleanup frame");
        calls.emplace_back("destroy-base");
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        calls.emplace_back("write:" + std::to_string(address));
        words[address] = value;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words.at(address);
    }
    std::uint32_t BeginName(const std::uint32_t source) override {
        Check(source == 0x5014u, "name source field");
        calls.emplace_back("begin-name");
        Fail("begin-name");
        current_name = "sysDialog/";
        return 0x9000u;
    }
    void ResetName(const std::uint32_t buffer,
                   const std::uint32_t source) override {
        Check(buffer == 0x9000u && source == 0x5014u,
              "name reset source");
        calls.emplace_back("reset-name");
        current_name = "sysDialog/";
    }
    void AppendName(const std::uint32_t buffer,
                    const std::string_view suffix) override {
        Check(buffer == 0x9000u, "name buffer");
        calls.emplace_back("append:" + std::string(suffix));
        Fail("append");
        current_name += suffix;
    }
    std::uint32_t FindWindow(const std::uint32_t frame,
                             const std::uint32_t buffer) override {
        Check(frame == 0x5000u && buffer == 0x9000u,
              "window lookup target");
        calls.emplace_back("find:" + current_name);
        Fail("find");
        if (current_name == "sysDialog/EditInfo") return 0x8000u;
        Check(current_name == "sysDialog/BtnOk", "button name");
        return 0x8004u;
    }
    void EndName(const std::uint32_t buffer) override {
        Check(buffer == 0x9000u, "temporary name cleanup");
        calls.emplace_back("end-name");
    }
    std::uint32_t Allocate(const std::uint32_t bytes) override {
        Check(bytes == 12u, "dialog callback size");
        calls.emplace_back("allocate");
        ++allocations;
        return allocations == null_allocation ? 0u :
            0xb000u + allocations * 0x20u;
    }
    std::uint32_t Subscribe(const std::uint32_t window,
                            const std::uint32_t event,
                            const std::uint32_t callback) override {
        ++subscriptions;
        Check((subscriptions == 1u && window == 0x8004u &&
               event == 0xa000u) ||
              (subscriptions == 2u && window == 0x6000u &&
               event == 0xa001u),
              "event target and kind");
        Check(callback == (subscriptions == null_allocation ? 0u :
              0xb000u + subscriptions * 0x20u),
              "callback pointer");
        calls.emplace_back("subscribe");
        Fail("subscribe");
        return subscriptions == null_connection ? 0u :
            0xc000u + subscriptions;
    }
    void ReleaseConnection(const std::uint32_t connection) override {
        Check(connection == 0xc001u || connection == 0xc002u,
              "connection release handle");
        calls.emplace_back("release");
        ++releases;
    }
};

void TestConstruct() {
    Runtime runtime;
    Check(pal4::SystemDialogFrame::Construct(
              0x5000u, 0x7000u, runtime) == 0x5000u &&
              runtime.calls[0] == "layout" &&
              runtime.calls[1] == "write:20480" &&
              runtime.words.at(0x5000u) == 0x843c64u &&
              runtime.words.at(0x5028u) == 0x8000u &&
              runtime.words.at(0x502cu) == 0x8004u,
          "layout, vtable and two windows");
    Check(runtime.calls[2] == "begin-name" &&
              runtime.calls[3] == "append:EditInfo" &&
              runtime.calls[4] == "find:sysDialog/EditInfo" &&
              runtime.calls[6] == "reset-name" &&
              runtime.calls[7] == "append:BtnOk" &&
              runtime.calls[8] == "find:sysDialog/BtnOk" &&
              runtime.calls.back() == "end-name",
          "temporary name is reused and released once");
    Check(runtime.allocations == 2u && runtime.subscriptions == 2u &&
              runtime.releases == 2u &&
              runtime.words.at(0xb024u) == 0x483a80u &&
              runtime.words.at(0xb028u) == 0x5000u &&
              runtime.words.at(0xb020u) == 0x843c5cu &&
              runtime.words.at(0xb044u) == 0x483a90u &&
              runtime.words.at(0xb048u) == 0x5000u &&
              runtime.words.at(0xb040u) == 0x843c5cu,
          "two twelve-byte callbacks and event connections");
}

void TestNullHandles() {
    Runtime runtime;
    runtime.null_allocation = 1u;
    runtime.null_connection = 2u;
    Check(pal4::SystemDialogFrame::Construct(
              0x5000u, 0x7000u, runtime) == 0x5000u &&
              runtime.allocations == 2u && runtime.subscriptions == 2u &&
              runtime.releases == 1u &&
              runtime.words.find(0xb020u) == runtime.words.end() &&
              runtime.calls.back() == "end-name",
          "null callback and connection leave remaining setup intact");
}

void TestFailures() {
    for (const auto& stage : {"layout", "begin-name", "append",
                              "find", "subscribe"}) {
        Runtime runtime;
        runtime.fail_at = stage;
        bool caught = false;
        try {
            (void)pal4::SystemDialogFrame::Construct(
                0x5000u, 0x7000u, runtime);
        } catch (const std::runtime_error&) {
            caught = true;
        }
        Check(caught, "failure propagates");
        if (runtime.fail_at == "layout") {
            Check(runtime.calls == std::vector<std::string>{"layout"},
                  "layout failure has no completed base to unwind");
        } else if (runtime.fail_at == "begin-name") {
            Check(runtime.calls.back() == "destroy-base",
                  "name initialization failure unwinds base");
        } else {
            Check(runtime.calls[runtime.calls.size() - 2u] == "end-name" &&
                      runtime.calls.back() == "destroy-base",
                  "later failure releases name before base");
        }
    }
}

}  // namespace

int main() {
    TestConstruct();
    TestNullHandles();
    TestFailures();
    return EXIT_SUCCESS;
}
