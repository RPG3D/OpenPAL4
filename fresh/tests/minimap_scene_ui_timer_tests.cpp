#include "pal4/minimap_scene_ui_timer.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
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

struct FakeEntryRuntime final : pal4::ITimerQueueEntryRuntime {
    std::vector<std::string>& calls;
    std::map<std::uint32_t, std::uint8_t> counts{{0x3001u, 2u},
                                                  {0x4001u, 2u}};
    std::uint32_t free_result = 0x5678u;

    explicit FakeEntryRuntime(std::vector<std::string>& calls) : calls(calls) {}
    void EraseName(pal4::TimerQueueEntryState&,
                   std::uint32_t, std::uint32_t) override {
        Check(false, "timer initializer does not erase entry name");
    }
    void SetTextFlag(pal4::TimerQueueEntryState&,
                     std::uint8_t) override {
        Check(false, "timer initializer does not set text flag");
    }
    std::uint8_t ReadReferenceCount(std::uint32_t data) override {
        Check(data == 0x3001u || data == 0x4001u,
              "entry or incoming text reference");
        calls.emplace_back(data == 0x3001u ? "entry-count" : "text-count");
        return counts[data];
    }
    void WriteReferenceCount(std::uint32_t data,
                             std::uint8_t count) override {
        counts[data] = count;
        calls.emplace_back(data == 0x3001u ? "entry-dec" : "text-dec");
    }
    bool AllocateName(pal4::TimerQueueEntryState&,
                      std::uint32_t, std::uint8_t) override {
        Check(false, "timer initializer does not allocate entry name");
        return false;
    }
    void CopyName(std::uint32_t, std::uint32_t,
                  std::uint32_t) override {
        Check(false, "timer initializer does not copy entry name");
    }
    void SetNameSize(pal4::TimerQueueEntryState&,
                     std::uint32_t) override {
        Check(false, "timer initializer does not size entry name");
    }
    void ReleaseCallback(std::uint32_t callback,
                         std::uint32_t deleting) override {
        Check(callback == 0x5000u && deleting == 1u,
              "owned callback deleting destructor");
        calls.emplace_back("release-callback");
    }
    std::uint32_t FreeName(std::uint32_t header) override {
        Check(header == 0x3000u || header == 0x4000u,
              "entry or incoming text header");
        calls.emplace_back(header == 0x3000u ? "free-entry" : "free-text");
        return free_result;
    }
};

struct Runtime final : pal4::IMinimapSceneUiTimerRuntime {
    std::vector<std::string> calls;
    FakeEntryRuntime entry_runtime{calls};
    std::map<std::uint32_t, std::uint32_t> callback_words;
    std::uint32_t callback = 0x5000u;
    bool throw_insert = false;
    bool throw_assign = false;
    bool throw_execute = false;
    bool throw_window_text = false;
    bool inserted_owns_callback = false;
    std::string error_text;
    std::u32string window_text;
    std::uint32_t window_text_target = 0u;
    std::uint32_t empty_text_reads = 0u;
    std::uint8_t text_flag = 0xffu;
    std::uint32_t realloc_length = 0u;
    std::uint32_t copied_destination = 0u;
    std::uint32_t terminator_address = 0u;

    std::uint32_t FrameManager() override {
        calls.emplace_back("frame");
        return 0x8000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x81a0u || address == 0x9004u,
              "timer queue and cursor address");
        calls.emplace_back(address == 0x81a0u ? "queue" : "cursor");
        return address == 0x81a0u ? 0x9000u : 0xabcu;
    }
    void RemoveTimer(std::uint32_t queue,
                     std::uint32_t identifier) override {
        Check(queue == 0x9000u && identifier == 0x50d0u,
              "remove existing timer by floor identifier");
        calls.emplace_back("remove");
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x10u, "16-byte timer callback");
        calls.emplace_back("allocate");
        return callback;
    }
    void WriteCallbackWord(std::uint32_t address,
                           std::uint32_t value) override {
        callback_words[address] = value;
        calls.emplace_back("callback-word");
    }
    void CopyFloorAttribute(pal4::TimerQueueEntryState& entry,
                            std::uint32_t source) override {
        Check(source == 0x50d0u, "copy floor identifier from owner");
        entry.name.data = 0x3001u;
        calls.emplace_back("copy-identifier");
    }
    void InsertTimer(std::uint32_t queue,
                     std::uint32_t period,
                     std::uint32_t queue_cursor,
                     const pal4::TimerQueueEntryState& entry) override {
        Check(queue == 0x9000u && period == 0x1234u &&
                  queue_cursor == 0xabcu &&
                  entry.name.data == 0x3001u &&
                  entry.initial_period_bits == 0x3f800000u &&
                  entry.current_period_bits == 0x3f800000u &&
                  entry.owns_callback == (callback != 0u) &&
                  entry.callback == callback,
              "insert complete timer entry");
        inserted_owns_callback = entry.owns_callback;
        calls.emplace_back("insert");
        if (throw_insert) {
            throw std::runtime_error("insert");
        }
    }
    void AssignText(std::uint32_t destination,
                    const pal4::StringBufferState& source) override {
        Check(destination == 0x50e0u &&
                  (source.data == 0x4001u || source.data == 0u) &&
                  source.length == 3u,
              "assign incoming timer text at owner offset");
        calls.emplace_back("assign");
        if (throw_assign) {
            throw std::runtime_error("assign");
        }
    }
    void ExecuteTimerScript(std::uint32_t owner) override {
        Check(owner == 0x5000u, "execute script with timer owner");
        calls.emplace_back("execute");
        if (throw_execute) {
            throw std::runtime_error("execute");
        }
    }
    std::string_view EmptyErrorText() override {
        ++empty_text_reads;
        calls.emplace_back("empty-text");
        return error_text;
    }
    void SetTextErrorFlag(pal4::MinimapSceneUiState& state,
                          std::uint32_t buffer,
                          std::uint8_t flag) override {
        Check(buffer == 0x50e0u, "timer text buffer error flag");
        text_flag = flag;
        state.WriteByte(0xe0u, flag);
        calls.emplace_back("flag");
    }
    void ReallocateText(pal4::MinimapSceneUiState& state,
                        std::uint32_t buffer,
                        std::uint32_t length) override {
        Check(buffer == 0x50e0u, "timer text reallocation target");
        realloc_length = length;
        state.WriteWord(0xe4u, 0x6000u);
        state.WriteWord(0xecu, length);
        calls.emplace_back("reallocate");
    }
    void CopyText(std::uint32_t destination,
                  std::string_view text) override {
        Check(text == error_text, "timer text copy source");
        copied_destination = destination;
        calls.emplace_back("copy-text");
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        Check(value == 0u, "timer text terminator");
        terminator_address = address;
        calls.emplace_back("terminator");
    }
    void SetWindowText(std::uint32_t window,
                       std::u32string_view text) override {
        window_text_target = window;
        window_text.assign(text);
        calls.emplace_back("set-window-text");
        if (throw_window_text) {
            throw std::runtime_error("set window text");
        }
    }
    pal4::ITimerQueueEntryRuntime& EntryRuntime() override {
        return entry_runtime;
    }
};

std::uint32_t Initialize(Runtime& runtime,
                         pal4::MinimapSceneUiState& state,
                         std::uint32_t data = 0x4001u) {
    return pal4::MinimapSceneUiTimer::Initialize(
        state, 0x5000u, 0x1234u, {data, 3u, 3u}, runtime);
}

}  // namespace

int main() {
    {
        Runtime runtime;
        pal4::MinimapSceneUiState state;
        Check(Initialize(runtime, state) == 0x4000u &&
                  state.ReadWord(0xf0u) == 0x1234u &&
                  runtime.callback_words ==
                      std::map<std::uint32_t, std::uint32_t>{
                          {0x5000u, pal4::MinimapSceneUiTimer::kCallbackVtable},
                          {0x5004u, 0x5000u},
                          {0x5008u, pal4::MinimapSceneUiTimer::kExecuteFunction},
                          {0x500cu, 0u}} &&
                  runtime.entry_runtime.counts[0x3001u] == 1u &&
                  runtime.entry_runtime.counts[0x4001u] == 1u &&
                  runtime.calls == std::vector<std::string>{
                                       "frame", "queue", "remove", "frame",
                                       "queue", "allocate", "callback-word",
                                       "callback-word", "callback-word",
                                       "callback-word", "copy-identifier",
                                       "cursor", "insert", "release-callback",
                                       "entry-count", "entry-dec", "assign",
                                       "execute", "text-count", "text-dec"},
              "timer setup order, callback layout and both text releases");
    }
    {
        Runtime runtime;
        runtime.callback = 0u;
        runtime.entry_runtime.counts[0x4001u] = 0xffu;
        pal4::MinimapSceneUiState state;
        Check(Initialize(runtime, state) == runtime.entry_runtime.free_result &&
                  runtime.callback_words.empty() &&
                  !runtime.inserted_owns_callback &&
                  runtime.calls.back() == "free-text" &&
                  runtime.entry_runtime.counts[0x3001u] == 1u,
              "allocation failure still inserts timer and frees sentinel text");
    }
    for (const auto count : {0u, 0xffu}) {
        Runtime runtime;
        runtime.entry_runtime.counts[0x4001u] =
            static_cast<std::uint8_t>(count);
        pal4::MinimapSceneUiState state;
        Check(Initialize(runtime, state) == runtime.entry_runtime.free_result &&
                  runtime.calls.back() == "free-text",
              "zero and sentinel incoming counts free text header");
    }
    {
        Runtime runtime;
        pal4::MinimapSceneUiState state;
        Check(Initialize(runtime, state, 0u) == 0u &&
                  runtime.calls.back() == "execute",
              "empty incoming text has no reference release");
    }
    for (const auto failure : {0, 1, 2}) {
        Runtime runtime;
        runtime.throw_insert = failure == 0;
        runtime.throw_assign = failure == 1;
        runtime.throw_execute = failure == 2;
        pal4::MinimapSceneUiState state;
        bool caught = false;
        try {
            (void)Initialize(runtime, state);
        } catch (const std::runtime_error&) {
            caught = true;
        }
        Check(caught && runtime.entry_runtime.counts[0x3001u] == 1u &&
                  runtime.entry_runtime.counts[0x4001u] == 1u &&
                  runtime.calls[runtime.calls.size() - 2u] == "text-count" &&
                  runtime.calls.back() == "text-dec" &&
                  (failure != 0 ||
                   std::find(runtime.calls.begin(), runtime.calls.end(),
                             "assign") == runtime.calls.end()),
              "insert, assignment and script failures release both resources");
    }
    {
        Runtime runtime;
        pal4::MinimapSceneUiState state;
        state.WriteWord(0xbcu, 0x8000u);
        state.WriteWord(0xe4u, 0x4001u);
        state.WriteWord(0xf0u, 0x1234u);
        pal4::MinimapSceneUiTimer::UpdateText(state, 0x5000u, runtime);
        Check(state.ReadWord(0xf0u) == 0u && runtime.text_flag == 0u &&
                  runtime.entry_runtime.counts[0x4001u] == 1u &&
                  runtime.window_text.empty() &&
                  runtime.window_text_target == 0x8000u &&
                  runtime.empty_text_reads == 2u &&
                  runtime.calls == std::vector<std::string>{
                                       "frame", "queue", "remove",
                                       "empty-text", "text-count", "text-dec",
                                       "flag", "empty-text", "set-window-text"},
              "shared empty text detaches and clears the timer display");
    }
    {
        Runtime runtime;
        runtime.error_text = "A\xff";
        pal4::MinimapSceneUiState state;
        state.WriteWord(0xbcu, 0x8000u);
        state.WriteWord(0xe4u, 0x4001u);
        state.WriteWord(0xecu, 31u);
        pal4::MinimapSceneUiTimer::UpdateText(state, 0x5000u, runtime);
        Check(runtime.realloc_length == 2u &&
                  runtime.copied_destination == 0x6000u &&
                  state.ReadWord(0xe8u) == 2u &&
                  runtime.terminator_address == 0x6002u &&
                  runtime.window_text == std::u32string{U'A', 0xffu} &&
                  runtime.calls == std::vector<std::string>{
                                       "frame", "queue", "remove",
                                       "empty-text", "text-count",
                                       "reallocate", "copy-text", "terminator",
                                       "empty-text", "set-window-text"},
              "shared nonempty text reallocates and widens unsigned bytes");
    }
    {
        Runtime runtime;
        runtime.error_text = "ok";
        runtime.entry_runtime.counts[0x4001u] = 0u;
        pal4::MinimapSceneUiState state;
        state.WriteWord(0xbcu, 0x8000u);
        state.WriteWord(0xe4u, 0x4001u);
        state.WriteWord(0xecu, 10u);
        pal4::MinimapSceneUiTimer::UpdateText(state, 0x5000u, runtime);
        Check(runtime.realloc_length == 0u && runtime.text_flag == 0xffu &&
                  runtime.copied_destination == 0x4001u &&
                  runtime.terminator_address == 0x4003u &&
                  runtime.window_text == U"ok",
              "unshared text reuses sufficient small capacity");
    }
    for (const auto capacity : {0u, 32u}) {
        Runtime runtime;
        runtime.error_text = "ok";
        runtime.entry_runtime.counts[0x4001u] = 0xffu;
        pal4::MinimapSceneUiState state;
        state.WriteWord(0xbcu, 0x8000u);
        state.WriteWord(0xe4u, 0x4001u);
        state.WriteWord(0xecu, capacity);
        pal4::MinimapSceneUiTimer::UpdateText(state, 0x5000u, runtime);
        Check(runtime.text_flag == 1u && runtime.realloc_length == 2u &&
                  runtime.copied_destination == 0x6000u,
              "small insufficient and large capacity reallocate text");
    }
    {
        Runtime runtime;
        runtime.throw_window_text = true;
        pal4::MinimapSceneUiState state;
        state.WriteWord(0xbcu, 0x8000u);
        bool caught = false;
        try {
            pal4::MinimapSceneUiTimer::UpdateText(state, 0x5000u, runtime);
        } catch (const std::runtime_error&) {
            caught = true;
        }
        Check(caught && runtime.calls.back() == "set-window-text",
              "CEGUI text failure propagates after timer removal");
    }
}
