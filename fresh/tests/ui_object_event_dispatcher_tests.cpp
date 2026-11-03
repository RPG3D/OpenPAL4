#include "pal4/ui_object_event_dispatcher.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::UiObjectEventRuntimeStub {
    std::uint32_t code{};
    bool forwarded{};
    bool throw_dispatch{};
    std::vector<char> calls;

    std::uint32_t ReadEventCode(std::uint32_t member) override {
        Check(member == 0x2010u, "event type is read at +0x10");
        calls.push_back('R');
        return code;
    }
    std::uint32_t EventArgsVtable() override {
        calls.push_back('V');
        return 0xabcdu;
    }
    void DispatchAction(std::uint32_t object,
                        const pal4::UiEventArgsRecord& args) override {
        Check(object == 0x1000u && args.vtable == 0xabcdu &&
                  args.handled == 0u && args.tail == 0u,
              "event one dispatches a zeroed CEGUI EventArgs from owner");
        calls.push_back('D');
        if (throw_dispatch) throw std::runtime_error("dispatch");
    }
    bool ForwardKeyEvent(std::uint32_t subobject,
                         std::uint32_t event) override {
        Check(subobject == 0x1028u && event == 0x2000u,
              "other event types forward original subobject and event");
        calls.push_back('F');
        return forwarded;
    }
};

}  // namespace

int main() {
    Runtime runtime;
    runtime.code = 1u;
    Check(pal4::UiObjectEventDispatcher::Handle(
              0x1028u, 0x2000u, runtime) &&
              runtime.calls == std::vector<char>{'R', 'V', 'D'},
          "event one dispatches and returns true");

    runtime.calls.clear();
    runtime.code = 2u;
    Check(!pal4::UiObjectEventDispatcher::Handle(
              0x1028u, 0x2000u, runtime) &&
              runtime.calls == std::vector<char>{'R', 'F'},
          "other events forward false return");

    runtime.calls.clear();
    runtime.forwarded = true;
    Check(pal4::UiObjectEventDispatcher::Handle(
              0x1028u, 0x2000u, runtime),
          "forwarded true return is preserved");

    runtime.code = 1u;
    runtime.throw_dispatch = true;
    try {
        (void)pal4::UiObjectEventDispatcher::Handle(
            0x1028u, 0x2000u, runtime);
        Check(false, "dispatch exception propagates");
    } catch (const std::runtime_error&) {
        Check(runtime.calls.back() == 'D',
              "dispatch exception reaches caller after local cleanup");
    }
    std::cout << "UI object event dispatcher tests passed\n";
}
