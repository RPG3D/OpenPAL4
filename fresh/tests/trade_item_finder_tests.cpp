#include "pal4/trade_item_finder.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Row {
    std::uint32_t id;
    std::string text;
};

struct Call {
    std::string_view operation;
    std::uint32_t address = 0u;
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::ITradeItemFinderRuntime {
    std::string query = "potion";
    std::vector<Row> rows;
    std::vector<Call> calls;

    std::uint32_t GameDatabaseInstance() override {
        calls.push_back({"database"});
        return 0x5000u;
    }
    std::string_view ReadText(const std::uint32_t address) override {
        calls.push_back({"text", address});
        if (address == 0x1000u) return query;
        Check(address >= 0x3014u &&
                  (address - 0x3014u) % 36u == 0u,
              "trade record text offset and stride");
        return rows.at((address - 0x3014u) / 36u).text;
    }
    std::uint32_t TradeData(const std::uint32_t database_table) override {
        Check(database_table == 0x506cu,
              "shop table lies at GameDB offset 108");
        calls.push_back({"trade-data", database_table});
        return 0x2000u;
    }
    std::uint32_t RecordCount(const std::uint32_t count_field) override {
        Check(count_field == 0x205cu, "trade record count offset");
        calls.push_back({"count", count_field});
        return static_cast<std::uint32_t>(rows.size());
    }
    std::uint32_t RecordBase(const std::uint32_t base_field) override {
        Check(base_field == 0x2060u, "trade record base offset");
        calls.push_back({"base", base_field});
        return 0x3000u;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.push_back({"id", address});
        Check(address >= 0x3004u &&
                  (address - 0x3004u) % 36u == 0u,
              "trade record ID offset and stride");
        return rows.at((address - 0x3004u) / 36u).id;
    }
};

void FindsIdAmongEqualText() {
    Runtime runtime;
    runtime.rows = {{10u, "potion"}, {20u, "other"},
                    {30u, "potion"}, {40u, "potion"}};
    Check(pal4::TradeItemFinder::FindByTextAndId(
              0x1000u, 30u, runtime) == 1u,
          "matching text can select a later matching ID");
    Check(runtime.calls == std::vector<Call>{
              {"database"}, {"text", 0x1000u}, {"database"},
              {"trade-data", 0x506cu}, {"count", 0x205cu},
              {"base", 0x2060u},
              {"text", 0x3014u}, {"text", 0x3038u},
              {"text", 0x305cu}, {"text", 0x3080u},
              {"id", 0x3004u}, {"id", 0x304cu}},
          "finder scans all text records before checking matching IDs");
}

void RejectsWrongIdAndText() {
    Runtime wrong_id;
    wrong_id.rows = {{10u, "potion"}, {20u, "other"},
                     {30u, "potion"}};
    Check(pal4::TradeItemFinder::FindByTextAndId(
              0x1000u, 20u, wrong_id) == 0u,
          "an ID on a differently named record does not match");
    Check(wrong_id.calls.back() == Call{"id", 0x304cu},
          "finder searches all text matches before returning false");

    Runtime wrong_case;
    wrong_case.query = "Potion";
    wrong_case.rows = {{10u, "potion"}};
    Check(pal4::TradeItemFinder::FindByTextAndId(
              0x1000u, 10u, wrong_case) == 0u &&
              wrong_case.calls.back() == Call{"text", 0x3014u},
          "text comparison is case-sensitive and skips ID lookup on mismatch");

    Runtime empty;
    Check(pal4::TradeItemFinder::FindByTextAndId(
              0x1000u, 10u, empty) == 0u &&
              empty.calls.back() == Call{"base", 0x2060u},
          "empty shop table returns false without reading records");
}

}  // namespace

int main() {
    try {
        FindsIdAmongEqualText();
        RejectsWrongIdAndText();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
