#include "pal4/smith_window.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISmithWindowInventoryRefreshRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {object + 0xbcu, 0x8000u},
        {object + 0xc0u, 0x8010u},
        {0x8000u, 11u}, {0x8004u, 22u},
        {0x8008u, 33u}, {0x800cu, 44u},
    };
    std::vector<std::string> calls;
    std::vector<std::uint32_t> looked_up;
    std::uint32_t end_at_collect = 0u;
    std::vector<std::uint32_t> values_at_collect;
    std::int32_t hover_id = 77;
    std::uint32_t missing_id = 22u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write");
    }
    std::int32_t PrescriptionHoverItemId(
        std::uint32_t owner) override {
        Check(owner == object, "hover owner");
        calls.emplace_back("hover");
        return hover_id;
    }
    void TouchSmithRecord(std::int32_t id) override {
        Check(id == hover_id, "Smith record ID");
        calls.emplace_back("touch");
    }
    bool HasInventoryItem(std::uint32_t id,
                          std::uint32_t category) override {
        Check(category == 7u, "inventory category");
        looked_up.push_back(id);
        calls.emplace_back("has-item");
        return id != missing_id;
    }
    void CollectMatchingInventoryItemIds(
        std::uint32_t owner, std::int32_t id) override {
        Check(owner == object && id == hover_id,
              "collect arguments");
        end_at_collect = words.at(object + 0xc0u);
        for (auto cursor = words.at(object + 0xbcu);
             cursor != end_at_collect; cursor += 4u)
            values_at_collect.push_back(words.at(cursor));
        calls.emplace_back("collect");
    }
    void RefreshCollectedEquipmentList(
        std::uint32_t owner) override {
        Check(owner == object, "refresh owner");
        calls.emplace_back("refresh");
    }
};

void CheckFirstMissingIsRemoved() {
    Runtime runtime;
    pal4::SmithWindow::RefreshAfterInventoryChange(
        Runtime::object, runtime);
    Check(runtime.looked_up == std::vector<std::uint32_t>{11u, 22u} &&
              runtime.end_at_collect == 0x800cu &&
              runtime.values_at_collect ==
                  std::vector<std::uint32_t>{11u, 33u, 44u},
          "first missing record shifts the remaining IDs");
    Check(runtime.calls.front() == "hover" &&
              runtime.calls.at(1) == "touch" &&
              runtime.calls.at(runtime.calls.size() - 2u) == "collect" &&
              runtime.calls.back() == "refresh",
          "Smith record and refresh call order");
}

void CheckOtherPositionsAndEmpty() {
    Runtime no_missing;
    no_missing.missing_id = 99u;
    pal4::SmithWindow::RefreshAfterInventoryChange(
        Runtime::object, no_missing);
    Check(no_missing.looked_up.size() == 4u &&
              no_missing.end_at_collect == 0x8010u &&
              no_missing.values_at_collect ==
                  std::vector<std::uint32_t>{11u, 22u, 33u, 44u},
          "all present records are retained");

    Runtime last_missing;
    last_missing.missing_id = 44u;
    pal4::SmithWindow::RefreshAfterInventoryChange(
        Runtime::object, last_missing);
    Check(last_missing.looked_up.size() == 4u &&
              last_missing.end_at_collect == 0x800cu &&
              last_missing.values_at_collect ==
                  std::vector<std::uint32_t>{11u, 22u, 33u},
          "last missing record only shortens the array");

    Runtime empty;
    empty.words[Runtime::object + 0xc0u] = 0x8000u;
    pal4::SmithWindow::RefreshAfterInventoryChange(
        Runtime::object, empty);
    Check(empty.looked_up.empty() &&
              empty.values_at_collect.empty() &&
              empty.calls.back() == "refresh",
          "empty array still recollects and refreshes");
}

}  // namespace

int main() {
    try {
        CheckFirstMissingIsRemoved();
        CheckOtherPositionsAndEmpty();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
