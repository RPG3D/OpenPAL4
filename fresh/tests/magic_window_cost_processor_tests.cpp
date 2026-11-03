#include "pal4/magic_window_cost_processor.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::MagicWindowCostRuntimeStub {
    std::vector<std::string> calls;

    void AdjustCounter(std::int32_t delta) override {
        calls.push_back("counter:" + std::to_string(delta));
    }
    void RemoveItem(std::uint32_t id, std::int32_t count) override {
        calls.push_back("item:" + std::to_string(id) + ":" +
                        std::to_string(count));
    }
};

void TestPositiveCostsAndOrder() {
    pal4::MagicDbStuntRecord record;
    record.counter_cost = 7;
    record.item_ids = {10u, 0u, 20u, 0xffffffffu, 30u};
    record.item_counts = {2, 99, 0, 8, -3};
    Runtime runtime;
    pal4::MagicWindowCostProcessor::Apply(record, runtime);
    Check(runtime.calls == std::vector<std::string>{
                               "counter:-7", "item:10:2", "item:20:0",
                               "item:30:-3"},
          "counter first, five signed item IDs, original counts");
}

void TestNonpositiveCounterAndEmptyItems() {
    pal4::MagicDbStuntRecord record;
    Runtime runtime;
    pal4::MagicWindowCostProcessor::Apply(record, runtime);
    Check(runtime.calls.empty(), "zero costs perform no calls");
    record.counter_cost = -1;
    record.item_ids[4] = 4u;
    record.item_counts[4] = 1;
    pal4::MagicWindowCostProcessor::Apply(record, runtime);
    Check(runtime.calls == std::vector<std::string>{"item:4:1"},
          "negative counter is skipped while final item is processed");
}

}  // namespace

int main() {
    TestPositiveCostsAndOrder();
    TestNonpositiveCounterAndEmptyItems();
}
