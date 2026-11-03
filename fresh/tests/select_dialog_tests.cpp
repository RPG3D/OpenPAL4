#include "pal4/select_dialog.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISelectDialogRuntime {
    static constexpr std::uint32_t kObject = 0x1000u;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;
    std::vector<std::string> names;
    std::vector<std::uint32_t> callbacks;
    std::vector<std::uint32_t> subscribed_windows;
    std::vector<std::string> subscribed_events;
    std::vector<std::uint32_t> subscribed_callbacks;
    std::string fail_at;
    bool null_callbacks = false;

    Runtime() {
        words[kObject + 36u] = 0x3000u;
        words[0x4020u + 68u] = 0x5000u;
        words[0x7000u + 4u] = 1u;
        words[0x7100u + 4u] = 3u;
    }

    void MaybeFail(const std::string& step) {
        if (fail_at == step) throw std::runtime_error(step);
    }
    void LoadLayout(std::uint32_t object,
                    const char* layout) override {
        Check(object == kObject &&
                  std::string(layout) == "SelectDialog",
              "layout arguments");
        events.emplace_back("layout");
        MaybeFail("layout");
    }
    void ConstructNavigation(std::uint32_t address) override {
        Check(address == kObject + 40u, "navigation address");
        events.emplace_back("navigation");
        MaybeFail("navigation");
    }
    std::uint8_t IndeterminateListByte() override {
        events.emplace_back("list-byte");
        return 0xa5u;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    std::string ReadLayoutPrefix(std::uint32_t object) override {
        Check(object == kObject, "prefix owner");
        events.emplace_back("prefix");
        MaybeFail("prefix");
        return "Menu/";
    }
    std::uint32_t FindWindow(
        std::uint32_t object,
        const std::string& name) override {
        Check(object == kObject, "window owner");
        names.push_back(name);
        const auto index = names.size();
        const auto step = "find" + std::to_string(index);
        events.push_back(step);
        MaybeFail(step);
        return 0x4000u +
            static_cast<std::uint32_t>(index - 1u) * 0x10u;
    }
    std::uint32_t AllocateCallback(std::uint32_t size) override {
        Check(size == 0x18u, "callback allocation size");
        events.emplace_back("allocate-callback");
        const auto callback = null_callbacks ? 0u :
            0x6000u +
            static_cast<std::uint32_t>(callbacks.size()) * 0x100u;
        callbacks.push_back(callback);
        return callback;
    }
    std::uint32_t Subscribe(std::uint32_t window,
                            const char* event,
                            std::uint32_t callback) override {
        subscribed_windows.push_back(window);
        subscribed_events.emplace_back(event);
        subscribed_callbacks.push_back(callback);
        events.emplace_back("subscribe");
        MaybeFail("subscribe");
        return subscribed_windows.size() == 1u ? 0x7000u :
            0x7100u;
    }
    void DestroyOwner(std::uint32_t owner,
                      std::uint32_t flags) override {
        Check(owner == 0x7000u && flags == 1u,
              "first owner destroyed with delete flag");
        events.emplace_back("destroy-owner");
    }
    pal4::SelectPosition GetPosition(
        std::uint32_t window) override {
        events.emplace_back("position");
        if (window == 0x4020u) return {11u, 22u};
        Check(window == 0x4040u, "answer position window");
        return {33u, 44u};
    }
    void ResetAnswerCollection(
        std::uint32_t collection,
        std::uint32_t storage,
        std::uint32_t mode,
        std::uint32_t body_slot) override {
        Check(collection == kObject + 84u &&
                  storage == 0u && mode == 1u &&
                  body_slot == kObject + 140u,
              "answer collection reset arguments");
        events.emplace_back("reset-answers");
    }
    void RemoveChild(std::uint32_t parent,
                     std::uint32_t child) override {
        Check(parent == 0x5000u && child == 0x4020u,
              "body detachment arguments");
        events.emplace_back("remove-child");
    }
    void CleanupMemoryBlock(std::uint32_t address) override {
        events.push_back("cleanup-" +
                         std::to_string(address - kObject));
    }
    void CleanupNavigation(std::uint32_t address) override {
        Check(address == kObject + 40u, "cleanup navigation");
        events.emplace_back("cleanup-navigation");
    }
    void CleanupBase(std::uint32_t object) override {
        Check(object == kObject, "cleanup base");
        events.emplace_back("cleanup-base");
    }
};

void CheckConstruct() {
    Runtime runtime;
    Check(pal4::SelectDialog::Construct(Runtime::kObject,
                                        runtime) == Runtime::kObject,
          "constructor return");
    Check(runtime.words.at(Runtime::kObject) ==
              pal4::SelectDialog::kVtable &&
              runtime.words.at(Runtime::kObject + 40u) ==
                  pal4::SelectDialog::kNavigationVtable &&
              runtime.words.at(Runtime::kObject + 160u) == 0u &&
              runtime.words.at(Runtime::kObject + 156u) ==
                  0xffffffffu,
          "vtable and trailing fields");
    for (const auto offset : {68u, 84u, 100u})
        Check(runtime.bytes.at(Runtime::kObject + offset) == 0xa5u,
              "indeterminate byte copied to three records");
    for (const auto offset : {64u, 72u, 76u, 80u, 88u,
                              92u, 96u, 104u, 108u, 112u})
        Check(runtime.words.at(Runtime::kObject + offset) == 0u,
              "selection records initialized");
    Check(runtime.names == std::vector<std::string>{
              "Menu/EditQuestion", "Menu/Panelhead",
              "Menu/Panelbody", "Menu/Panelfoot",
              "Menu/StaticAnswer"},
          "five exact layout suffixes");
    for (unsigned index = 0u; index < 5u; ++index)
        Check(runtime.words.at(Runtime::kObject + 132u +
                               index * 4u) ==
                  0x4000u + index * 0x10u,
              "window stored at expected offset");
    Check(runtime.callbacks ==
              std::vector<std::uint32_t>{0x6000u, 0x6100u} &&
              runtime.words.at(0x6000u) ==
                  pal4::SelectDialog::kCallbackVtable &&
              runtime.words.at(0x6000u + 8u) ==
                  pal4::SelectDialog::kKeyCallback &&
              runtime.words.at(0x6100u + 8u) ==
                  pal4::SelectDialog::kClickCallback &&
              runtime.words.at(0x6000u + 12u) == 0u &&
              runtime.words.at(0x6100u + 12u) == 0u &&
              runtime.words.at(0x6000u + 16u) == Runtime::kObject &&
              runtime.words.at(0x6100u + 16u) == Runtime::kObject,
          "callback records match IDA layout");
    Check(runtime.subscribed_windows ==
              std::vector<std::uint32_t>{0x3000u, 0x4040u} &&
              runtime.subscribed_events ==
                  std::vector<std::string>{
                      "EventKeyDown", "EventClicked"} &&
              runtime.subscribed_callbacks == runtime.callbacks &&
              runtime.words.at(0x7000u + 4u) == 0u &&
              runtime.words.at(0x7100u + 4u) == 2u,
          "subscription and owner reference release");
    Check(runtime.words.at(Runtime::kObject + 116u) == 11u &&
              runtime.words.at(Runtime::kObject + 120u) == 22u &&
              runtime.words.at(Runtime::kObject + 124u) == 33u &&
              runtime.words.at(Runtime::kObject + 128u) == 44u &&
              runtime.words.at(Runtime::kObject + 152u) == 0x5000u,
          "saved positions and detached body parent");
    const auto reset = std::find(runtime.events.begin(),
                                 runtime.events.end(),
                                 "reset-answers");
    const auto remove = std::find(runtime.events.begin(),
                                  runtime.events.end(),
                                  "remove-child");
    Check(reset != runtime.events.end() && remove !=
              runtime.events.end() && reset < remove,
          "answer reset precedes body detachment");
}

void CheckNullCallbacks() {
    Runtime runtime;
    runtime.null_callbacks = true;
    (void)pal4::SelectDialog::Construct(Runtime::kObject,
                                        runtime);
    Check(runtime.callbacks ==
              std::vector<std::uint32_t>{0u, 0u} &&
              runtime.subscribed_callbacks == runtime.callbacks,
          "allocation failure still subscribes null callbacks");
}

void CheckFailure(const std::string& step,
                  const std::vector<std::string>& cleanup) {
    Runtime runtime;
    runtime.fail_at = step;
    bool threw = false;
    try {
        (void)pal4::SelectDialog::Construct(Runtime::kObject,
                                            runtime);
    } catch (const std::runtime_error& error) {
        threw = step == error.what();
    }
    Check(threw, "constructor forwards runtime failure");
    Check(runtime.events.size() >= cleanup.size() &&
              std::equal(cleanup.begin(), cleanup.end(),
                         runtime.events.end() - cleanup.size()),
          "constructor unwinds completed stages in reverse");
}

struct LifecycleRuntime final : pal4::ISelectDialogLifecycleRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::vector<std::uint32_t> freed;
    std::vector<std::uint32_t> flagged;
    std::uint32_t result = 0xabcdef01u;

    LifecycleRuntime() {
        words[Runtime::kObject + 104u] = 0x7000u;
        words[Runtime::kObject + 108u] = 0x7008u;
        words[Runtime::kObject + 112u] = 0x7010u;
        words[Runtime::kObject + 88u] = 0x7100u;
        words[Runtime::kObject + 92u] = 0x7108u;
        words[Runtime::kObject + 96u] = 0x7110u;
        words[Runtime::kObject + 72u] = 0x7200u;
        words[Runtime::kObject + 76u] = 0x7230u;
        words[Runtime::kObject + 80u] = 0x7240u;
        words[Runtime::kObject + 48u] = 0x7300u;
        words[Runtime::kObject + 52u] = 0x7308u;
        words[Runtime::kObject + 56u] = 0x7310u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        events.emplace_back("write");
    }
    void CleanupCallbacks(std::uint32_t object) override {
        Check(object == Runtime::kObject,
              "callback cleanup owner");
        events.emplace_back("cleanup-callbacks");
    }
    void SetErrorFlag(std::uint32_t record,
                      std::uint32_t value) override {
        Check(value == 1u, "record cleanup flag");
        flagged.push_back(record);
        events.emplace_back("flag");
    }
    void Free(std::uint32_t address) override {
        freed.push_back(address);
        events.emplace_back("free");
    }
    std::uint32_t DestructBase(std::uint32_t object) override {
        Check(object == Runtime::kObject, "base destructor owner");
        events.emplace_back("base");
        return result;
    }
};

void CheckDestruct() {
    LifecycleRuntime runtime;
    Check(pal4::SelectDialog::Destruct(
              Runtime::kObject, runtime) == runtime.result,
          "destructor returns base result");
    Check(runtime.words.at(Runtime::kObject) ==
              pal4::SelectDialog::kVtable &&
              runtime.words.at(Runtime::kObject + 40u) ==
                  pal4::SelectDialog::kNavigationVtable,
          "destructor restores vtables");
    Check(runtime.freed ==
              std::vector<std::uint32_t>{
                  0x7000u, 0x7100u, 0x7200u, 0x7300u} &&
              runtime.flagged ==
                  std::vector<std::uint32_t>{
                      0x7200u, 0x7210u, 0x7220u},
          "buffers and 16-byte records released in order");
    for (const auto offset : {48u, 52u, 56u, 72u, 76u,
                              80u, 88u, 92u, 96u, 104u,
                              108u, 112u})
        Check(runtime.words.at(Runtime::kObject + offset) == 0u,
              "destructor clears buffer fields");
    Check(runtime.events.front() == "write" &&
              runtime.events[2u] == "cleanup-callbacks" &&
              runtime.events.back() == "base",
          "destructor vtable and base order");

    LifecycleRuntime empty;
    empty.words[Runtime::kObject + 72u] = 0u;
    empty.words[Runtime::kObject + 76u] = 0u;
    (void)pal4::SelectDialog::Destruct(Runtime::kObject, empty);
    Check(empty.flagged.empty() && empty.freed[2u] == 0u,
          "empty record array still frees null pointer");
}

struct OptionRuntime final : pal4::ISelectDialogOptionRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> operations;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> freed;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> copies;
    std::uint32_t next_address = 0x9000u;

    OptionRuntime() {
        words[Runtime::kObject + 72u] = 0u;
        words[Runtime::kObject + 76u] = 0u;
        words[Runtime::kObject + 80u] = 0u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        operations.emplace_back("allocate");
        allocations.push_back(size);
        const auto result = next_address;
        next_address += 0x100u;
        return result;
    }
    void CopyRecord(std::uint32_t destination,
                    std::uint32_t source) override {
        operations.emplace_back("copy-record");
        copies.emplace_back(destination, source);
    }
    void CopyBuffer(std::uint32_t destination,
                    std::uint32_t first,
                    std::uint32_t last) override {
        Check(destination == first && last >= first + 16u,
              "buffer copy span");
        operations.emplace_back("copy-buffer");
    }
    void CopyMultipleBuffers(std::uint32_t destination,
                             std::uint32_t count,
                             std::uint32_t source) override {
        Check(destination != 0u && count == 1u &&
                  source == 0xa000u,
              "one source option copied");
        operations.emplace_back("copy-multiple");
    }
    void RemoveAndReplaceBuffer(
        std::uint32_t destination,
        std::uint32_t end,
        std::uint32_t source) override {
        Check(destination == end && source == 0xa000u,
              "in-place replacement arguments");
        operations.emplace_back("replace");
    }
    void CleanupStrings(std::uint32_t begin,
                        std::uint32_t end) override {
        Check(end >= begin, "cleanup range");
        operations.emplace_back("cleanup");
    }
    void Free(std::uint32_t address) override {
        freed.push_back(address);
        operations.emplace_back("free");
    }
};

void CheckAddButton() {
    OptionRuntime runtime;
    const auto add = [&] {
        return pal4::SelectDialog::AddButton(
            Runtime::kObject, 0xa000u, runtime);
    };
    Check(add() == 0u &&
              runtime.words.at(Runtime::kObject + 72u) == 0x9000u &&
              runtime.words.at(Runtime::kObject + 76u) == 0x9010u &&
              runtime.words.at(Runtime::kObject + 80u) == 0x9010u,
          "first option grows zero capacity to one");
    Check(add() == 0x9000u &&
              runtime.words.at(Runtime::kObject + 72u) == 0x9100u &&
              runtime.words.at(Runtime::kObject + 76u) == 0x9120u &&
              runtime.words.at(Runtime::kObject + 80u) == 0x9120u,
          "second option grows capacity to two");
    Check(add() == 0x9100u &&
              runtime.words.at(Runtime::kObject + 72u) == 0x9200u &&
              runtime.words.at(Runtime::kObject + 76u) == 0x9230u &&
              runtime.words.at(Runtime::kObject + 80u) == 0x9240u,
          "third option doubles capacity to four");
    const auto before = runtime.operations.size();
    Check(add() == 0x9240u &&
              runtime.words.at(Runtime::kObject + 76u) == 0x9240u &&
              runtime.operations.size() == before + 3u &&
              runtime.operations[before] == "copy-buffer" &&
              runtime.operations[before + 1u] ==
                  "copy-multiple" &&
              runtime.operations[before + 2u] == "replace",
          "fourth option uses spare capacity and returns new end");
    Check(add() == 0x9200u &&
              runtime.words.at(Runtime::kObject + 72u) == 0x9300u &&
              runtime.words.at(Runtime::kObject + 76u) == 0x9350u &&
              runtime.words.at(Runtime::kObject + 80u) == 0x9380u,
          "fifth option doubles capacity to eight");
    Check(runtime.allocations ==
              std::vector<std::uint32_t>{16u, 32u, 64u,
                                         128u} &&
              runtime.freed ==
                  std::vector<std::uint32_t>{
                      0u, 0x9000u, 0x9100u, 0x9200u} &&
              runtime.copies.size() == 7u,
          "growth copies old records and frees prior storage");
}

struct TextRuntime final : pal4::ISelectDialogTextRuntime {
    std::vector<std::string> events;
    std::string converted;
    std::u32string received;
    std::uint32_t received_option = 0u;

    std::string ConvertToUtf8(
        const std::string& source) override {
        events.emplace_back("convert");
        Check(source == "source", "text conversion source");
        return converted;
    }
    void InitializeList(std::uint32_t object,
                        const std::u32string& text,
                        std::uint32_t option) override {
        Check(object == Runtime::kObject,
              "initialize list owner");
        events.emplace_back("initialize-list");
        received = text;
        received_option = option;
    }
    std::uint32_t DestroyCeguiString() override {
        events.emplace_back("destroy-string");
        return 0x12345678u;
    }
};

void CheckSetQuestionAndOptions() {
    for (const auto& pair : {
             std::pair<std::string, std::u32string>{"", U""},
             {"Answer", U"Answer"},
             {"\xe4\xb8\xad\xe6\x96\x87", U"\u4e2d\u6587"},
             {"A\xf0\x9f\x98\x80", U"A\U0001f600"},
             {std::string("A\0ignored", 9u), U"A"}}) {
        TextRuntime runtime;
        runtime.converted = pair.first;
        Check(pal4::SelectDialog::SetQuestionAndOptions(
                  Runtime::kObject, "source", 7u,
                  runtime) == 0x12345678u &&
                  runtime.received == pair.second &&
                  runtime.received_option == 7u &&
                  runtime.events ==
                      std::vector<std::string>{
                          "convert", "initialize-list",
                          "destroy-string"},
              "text conversion and list initialization");
    }
}

}  // namespace

int main() {
    try {
        CheckConstruct();
        CheckNullCallbacks();
        CheckFailure("layout", {});
        CheckFailure("navigation", {"cleanup-base"});
        const std::vector<std::string> full_cleanup{
            "cleanup-100", "cleanup-84", "cleanup-68",
            "cleanup-navigation", "cleanup-base"};
        CheckFailure("prefix", full_cleanup);
        CheckFailure("find3", full_cleanup);
        CheckFailure("subscribe", full_cleanup);
        CheckDestruct();
        CheckAddButton();
        CheckSetQuestionAndOptions();
        std::cout << "select_dialog_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
