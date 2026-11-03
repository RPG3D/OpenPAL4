#include "pal4/numeric_spinner.h"

#include <algorithm>
#include <cstdint>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::INumericSpinnerRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::string display;
    std::string name;
    std::unordered_map<std::uint32_t, float> floats;
    std::unordered_map<std::uint32_t, float> widths;
    std::unordered_map<std::uint32_t, float> heights;
    std::unordered_map<std::uint32_t, float> xs;
    std::unordered_map<std::uint32_t, float> ys;
    std::array<float, 4> assigned_rect{};
    std::string reset_text;
    std::string item_text;
    std::unordered_map<std::uint32_t, bool> disabled;
    std::vector<std::pair<std::uint32_t, bool>> enabled_changes;
    std::uint32_t selected_list = 0u;
    std::uint32_t selected_row = 0u;
    std::vector<std::uint32_t> event_keys;
    bool create_sets_root = false;
    std::uint8_t initial_byte = 0x72u;
    std::uint32_t allocated_data = 0x9001u;

    void ConstructListboxItem(std::uint32_t item,
                              std::string_view text) override {
        Check(item == 0x1000u, "base item address");
        display = text;
        calls.emplace_back("base");
    }
    std::uint8_t InitialNameByte() override { return initial_byte; }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("byte:" + std::to_string(address));
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("word:" + std::to_string(address));
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void SetErrorFlag(std::uint32_t address,
                      std::uint8_t value) override {
        Check(address == 0x1000u + 444u && value == 1u,
              "constructor error flag");
        calls.emplace_back("flag");
    }
    void ReallocateName(std::uint32_t address,
                        std::uint32_t length) override {
        Check(address == 0x1000u + 444u && length != 0u,
              "name allocation arguments");
        words[address + 4u] = allocated_data;
        calls.emplace_back("reallocate");
    }
    void CopyName(std::uint32_t destination,
                  std::string_view text) override {
        Check(destination == allocated_data, "name copy destination");
        name = text;
        calls.emplace_back("copy");
    }
    void CreateControls(std::uint32_t item) override {
        Check(item == 0x1000u, "control owner");
        calls.emplace_back("create");
        if (create_sets_root) words[item + 436u] = 0x2000u;
    }
    void RemoveChild(std::uint32_t parent,
                     std::uint32_t child) override {
        calls.emplace_back("remove:" + std::to_string(parent) + ":" +
                           std::to_string(child));
    }
    void DestroyWindow(std::uint32_t window) override {
        calls.emplace_back("destroy:" + std::to_string(window));
    }
    void Free(std::uint32_t address) override {
        calls.emplace_back("free:" + std::to_string(address));
    }
    std::uint32_t BaseItemVtable() override { return 0x8412f4u; }
    void DestroyString(std::uint32_t address) override {
        calls.emplace_back("string:" + std::to_string(address));
    }
    void AddChild(std::uint32_t parent,
                  std::uint32_t child) override {
        calls.emplace_back("add:" + std::to_string(parent) + ":" +
                           std::to_string(child));
    }
    void SetRect(std::uint32_t window,
                 const std::array<float, 4>& rect) override {
        Check(window == 0x2000u, "layout root");
        assigned_rect = rect;
        calls.emplace_back("rect");
    }
    float ReadFloat(std::uint32_t address) override {
        calls.emplace_back("float");
        return floats[address];
    }
    std::int32_t FloatToLong(float value) override {
        calls.emplace_back("ftol");
        return static_cast<std::int32_t>(std::lrint(value));
    }
    float GetWidth(std::uint32_t window) override {
        calls.emplace_back("width:" + std::to_string(window));
        return widths[window];
    }
    float GetHeight(std::uint32_t window) override {
        calls.emplace_back("height:" + std::to_string(window));
        return heights[window];
    }
    void SetX(std::uint32_t window, float value) override {
        xs[window] = value;
        calls.emplace_back("x:" + std::to_string(window));
    }
    void SetY(std::uint32_t window, float value) override {
        ys[window] = value;
        calls.emplace_back("y:" + std::to_string(window));
    }
    void SetEnabled(std::uint32_t window, bool enabled) override {
        enabled_changes.emplace_back(window, enabled);
        calls.emplace_back("enable:" + std::to_string(window) +
                           (enabled ? ":1" : ":0"));
    }
    std::string_view ReadCString(std::uint32_t address) override {
        Check(address == 0x8dc780u, "reset text global");
        calls.emplace_back("read_text");
        return reset_text;
    }
    void SetText(std::uint32_t window,
                 std::string_view text) override {
        Check(window == 0x2300u, "text window");
        reset_text = text;
        calls.emplace_back("set_text");
    }
    bool IsDisabled(std::uint32_t window, bool inherited) override {
        Check(!inherited, "disabled query ignores inherited state");
        calls.emplace_back("disabled:" + std::to_string(window));
        return disabled[window];
    }
    void SetItemText(std::uint32_t item,
                     std::string_view text) override {
        Check(item == 0x1000u, "item text owner");
        item_text = text;
        calls.emplace_back("item_text");
    }
    void InvokeChangeCallback(std::uint32_t callback) override {
        Check(callback == 0x5000u, "change callback address");
        calls.emplace_back("callback");
    }
    void SelectGridCell(std::uint32_t list,
                        std::uint32_t row,
                        std::uint32_t column,
                        bool selected) override {
        Check(column == 0u && selected, "grid selection arguments");
        selected_list = list;
        selected_row = row;
        calls.emplace_back("select");
    }
    void DispatchUiEvent(std::uint32_t key,
                         std::uint32_t row,
                         std::uint32_t column) override {
        Check(row == 7u && column == 0u,
              "spinner event payload");
        event_keys.push_back(key);
        calls.emplace_back("event");
    }
};

std::size_t Find(const std::vector<std::string>& calls,
                 const std::string& call) {
    for (std::size_t i = 0; i < calls.size(); ++i)
        if (calls[i] == call) return i;
    throw std::runtime_error("missing call: " + call);
}

void CheckConstruct() {
    for (const auto value : {0, 42, -42, -2147483647 - 1}) {
        Runtime runtime;
        Check(pal4::NumericSpinner::Construct(
                  0x1000u, "TradeItem7", value, runtime) == 0x1000u,
              "constructor return");
        Check(runtime.display == std::to_string(value) &&
                  runtime.name == "TradeItem7" &&
                  runtime.bytes[0x1000u + 444u] == 0x72u &&
                  runtime.words[0x1000u + 448u] == 0x9001u &&
                  runtime.words[0x1000u + 452u] == 10u &&
                  runtime.bytes[0x9001u + 10u] == 0u,
              "display and name storage");
        Check(runtime.words[0x1000u] == pal4::NumericSpinner::kVtable &&
                  runtime.words[0x1000u + 440u] ==
                      static_cast<std::uint32_t>(value) &&
                  runtime.words[0x1000u + 424u] == 0u &&
                  runtime.words[0x1000u + 428u] == 0u &&
                  runtime.words[0x1000u + 432u] == 0u &&
                  runtime.words[0x1000u + 436u] == 0u &&
                  runtime.words[0x1000u + 468u] == 0x80000000u &&
                  runtime.words[0x1000u + 472u] == 0x7fffffffu &&
                  runtime.words[0x1000u + 476u] == 0u,
              "constructor field layout");
        Check(Find(runtime.calls, "base") < Find(runtime.calls, "flag") &&
                  Find(runtime.calls, "flag") <
                      Find(runtime.calls, "reallocate") &&
                  Find(runtime.calls, "reallocate") <
                      Find(runtime.calls, "copy") &&
                  Find(runtime.calls, "copy") < Find(runtime.calls, "create"),
              "constructor operation order");
    }

    Runtime empty;
    empty.initial_byte = 0xa5u;
    (void)pal4::NumericSpinner::Construct(0x1000u, "", 0, empty);
    Check(empty.calls.size() != 0u &&
              Find(empty.calls, "flag") < Find(empty.calls, "create") &&
              empty.words[0x1000u + 448u] == 0u &&
              empty.bytes[0x1000u + 444u] == 0xa5u,
          "empty name and supplied initial byte");
    Check(empty.name.empty(), "empty name skips copy");

    Runtime embedded_null;
    (void)pal4::NumericSpinner::Construct(
        0x1000u, std::string_view("A\0B", 3), 1, embedded_null);
    Check(embedded_null.name == "A" &&
              embedded_null.words[0x1000u + 452u] == 1u,
          "constructor uses C string length");
}

void CheckCopyPair() {
    Runtime runtime;
    runtime.words[0x1000u + 440u] = 99u;
    runtime.words[0x1000u + 460u] = 0xabcdef01u;
    runtime.words[0x1000u + 464u] = 0x80000000u;
    Check(pal4::NumericSpinner::CopyPair(0x1000u, 0x2000u, runtime) ==
              0x2000u &&
              runtime.words[0x2000u] == 0xabcdef01u &&
              runtime.words[0x2004u] == 0x80000000u,
          "pair copies +460/+464 and returns destination");
    pal4::NumericSpinner::NoopThreeArgs(1u, 2u, 3u);
}

void CheckDestruct(const std::uint8_t references,
                   const bool with_root, const bool with_parent) {
    Runtime runtime;
    runtime.words[0x1000u + 424u] = 0x2100u;
    runtime.words[0x1000u + 428u] = 0x2200u;
    runtime.words[0x1000u + 432u] = 0x2300u;
    runtime.words[0x1000u + 436u] = with_root ? 0x2000u : 0u;
    runtime.words[0x2000u + 68u] = with_parent ? 0x3000u : 0u;
    runtime.words[0x1000u + 448u] = 0x9001u;
    runtime.bytes[0x9000u] = references;
    pal4::NumericSpinner::Destruct(0x1000u, runtime);
    if (with_root) {
        Check(Find(runtime.calls, "remove:8192:8448") <
                  Find(runtime.calls, "remove:8192:8704") &&
                  Find(runtime.calls, "remove:8192:8704") <
                  Find(runtime.calls, "remove:8192:8960") &&
                  Find(runtime.calls, "remove:8192:8960") <
                  Find(runtime.calls, "destroy:8448") &&
                  Find(runtime.calls, "destroy:8448") <
                  Find(runtime.calls, "destroy:8704") &&
                  Find(runtime.calls, "destroy:8704") <
                  Find(runtime.calls, "destroy:8960") &&
                  Find(runtime.calls, "destroy:8960") <
                  Find(runtime.calls, "destroy:8192"),
              "window detach and destruction order");
        if (with_parent)
            Check(Find(runtime.calls, "remove:12288:8192") <
                      Find(runtime.calls, "destroy:8448"),
                  "parent removal precedes window destruction");
        Check(runtime.words[0x1000u + 436u] == 0u,
              "root pointer cleared");
    } else {
        for (const auto& call : runtime.calls)
            Check(call.find("destroy:") != 0u &&
                      call.find("remove:") != 0u,
                  "no root skips window cleanup");
    }
    if (references == 0u || references == 0xffu)
        Check(Find(runtime.calls, "free:36864") <
                  Find(runtime.calls, "string:4252"),
              "unshared name freed");
    else
        Check(runtime.bytes[0x9000u] == references - 1u,
              "shared name reference decremented");
    Check(runtime.words[0x1000u] == 0x8412f4u &&
              runtime.words[0x1000u + 448u] == 0u &&
              runtime.words[0x1000u + 452u] == 0u &&
              runtime.words[0x1000u + 456u] == 0u &&
              Find(runtime.calls, "string:4252") <
                  Find(runtime.calls, "string:4100"),
          "base item and string cleanup");
}

void CheckLayout(const bool horizontal, const bool create_root,
                 const bool attach_root) {
    Runtime runtime;
    runtime.create_sets_root = create_root;
    runtime.words[0x1000u + 436u] = create_root ? 0u : 0x2000u;
    runtime.words[0x1000u + 316u] = 0x3000u;
    runtime.words[0x1000u + 424u] = 0x2100u;
    runtime.words[0x1000u + 428u] = 0x2200u;
    runtime.words[0x1000u + 432u] = 0x2300u;
    runtime.words[0x2000u + 68u] = attach_root ? 0u : 0x3000u;
    runtime.floats[0x3000u + 1988u] = 2.6f;
    runtime.widths[0x2100u] = 10.0f;
    runtime.widths[0x2300u] = 40.0f;
    runtime.heights[0x2100u] = 8.0f;
    runtime.heights[0x2300u] = 9.0f;
    runtime.heights[0x2200u] = 7.0f;
    const std::array<float, 4> rect = horizontal
        ? std::array<float, 4>{5.0f, 105.0f, 0.0f, 0.0f}
        : std::array<float, 4>{6.0f, 106.0f, 0.0f, 0.0f};
    pal4::NumericSpinner::Layout(0x1000u, 5.0f, rect, runtime);
    Check(runtime.assigned_rect == rect &&
              runtime.xs[0x2100u] == 0.0f &&
              runtime.xs[0x2300u] == 10.0f &&
              runtime.xs[0x2200u] == 50.0f,
          "layout rect and horizontal positions");
    Check(runtime.ys[0x2100u] == (horizontal ? 0.0f : 89.0f) &&
              runtime.ys[0x2300u] == (horizontal ? 0.0f : 88.0f) &&
              runtime.ys[0x2200u] == (horizontal ? 0.0f : 90.0f),
          "layout branch and vertical positions");
    Check(Find(runtime.calls, "rect") <
              Find(runtime.calls, "x:8448") &&
              (create_root ==
               (std::find(runtime.calls.begin(), runtime.calls.end(),
                          "create") != runtime.calls.end())),
          "root creation and rect order");
    Check(attach_root ==
              (std::find(runtime.calls.begin(), runtime.calls.end(),
                         "add:12288:8192") != runtime.calls.end()),
          "parent attach gate");
    if (!horizontal)
        Check(Find(runtime.calls, "float") < Find(runtime.calls, "ftol"),
              "vertical offset conversion");
}

void CheckResetAndRemove() {
    Runtime runtime;
    runtime.words[0x1000u + 424u] = 0x2100u;
    runtime.words[0x1000u + 428u] = 0x2200u;
    runtime.words[0x1000u + 432u] = 0x2300u;
    runtime.words[0x1000u + 440u] = 5u;
    runtime.words[0x1000u + 468u] = 1u;
    runtime.words[0x1000u + 472u] = 10u;
    runtime.reset_text = "";
    pal4::NumericSpinner::Reset(0x1000u, runtime);
    Check(Find(runtime.calls, "enable:8448:1") <
              Find(runtime.calls, "enable:8704:1") &&
              Find(runtime.calls, "enable:8704:1") <
              Find(runtime.calls, "set_text") &&
              Find(runtime.calls, "set_text") <
              Find(runtime.calls, "word:4568") &&
              runtime.words[0x1000u + 440u] == 0u &&
              runtime.words[0x1000u + 468u] == 0u &&
              runtime.words[0x1000u + 472u] == 0u,
          "reset order and fields");

    pal4::NumericSpinner::RemoveFromParent(0x1000u, runtime);
    runtime.words[0x1000u + 436u] = 0x2000u;
    pal4::NumericSpinner::RemoveFromParent(0x1000u, runtime);
    runtime.words[0x2000u + 68u] = 0x3000u;
    pal4::NumericSpinner::RemoveFromParent(0x1000u, runtime);
    Check(std::count(runtime.calls.begin(), runtime.calls.end(),
                     "remove:12288:8192") == 1,
          "only attached root removed");
}

Runtime ValueRuntime(std::int32_t previous = 0) {
    Runtime runtime;
    runtime.words[0x1000u + 440u] =
        static_cast<std::uint32_t>(previous);
    runtime.words[0x1000u + 468u] =
        static_cast<std::uint32_t>(-10);
    runtime.words[0x1000u + 472u] = 10u;
    runtime.words[0x1000u + 424u] = 0x2100u;
    runtime.words[0x1000u + 428u] = 0x2200u;
    runtime.words[0x1000u + 432u] = 0x2300u;
    return runtime;
}

void CheckUpdateValue() {
    auto ordinary = ValueRuntime();
    pal4::NumericSpinner::UpdateValue(0x1000u, 4, ordinary);
    Check(ordinary.words[0x1000u + 440u] == 4u &&
              ordinary.item_text == "4" && ordinary.reset_text == "4" &&
              ordinary.enabled_changes ==
                  (std::vector<std::pair<std::uint32_t, bool>>{
                      {0x2100u, true}, {0x2200u, true}}) &&
              Find(ordinary.calls, "disabled:8704") <
                  Find(ordinary.calls, "item_text") &&
              Find(ordinary.calls, "item_text") <
                  Find(ordinary.calls, "set_text"),
          "ordinary positive value update");

    auto upper = ValueRuntime();
    pal4::NumericSpinner::UpdateValue(0x1000u, 15, upper);
    Check(upper.words[0x1000u + 440u] == 10u &&
              upper.item_text == "10" &&
              upper.enabled_changes.back() ==
                  std::pair<std::uint32_t, bool>{0x2200u, false},
          "signed upper clamp and right button gate");

    auto lower = ValueRuntime();
    pal4::NumericSpinner::UpdateValue(0x1000u, -20, lower);
    Check(lower.words[0x1000u + 440u] ==
                  static_cast<std::uint32_t>(-10) &&
              lower.item_text == "-10" &&
              lower.enabled_changes.front() ==
                  std::pair<std::uint32_t, bool>{0x2100u, false},
          "signed lower clamp and left button gate");

    auto blocked = ValueRuntime();
    blocked.disabled[0x2200u] = true;
    pal4::NumericSpinner::UpdateValue(0x1000u, 1, blocked);
    Check(blocked.words[0x1000u + 440u] == 0u &&
              blocked.item_text.empty() &&
              blocked.enabled_changes.empty(),
          "disabled direction button stops update");

    auto wrapped = ValueRuntime(-2147483647 - 1);
    wrapped.disabled[0x2100u] = true;
    pal4::NumericSpinner::UpdateValue(0x1000u, 2147483647, wrapped);
    Check(wrapped.words[0x1000u + 440u] == 0x80000000u &&
              Find(wrapped.calls, "disabled:8448") == 0u,
          "32-bit subtraction wrap chooses left button");

    auto no_button = ValueRuntime();
    no_button.words[0x1000u + 424u] = 0u;
    no_button.words[0x1000u + 428u] = 0u;
    no_button.words[0x1000u + 432u] = 0u;
    pal4::NumericSpinner::UpdateValue(0x1000u, -1, no_button);
    Check(no_button.item_text == "-1" &&
              no_button.enabled_changes.empty() &&
              std::find(no_button.calls.begin(), no_button.calls.end(),
                        "set_text") == no_button.calls.end(),
          "missing controls still update item text");

    auto callback = ValueRuntime();
    callback.words[0x1000u + 476u] = 0x5000u;
    callback.bytes[0x1000u + 421u] = 1u;
    pal4::NumericSpinner::UpdateValue(0x1000u, 3, callback);
    Check(Find(callback.calls, "callback") <
              Find(callback.calls, "enable:8704:0") &&
              callback.enabled_changes.size() == 4u &&
              callback.enabled_changes[2] ==
                  std::pair<std::uint32_t, bool>{0x2200u, false} &&
              callback.enabled_changes[3] ==
                  std::pair<std::uint32_t, bool>{0x2100u, false},
          "callback precedes item-disabled override");
}

void CheckInputCallbacks() {
    auto runtime = ValueRuntime(2);
    runtime.words[0x1000u + 308u] = 7u;
    runtime.words[0x1000u + 316u] = 0x3000u;
    Check(pal4::NumericSpinner::OnLeftClick(0x1000u, runtime) == 0u &&
              runtime.words[0x1000u + 440u] == 1u &&
              runtime.selected_list == 0x3000u &&
              runtime.selected_row == 7u &&
              Find(runtime.calls, "item_text") <
                  Find(runtime.calls, "select"),
          "left click decrements and selects list cell");
    runtime.calls.clear();
    Check(pal4::NumericSpinner::OnRightClick(0x1000u, runtime) == 0u &&
              runtime.words[0x1000u + 440u] == 2u &&
              runtime.selected_list == 0x3000u &&
              runtime.selected_row == 7u &&
              Find(runtime.calls, "item_text") <
                  Find(runtime.calls, "select"),
          "right click increments and selects list cell");

    runtime.words[0x4000u + 28u] = 0u;
    Check(pal4::NumericSpinner::OnLeftDown(
              0x1000u, 0x4000u, runtime) == 1u &&
              pal4::NumericSpinner::OnRightDown(
                  0x1000u, 0x4000u, runtime) == 1u &&
              pal4::NumericSpinner::OnButtonUp(
                  0x1000u, runtime) == 1u &&
              runtime.event_keys ==
                  (std::vector<std::uint32_t>{
                      0x8b185cu, 0x8b1860u, 0x8b1864u}),
          "mouse down and up event keys");
    runtime.words[0x4000u + 28u] = 1u;
    Check(pal4::NumericSpinner::OnLeftDown(
              0x1000u, 0x4000u, runtime) == 1u &&
              pal4::NumericSpinner::OnRightDown(
                  0x1000u, 0x4000u, runtime) == 1u &&
              runtime.event_keys.size() == 3u,
          "nonzero event field suppresses down dispatch");

    auto blocked = ValueRuntime(2);
    blocked.words[0x1000u + 308u] = 7u;
    blocked.words[0x1000u + 316u] = 0x3000u;
    blocked.disabled[0x2100u] = true;
    Check(pal4::NumericSpinner::OnLeftClick(0x1000u, blocked) == 0u &&
              blocked.words[0x1000u + 440u] == 2u &&
              blocked.selected_row == 7u,
          "blocked value change still selects cell");
}

}  // namespace

int main() {
    try {
        CheckConstruct();
        CheckCopyPair();
        CheckDestruct(0u, true, true);
        CheckDestruct(0xffu, true, false);
        CheckDestruct(2u, false, false);
        CheckLayout(true, false, true);
        CheckLayout(false, true, false);
        CheckResetAndRemove();
        CheckUpdateValue();
        CheckInputCallbacks();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
