#include "pal4/combat_ending_task_factories.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatEndingTaskFactoryRuntime {
    struct Write {
        std::uint32_t address;
        std::uint32_t value;
    };
    std::vector<Write> writes;
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        writes.push_back({address, value});
    }
};

void AllEightBindings() {
    Runtime runtime;
    constexpr auto bindings = pal4::CombatEndingTaskFactories::kBindings;
    Check(bindings.size() == 8u, "eight factory entries");
    for (std::uint32_t i = 0u; i < bindings.size(); ++i) {
        const auto& binding = bindings[i];
        Check(binding.entry == 0x547b50u + 0x10u * i &&
                  binding.vtable == 0x8453acu + 4u * i,
              "IDA entry and vtable order");
        const auto factory = 0x10000u + 4u * i;
        Check(pal4::CombatEndingTaskFactories::Bind(
                  binding.type, factory, runtime) == factory &&
                  runtime.writes.size() == i + 1u &&
                  runtime.writes.back().address == factory &&
                  runtime.writes.back().value == binding.vtable,
              "factory writes one vtable and returns its object");
    }
}

void UnknownTypeIsNotWritten() {
    Runtime runtime;
    bool failed = false;
    try {
        (void)pal4::CombatEndingTaskFactories::Bind(
            static_cast<pal4::CombatEndingTaskType>(99),
            0x1234u, runtime);
    } catch (const std::invalid_argument&) {
        failed = true;
    }
    Check(failed && runtime.writes.empty(),
          "invalid type has no IDA entry");
}

}  // namespace

int main() {
    try {
        AllEightBindings();
        UnknownTypeIsNotWritten();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
