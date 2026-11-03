#include "pal4/load_window_ui_registration.h"

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

struct Runtime final : pal4::LoadWindowUiRuntimeStub {
    std::uint32_t allocation = 0x3000u;
    std::uint32_t replacement{};
    std::uint32_t deleted{};
    bool fail_insert{};
    std::vector<std::string> calls;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 8u, "UI callback allocation has two words");
        calls.emplace_back("allocate");
        return allocation;
    }
    std::uint32_t WindowNameAddress() override {
        calls.emplace_back("name");
        return 0x2000u;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        if (address == 0x3004u && value == 0x2000u) {
            calls.emplace_back("name-field");
        } else if (address == 0x3000u &&
                   value == pal4::LoadWindowUiRegistration::kCallbackVtable) {
            calls.emplace_back("vtable");
        } else {
            Check(false, "callback write uses observed offset and value");
        }
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x9000u;
    }
    void InsertOrReplace(std::uint32_t tree,
                         std::string_view event,
                         std::uint32_t& callback) override {
        Check(tree == 0x9104u && event == "show_load_ui" &&
                  callback == allocation,
              "command is inserted in manager tree +0x104");
        calls.emplace_back("insert");
        if (fail_insert) throw std::runtime_error("insert");
        if (replacement != 0u) callback = replacement;
    }
    void DeleteCallback(std::uint32_t callback,
                        std::uint32_t flags) override {
        Check(flags == 1u, "local callback uses deleting flag one");
        deleted = callback;
        calls.emplace_back("delete");
    }
};

}  // namespace

int main() {
    Runtime runtime;
    Check(pal4::LoadWindowUiRegistration::RegisterShowUi(runtime) ==
              pal4::LoadWindowUiRegistration::kResultAddress &&
              runtime.deleted == 0x3000u &&
              runtime.calls == std::vector<std::string>{
                  "allocate", "name", "name-field", "vtable",
                  "manager", "insert", "delete"},
          "registration builds callback, inserts command and releases it");

    runtime = Runtime{};
    runtime.allocation = 0u;
    Check(pal4::LoadWindowUiRegistration::RegisterShowUi(runtime) ==
              pal4::LoadWindowUiRegistration::kResultAddress &&
              runtime.calls == std::vector<std::string>{
                  "allocate", "manager", "insert"},
          "null callback is still passed to command registration");

    runtime = Runtime{};
    runtime.replacement = 0x4000u;
    Check(pal4::LoadWindowUiRegistration::RegisterShowUiThunk(runtime) ==
              pal4::LoadWindowUiRegistration::kResultAddress &&
              runtime.deleted == 0x4000u,
          "thunk releases the local pointer after tree replacement");

    runtime = Runtime{};
    runtime.fail_insert = true;
    try {
        (void)pal4::LoadWindowUiRegistration::RegisterShowUi(runtime);
        Check(false, "tree insertion exception must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.deleted == 0x3000u &&
                  runtime.calls.back() == "delete",
              "insertion exception releases callback");
    }
    std::cout << "Load window UI registration tests passed\n";
}
