#include "pal4/collection_ui_registration.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::CollectionUiRegistrationRuntimeStub {
    std::vector<char> events;
    std::uint32_t allocated{0x6000u};
    bool throw_on_insert{};
    std::uint32_t seen_callback{0xffffffffu};

    std::uint32_t Allocate(std::size_t bytes) override {
        Check(bytes == 8u, "callback allocation size is eight bytes");
        events.push_back('A');
        return allocated;
    }
    void InitializeCallback(std::uint32_t address,
                            std::uint32_t vtable,
                            std::string_view name) override {
        Check(address == 0x6000u &&
                  vtable == pal4::CollectionUiRegistration::kCallbackVtable &&
                  name == "CastWindow",
              "callback stores its vtable and CastWindow name");
        events.push_back('I');
    }
    std::uint32_t FrameManager() override {
        events.push_back('M');
        return 0x9000u;
    }
    void InsertOrReplace(std::uint32_t tree_address,
                         std::string_view event_name,
                         std::uint32_t& callback) override {
        Check(tree_address == 0x9104u && event_name == "show_cast_ui",
              "callback inserts at UIFrameManager +0x104 under event name");
        seen_callback = callback;
        events.push_back('T');
        if (throw_on_insert) throw std::runtime_error("tree insert");
    }
    void DeleteCallback(std::uint32_t address,
                        std::uint32_t flags) override {
        Check(address == 0x6000u && flags == 1u,
              "local callback release uses virtual deletion flag one");
        events.push_back('D');
    }
};

void CheckRegistration() {
    Runtime runtime;
    Check(pal4::CollectionUiRegistration::RegisterShowCastUi(runtime) ==
              0x8e43a8u && runtime.seen_callback == 0x6000u &&
              runtime.events == std::vector<char>{'A', 'I', 'M', 'T', 'D'},
          "registration inserts and releases local callback in order");
    runtime.events.clear();
    Check(pal4::CollectionUiRegistration::RegisterShowCastUiThunk(runtime) ==
              0x8e43a8u &&
              runtime.events == std::vector<char>{'A', 'I', 'M', 'T', 'D'},
          "thunk reaches same registration path");
    runtime.events.clear();
    runtime.allocated = 0u;
    Check(pal4::CollectionUiRegistration::RegisterShowCastUi(runtime) ==
              0x8e43a8u && runtime.seen_callback == 0u &&
              runtime.events == std::vector<char>{'A', 'M', 'T'},
          "zero allocation still inserts null callback and skips release");
}

void CheckInsertionFailure() {
    Runtime runtime;
    runtime.throw_on_insert = true;
    bool threw = false;
    try {
        (void)pal4::CollectionUiRegistration::RegisterShowCastUi(runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw &&
              runtime.events == std::vector<char>{'A', 'I', 'M', 'T', 'D'},
          "tree insertion failure releases local callback once");
}

}  // namespace

int main() {
    CheckRegistration();
    CheckInsertionFailure();
    std::cout << "collection UI registration tests passed\n";
}
