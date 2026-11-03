#include "pal4/native_event_handler_array.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void CheckNativeStorageAndDispatch() {
    using Call = std::pair<std::uint32_t, std::uint32_t>;
    std::vector<Call> calls;
    pal4::NativeEventHandlerArray* current = nullptr;
    bool append_during_update = false;
    pal4::NativeEventHandlerArray handlers{
        [&](std::uint32_t object, std::uint32_t slot) {
            calls.emplace_back(object, slot);
            if (append_during_update && slot == 0x14u && object == 11u) {
                append_during_update = false;
                // Three entries have capacity four: appending must not
                // invalidate the original iterator, which reloads only end.
                (void)current->Insert(44u);
            }
            return object + slot;
        },
        [&](std::uint32_t object, std::uint32_t slot, std::uint32_t event) {
            calls.emplace_back(object, slot);
            return static_cast<std::uint8_t>(object == event && slot == 8u);
        }};
    current = &handlers;
    Check(handlers.Size() == 0u && handlers.GetLast() == 0u &&
              handlers.Remove(11u) == 0u && !handlers.Contains(11u),
          "native empty array queries");
    Check(handlers.Insert(11u) == 23u && handlers.Insert(22u) == 34u &&
              handlers.Insert(33u) == 45u && handlers.Size() == 3u &&
              handlers.OutstandingAllocations() == 1u,
          "growth releases previous native buffers and forwards callback returns");
    Check(calls == std::vector<Call>{{11u, 12u}, {22u, 12u}, {33u, 12u}},
          "insert callbacks preserve pointer order");
    calls.clear();
    Check(handlers.Contains(22u) &&
              calls == std::vector<Call>{{33u, 4u}, {33u, 8u}, {22u, 8u}},
          "input predicates use special last check then reverse traversal");
    calls.clear();
    append_during_update = true;
    handlers.Update();
    Check(calls == std::vector<Call>{{11u, 20u}, {44u, 12u},
                                    {22u, 20u}, {33u, 20u}, {44u, 20u}} &&
              handlers.Size() == 4u,
          "native frame traversal reloads end after callback append");
    calls.clear();
    Check(handlers.Remove(22u) == 27u && handlers.Size() == 1u &&
              handlers.GetLast() == 11u &&
              calls == std::vector<Call>{{11u, 16u}},
          "middle removal truncates all following handlers and resumes remaining last");
    Check(handlers.Insert(11u) == 23u && handlers.Size() == 2u,
          "duplicates retain original array semantics");
    calls.clear();
    Check(handlers.Remove(11u) == 0u && handlers.Size() == 0u &&
              handlers.Remove(99u) == 0u && calls.empty(),
          "first duplicate removal and allocated empty removal return zero");
    handlers.ReleaseStorage();
    handlers.ReleaseStorage();
    Check(handlers.Size() == 0u && handlers.OutstandingAllocations() == 0u,
          "storage shutdown is repeatable and releases owned allocations");
    Check(handlers.Insert(55u) == 67u && handlers.GetLast() == 55u,
          "array can be reused after storage shutdown");
    handlers.ReleaseStorage();
}

}  // namespace

int main() {
    try {
        CheckNativeStorageAndDispatch();
        std::cout << "PASS: native event handler storage and dispatch\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
