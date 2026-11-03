#include "pal4/listbox_spinner_item.h"

#include <cstdint>
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

struct Runtime final : pal4::IListboxSpinnerItemRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::string global_text = "";
    std::string base_text;
    std::uint32_t created_spinner = 0x2000u;
    std::int32_t created_value = 0;
    std::uint32_t removed_spinner = 0u;
    std::uint32_t reset_spinner = 0u;
    std::uint32_t virtual_spinner = 0u;
    std::uint32_t virtual_slot = 0u;
    std::uint32_t updated_spinner = 0u;
    std::int32_t updated_value = 0;

    std::string_view ReadCString(std::uint32_t address) override {
        Check(address == 0x8dc780u, "empty text global");
        calls.emplace_back("read_text");
        return global_text;
    }
    void ConstructBase(std::uint32_t item,
                       std::string_view text) override {
        Check(item == 0x1000u, "base item");
        base_text = text;
        calls.emplace_back("base");
    }
    std::uint32_t CreateOrGetSpinner(std::int32_t value) override {
        created_value = value;
        calls.emplace_back("create");
        return created_spinner;
    }
    void ResetSpinner(std::uint32_t spinner) override {
        reset_spinner = spinner;
        calls.emplace_back("reset");
    }
    void RemoveSpinner(std::uint32_t spinner) override {
        removed_spinner = spinner;
        calls.emplace_back("remove");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write:" + std::to_string(address));
    }
    std::uint32_t BaseItemVtable() override { return 0x8412f4u; }
    void DestroyString(std::uint32_t address) override {
        calls.emplace_back("string:" + std::to_string(address));
    }
    std::uint32_t CallVirtual(std::uint32_t spinner,
                              std::uint32_t slot) override {
        virtual_spinner = spinner;
        virtual_slot = slot;
        calls.emplace_back("virtual");
        return 0xabcdu + slot;
    }
    void UpdateSpinnerValue(std::uint32_t spinner,
                            std::int32_t value) override {
        updated_spinner = spinner;
        updated_value = value;
        calls.emplace_back("update");
    }
};

std::size_t Find(const std::vector<std::string>& calls,
                 const std::string& target) {
    for (std::size_t i = 0; i < calls.size(); ++i)
        if (calls[i] == target) return i;
    throw std::runtime_error("missing call: " + target);
}

void CheckLifecycle() {
    for (const auto value : {0, 42, -42}) {
        Runtime runtime;
        runtime.global_text = "";
        Check(pal4::ListboxSpinnerItem::Construct(
                  0x1000u, value, runtime) == 0x1000u &&
                  runtime.base_text.empty() &&
                  runtime.created_value == value &&
                  runtime.reset_spinner == 0x2000u &&
                  runtime.words[0x1000u] ==
                      pal4::ListboxSpinnerItem::kVtable &&
                  runtime.words[0x1000u + 424u] == 0x2000u &&
                  runtime.words[0x1000u + 428u] == 0u &&
                  runtime.words[0x1000u + 432u] == 0u &&
                  Find(runtime.calls, "base") <
                      Find(runtime.calls, "create") &&
                  Find(runtime.calls, "create") <
                      Find(runtime.calls, "reset"),
              "constructor layout and ownership order");
        runtime.calls.clear();
        pal4::ListboxSpinnerItem::Destruct(0x1000u, runtime);
        Check(runtime.removed_spinner == 0x2000u &&
                  runtime.words[0x1000u] == 0x8412f4u &&
                  Find(runtime.calls, "remove") <
                      Find(runtime.calls, "string:4252") &&
                  Find(runtime.calls, "string:4252") <
                      Find(runtime.calls, "string:4100"),
              "destructor returns spinner and destroys base strings");
    }
}

void CheckForwarding() {
    Runtime runtime;
    runtime.words[0x1000u + 424u] = 0x2000u;
    runtime.words[0x1000u + 308u] = 7u;
    runtime.words[0x1000u + 316u] = 0x3000u;
    runtime.words[0x2000u + 440u] =
        static_cast<std::uint32_t>(-3);
    Check(pal4::ListboxSpinnerItem::PrepareAndCallUpdate(
              0x1000u, runtime) == 0xabd9u &&
              runtime.words[0x2000u + 308u] == 7u &&
              runtime.words[0x2000u + 316u] == 0x3000u &&
              runtime.virtual_spinner == 0x2000u &&
              runtime.virtual_slot == 12u &&
              Find(runtime.calls, "write:8500") <
                  Find(runtime.calls, "write:8508") &&
              Find(runtime.calls, "write:8508") <
                  Find(runtime.calls, "virtual"),
          "prepare row/list and call virtual slot 12");
    Check(pal4::ListboxSpinnerItem::CallUpdate(0x1000u, runtime) ==
              0xabe1u && runtime.virtual_slot == 20u,
          "call virtual slot 20");
    Check(pal4::ListboxSpinnerItem::GetCurrentValue(
              0x1000u, runtime) == -3,
          "signed current value");
    pal4::ListboxSpinnerItem::SetCurrentValue(0x1000u, 14, runtime);
    Check(runtime.updated_spinner == 0x2000u &&
              runtime.updated_value == 14,
          "value setter forwards to spinner");
}

void CheckRangeAndCallback() {
    Runtime runtime;
    runtime.words[0x1000u + 424u] = 0x2000u;
    runtime.words[0x2000u + 440u] = 5u;
    pal4::ListboxSpinnerItem::SetRange(0x1000u, -4, 10,
                                        runtime);
    Check(runtime.words[0x2000u + 472u] == 10u &&
              runtime.words[0x2000u + 468u] ==
                  static_cast<std::uint32_t>(-4) &&
              runtime.updated_value == 5 &&
              Find(runtime.calls, "write:8664") <
                  Find(runtime.calls, "read:8632") &&
              Find(runtime.calls, "read:8632") <
                  Find(runtime.calls, "write:8660") &&
              Find(runtime.calls, "write:8660") <
                  Find(runtime.calls, "update"),
          "range writes maximum, reads current, writes minimum and refreshes");
    runtime.calls.clear();
    Check(pal4::ListboxSpinnerItem::SetChangeCallback(
              0x1000u, 0x1234u, 0x5678u, runtime) == 0x1000u + 428u &&
              runtime.words[0x1000u + 428u] == 0x1234u &&
              runtime.words[0x1000u + 432u] == 0x5678u &&
              runtime.words[0x2000u + 476u] == 0x1000u + 428u &&
              Find(runtime.calls, "write:4528") <
                  Find(runtime.calls, "write:4524") &&
              Find(runtime.calls, "write:4524") <
                  Find(runtime.calls, "write:8668"),
          "callback pair and spinner pointer layout");
}

}  // namespace

int main() {
    try {
        CheckLifecycle();
        CheckForwarding();
        CheckRangeAndCallback();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
