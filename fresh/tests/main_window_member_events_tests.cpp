#include "pal4/main_window_member_events.h"

#include <cstdlib>
#include <iostream>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::MainWindowMemberEventRuntimeStub {
    std::uint32_t key_calls{};
    std::uint32_t selection_calls{};
    std::uint8_t key_result = 3u;
    std::uint8_t selection_result = 7u;

    std::uint8_t HandleKeyEvent(
        pal4::MainWindowMemberState& state,
        std::uint32_t event) override {
        Check(event == 0x4567u && state.bytes[0x40u] == 2u,
              "key thunk forwards event and original member object");
        ++key_calls;
        return key_result;
    }
    std::uint8_t HandleSelection(
        pal4::MainWindowMemberState& state) override {
        Check(state.bytes[0x40u] == 0u,
              "base selection runs only when flag is clear");
        ++selection_calls;
        return selection_result;
    }
};

}  // namespace

int main() {
    pal4::MainWindowMemberState state{};
    Runtime runtime;
    state.bytes[0x40u] = 2u;
    Check(pal4::MainWindowMemberEvents::ForwardKey(
              state, 0x4567u, runtime) == 3u && runtime.key_calls == 1u &&
              state.bytes[0x40u] == 2u,
          "key event is a pure forwarding thunk");
    Check(pal4::MainWindowMemberEvents::ResetSelection(
              state, runtime) == 1u && state.bytes[0x40u] == 0u &&
              runtime.selection_calls == 0u,
          "nonzero flag resets and returns one without base selection");
    Check(pal4::MainWindowMemberEvents::ResetSelection(
              state, runtime) == 7u && runtime.selection_calls == 1u,
          "clear flag forwards the base selection result");
    runtime.selection_result = 0u;
    Check(pal4::MainWindowMemberEvents::ResetSelection(
              state, runtime) == 0u && runtime.selection_calls == 2u,
          "clear flag preserves a false base result");
    std::cout << "Main window member events tests passed\n";
}
