#include "pal4/introduction_window_ui_registration.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IntroductionWindowUiRuntimeStub {
    std::uint32_t allocation = 0x3000u;
    bool throw_insert{};
    std::vector<std::string> calls;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 8u, "show callback allocates eight bytes");
        calls.emplace_back("allocate");
        return allocation;
    }
    std::uint32_t WindowNameAddress() override {
        calls.emplace_back("name");
        return 0x5000u;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        if (address == 0x3004u && value == 0x5000u) {
            calls.emplace_back("write-name");
        } else if (address == 0x3000u &&
                   value == pal4::IntroductionWindowUiRegistration::
                                kCallbackVtable) {
            calls.emplace_back("write-vtable");
        } else {
            Check(false, "callback writes name then vtable");
        }
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x8000u;
    }
    void InsertOrReplace(std::uint32_t tree, std::string_view event,
                         std::uint32_t& callback) override {
        Check(tree == 0x8104u && event == "show_introduction_ui" &&
                  callback == allocation,
              "tree insertion uses manager +0x104 and event name");
        calls.emplace_back("insert");
        if (throw_insert) throw std::runtime_error("insert");
    }
    void DeleteCallback(std::uint32_t callback,
                        std::uint32_t flags) override {
        Check(callback == 0x3000u && flags == 1u,
              "local callback is released with deleting flag one");
        calls.emplace_back("delete");
    }
};

}  // namespace

int main() {
    Runtime runtime;
    Check(pal4::IntroductionWindowUiRegistration::RegisterShowUi(runtime) ==
              0x8e43b0u &&
              runtime.calls == std::vector<std::string>{
                  "allocate", "name", "write-name", "write-vtable",
                  "manager", "insert", "delete"},
          "show callback is inserted and local reference released");

    runtime.calls.clear();
    runtime.allocation = 0u;
    Check(pal4::IntroductionWindowUiRegistration::RegisterShowUi(runtime) ==
              0x8e43b0u &&
              runtime.calls == std::vector<std::string>{
                  "allocate", "manager", "insert"},
          "failed allocation still inserts a null callback");

    runtime.calls.clear();
    runtime.allocation = 0x3000u;
    runtime.throw_insert = true;
    try {
        (void)pal4::IntroductionWindowUiRegistration::RegisterShowUi(runtime);
        Check(false, "insertion exception propagates");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{
                  "allocate", "name", "write-name", "write-vtable",
                  "manager", "insert", "delete"},
              "insertion exception releases callback");
    }

    runtime.calls.clear();
    runtime.throw_insert = false;
    Check(pal4::IntroductionWindowUiRegistration::RegisterShowUiThunk(
              runtime) == 0x8e43b0u &&
              runtime.calls.front() == "allocate" &&
              runtime.calls.back() == "delete",
          "thunk forwards to show callback registration");
    std::cout << "Introduction window UI registration tests passed\n";
}
