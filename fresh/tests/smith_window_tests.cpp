#include "pal4/smith_window.h"
#include "pal4/smith_window_factory.h"
#include "pal4/smith_window_factory_runtime.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISmithWindowRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    static constexpr std::uint32_t layout = 0x237du;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::pair<std::uint32_t, std::string>> bindings;
    std::vector<std::tuple<std::uint32_t, pal4::SmithWindowEvent,
                           std::uint32_t>> subscriptions;
    std::vector<std::pair<std::uint32_t, bool>> always_top;
    std::vector<std::pair<std::uint32_t, bool>> read_only;
    std::vector<std::pair<std::uint32_t, bool>> enabled;
    std::vector<std::pair<std::uint32_t, bool>> visible;
    std::vector<std::pair<std::uint32_t, bool>> scrollbars;
    std::vector<std::uint32_t> released;
    std::vector<std::uint32_t> mouse_windows;
    std::vector<std::string> calls;
    std::uint32_t callback_count = 0u;
    std::uint32_t fail_callback_at = 0u;
    std::uint32_t no_subscription_at = 0u;

    void LoadLayout(std::uint32_t owner,
                    std::uint32_t name) override {
        Check(owner == object && name == layout, "layout arguments");
        words[object + 0x24u] = 0x8000u;
        calls.emplace_back("layout");
    }
    void InitializeListAdapter(std::uint32_t adapter) override {
        Check(adapter == object + 0x28u, "list adapter address");
        calls.emplace_back("list-adapter");
    }
    void InitializeNameNode(std::uint32_t node) override {
        Check(node == object + 0xa4u, "name node address");
        calls.emplace_back("name-node");
    }
    void ResetTextFlag(std::uint32_t state,
                       std::uint8_t flag) override {
        Check(state == object + 0xd8u && flag == 0u &&
                  bytes.at(state) == 0x7du,
              "final text state flag reset");
        calls.emplace_back("reset-text");
    }
    std::uint32_t GetWindowByName(std::uint32_t owner,
                                  std::string_view name) override {
        Check(owner == object, "lookup owner");
        const auto window = 0x9000u +
            static_cast<std::uint32_t>(bindings.size()) * 0x100u;
        bindings.emplace_back(window, name);
        calls.emplace_back("lookup");
        return window;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write-word");
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("write-byte");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void SetAlwaysOnTop(std::uint32_t window,
                        bool value) override {
        always_top.emplace_back(window, value);
        calls.emplace_back("top");
    }
    void SetReadOnly(std::uint32_t window, bool value) override {
        read_only.emplace_back(window, value);
        calls.emplace_back("read-only");
    }
    void SetEnabled(std::uint32_t window, bool value) override {
        enabled.emplace_back(window, value);
        calls.emplace_back("enabled");
    }
    void SetVisible(std::uint32_t window, bool value) override {
        visible.emplace_back(window, value);
        calls.emplace_back("visible");
    }
    std::uint32_t AllocateCallback(std::uint32_t bytes_count) override {
        Check(bytes_count == 0x18u, "callback allocation size");
        ++callback_count;
        calls.emplace_back("allocate-callback");
        return callback_count == fail_callback_at
            ? 0u : 0x20000u + callback_count * 0x100u;
    }
    std::uint32_t Subscribe(std::uint32_t window,
                            pal4::SmithWindowEvent event,
                            std::uint32_t callback) override {
        subscriptions.emplace_back(window, event, callback);
        calls.emplace_back("subscribe");
        return subscriptions.size() == no_subscription_at
            ? 0u : 0x30000u +
                static_cast<std::uint32_t>(subscriptions.size());
    }
    void ReleaseSubscription(std::uint32_t subscription) override {
        released.push_back(subscription);
        calls.emplace_back("release");
    }
    void SetShowVertScrollbar(std::uint32_t list,
                              bool value) override {
        scrollbars.emplace_back(list, value);
        calls.emplace_back("scrollbar");
    }
    void ResetState(std::uint32_t owner) override {
        Check(owner == object, "state reset owner");
        calls.emplace_back("reset-state");
    }
    void SetupMouseEventHandlers(std::uint32_t adapter,
                                 std::uint32_t window) override {
        Check(adapter == object + 0x28u, "mouse adapter address");
        mouse_windows.push_back(window);
        calls.emplace_back("mouse-handler");
    }

    std::uint32_t Window(std::uint32_t offset) const {
        return words.at(object + offset);
    }
};

void CheckStateAndWindows() {
    Runtime runtime;
    Check(pal4::SmithWindow::Construct(Runtime::object,
                                      Runtime::layout, runtime) ==
              Runtime::object,
          "constructor returns object");
    Check(runtime.calls[0] == "layout" &&
              runtime.calls[1] == "list-adapter" &&
              runtime.words.at(Runtime::object) == 0x843dc4u &&
              runtime.words.at(Runtime::object + 0x28u) == 0x843d9cu &&
              runtime.words.at(Runtime::object + 0xe8u) == 0x843df4u &&
              runtime.words.at(Runtime::object + 0xf0u) == 0x843ddcu &&
              runtime.words.at(Runtime::object + 0xecu) == Runtime::object &&
              runtime.words.at(Runtime::object + 0xf4u) == Runtime::object,
          "layout, vtables and final self references");
    for (const auto offset : {0x80u, 0x81u, 0xacu})
        Check(runtime.bytes.at(Runtime::object + offset) == 0u,
              "initial state bytes cleared");
    for (const auto offset : {0xb8u, 0xc8u, 0xd8u})
        Check(runtime.bytes.at(Runtime::object + offset) == 0x7du,
              "text record copies low byte of layout pointer");
    for (const auto offset : {0xbcu, 0xc0u, 0xc4u,
                              0xccu, 0xd0u, 0xd4u})
        Check(runtime.words.at(Runtime::object + offset) == 0u,
              "text record words cleared");

    const std::array<std::pair<std::uint32_t, const char*>, 22> expected{{
        {0x60u, "ButtonFound"}, {0x64u, "ButtonSmithery"},
        {0x68u, "ButtonAddMagic"}, {0x40u, "ListEquipment"},
        {0x48u, "EditEquipmentInfo"}, {0x78u, "zuozi"},
        {0x4cu, "EditEquipmentAttr"}, {0x58u, "ButtonLeft"},
        {0x5cu, "ButtonRight"}, {0x44u, "ListPrescription"},
        {0x50u, "EditPrescriptionInfo"},
        {0x54u, "EditPrescriptionAttr"},
        {0x6cu, "StaticEquipmentCaption"},
        {0x70u, "StaticPrescriptionCaption"},
        {0x74u, "StaticImageSystemBar"},
        {0x7cu, "ImgDisusedMask"},
        {0x90u, "WindowEquipment"},
        {0x94u, "WindowEquipmentInfo"},
        {0x98u, "WindowEquipmentAttr"},
        {0x84u, "WindowPrescription"},
        {0x88u, "WindowPrescriptionInfo"},
        {0x8cu, "WindowPrescriptionAttr"},
    }};
    Check(runtime.bindings.size() == expected.size(),
          "exact number of named controls");
    for (std::size_t i = 0; i < expected.size(); ++i) {
        Check(runtime.bindings[i].second ==
                  std::string("SmithWindow/") + expected[i].second &&
                  runtime.Window(expected[i].first) ==
                      runtime.bindings[i].first,
              "control path, order and member offset");
    }
    Check(runtime.read_only ==
              (std::vector<std::pair<std::uint32_t, bool>>{
                  {runtime.Window(0x48u), true},
                  {runtime.Window(0x4cu), true},
                  {runtime.Window(0x50u), true},
                  {runtime.Window(0x54u), true}}) &&
              runtime.enabled.size() == 5u,
          "four text boxes become read-only and disabled");
    Check(runtime.visible ==
              (std::vector<std::pair<std::uint32_t, bool>>{
                  {runtime.Window(0x90u), false},
                  {runtime.Window(0x94u), false},
                  {runtime.Window(0x98u), false},
                  {runtime.Window(0x84u), false},
                  {runtime.Window(0x88u), false},
                  {runtime.Window(0x8cu), false}}),
          "six equipment and prescription panels start hidden");
}

void CheckEventsAndFinish() {
    Runtime runtime;
    (void)pal4::SmithWindow::Construct(Runtime::object,
                                       Runtime::layout, runtime);
    const std::array<std::tuple<std::uint32_t, pal4::SmithWindowEvent,
                                std::uint32_t>, 9> expected{{
        {0x60u, pal4::SmithWindowEvent::Clicked, 0x49e200u},
        {0x64u, pal4::SmithWindowEvent::Clicked, 0x49e360u},
        {0x68u, pal4::SmithWindowEvent::Clicked, 0x49e4c0u},
        {0x58u, pal4::SmithWindowEvent::Clicked, 0x49dd00u},
        {0x5cu, pal4::SmithWindowEvent::Clicked, 0x49dd80u},
        {0x40u, pal4::SmithWindowEvent::HoverRowChanged, 0x49c460u},
        {0x40u, pal4::SmithWindowEvent::MouseClick, 0x49c520u},
        {0x44u, pal4::SmithWindowEvent::HoverRowChanged, 0x49c4a0u},
        {0x44u, pal4::SmithWindowEvent::MouseClick, 0x49d5c0u},
    }};
    Check(runtime.subscriptions.size() == expected.size() &&
              runtime.callback_count == expected.size() &&
              runtime.released.size() == expected.size(),
          "nine callbacks subscribed and subscription references released");
    for (std::size_t i = 0; i < expected.size(); ++i) {
        const auto [offset, event, handler] = expected[i];
        const auto callback = 0x20100u +
            static_cast<std::uint32_t>(i) * 0x100u;
        Check(runtime.subscriptions[i] ==
                  std::make_tuple(runtime.Window(offset), event, callback) &&
                  runtime.words.at(callback) == 0x843d94u &&
                  runtime.words.at(callback + 8u) == handler &&
                  runtime.words.at(callback + 12u) == 0u &&
                  runtime.words.at(callback + 16u) == Runtime::object &&
                  runtime.released[i] == 0x30001u + i,
              "event window, kind, callback layout and release order");
    }
    Check(runtime.scrollbars ==
              (std::vector<std::pair<std::uint32_t, bool>>{
                  {runtime.Window(0x40u), false},
                  {runtime.Window(0x44u), false}}) &&
              runtime.mouse_windows ==
                  (std::vector<std::uint32_t>{
                      runtime.Window(0x60u), runtime.Window(0x64u),
                      runtime.Window(0x68u)}) &&
              runtime.always_top ==
                  (std::vector<std::pair<std::uint32_t, bool>>{
                      {runtime.Window(0x78u), true},
                      {runtime.Window(0x58u), true},
                      {runtime.Window(0x5cu), true},
                      {runtime.Window(0x6cu), false},
                      {0x8000u, true}}) &&
              runtime.enabled.back() ==
                  std::make_pair(runtime.Window(0x6cu), false),
          "scrollbars, mouse handlers and final window settings");
    const auto reset = std::find(runtime.calls.begin(),
                                 runtime.calls.end(), "reset-state");
    Check(reset != runtime.calls.end() &&
              std::distance(reset, runtime.calls.end()) > 3 &&
              *(reset + 1) == "mouse-handler",
          "state reset precedes mouse event setup");
}

void CheckMissingCallbackAndSubscription() {
    Runtime runtime;
    runtime.fail_callback_at = 3u;
    runtime.no_subscription_at = 6u;
    (void)pal4::SmithWindow::Construct(Runtime::object,
                                       Runtime::layout, runtime);
    Check(std::get<2>(runtime.subscriptions[2]) == 0u &&
              runtime.released.size() == 8u,
          "null callback is subscribed; null subscription is not released");
}

struct AllocationRuntime final : pal4::ISmithWindowFactoryRuntime {
    std::uint32_t allocated = Runtime::object;
    unsigned frees = 0u;
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0xfcu, "factory allocation size");
        return allocated;
    }
    std::uint32_t Construct(std::uint32_t,
                            std::uint32_t) override {
        Check(false, "composition must call reconstructed constructor");
        return 0u;
    }
    void Free(std::uint32_t object) override {
        Check(object == allocated, "factory free address");
        ++frees;
    }
};

void CheckFactoryComposition() {
    AllocationRuntime allocation;
    Runtime window;
    pal4::SmithWindowFactoryRuntime adapter(allocation, window);
    Check(pal4::SmithWindowFactory::Create(Runtime::layout, adapter) ==
              Runtime::object &&
              window.bindings.size() == 22u &&
              window.subscriptions.size() == 9u &&
              allocation.frees == 0u,
          "factory creates and initializes reconstructed SmithWindow");
}

}  // namespace

int main() {
    try {
        CheckStateAndWindows();
        CheckEventsAndFinish();
        CheckMissingCallbackAndSubscription();
        CheckFactoryComposition();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
