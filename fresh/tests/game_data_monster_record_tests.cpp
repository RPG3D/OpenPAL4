#include "pal4/game_data_monster_record.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataMonsterRecordRuntime {
    std::vector<std::uint32_t> constructed;
    std::vector<std::uint32_t> cleared;
    std::vector<std::string> calls;

    void ConstructText(std::uint32_t address) override {
        constructed.push_back(address);
        calls.emplace_back("text");
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        Check(value == 0, "record reset only writes zero");
        cleared.push_back(address);
        calls.emplace_back("zero");
    }
};

void CheckInitialize() {
    Runtime runtime;
    const auto base = 0x5000u;
    Check(pal4::GameDataMonsterRecord::Initialize(base, runtime) == base,
          "record constructor returns its object address");
    Check(runtime.constructed.size() == 11 && runtime.calls.size() == 16,
          "constructor builds eleven texts and clears five words");
    for (std::uint32_t i = 0; i < 11; ++i) {
        const auto expected = i < 6 ? base + 72 + i * 16
                                    : base + 168 + (i - 6) * 16;
        Check(runtime.constructed[i] == expected &&
                  runtime.calls[i] == "text",
              "text objects are constructed at exact offsets in IDA order");
    }
    Check(runtime.cleared == std::vector<std::uint32_t>{
                                 base + 64, base + 248, base + 20,
                                 base + 24, base + 28},
          "scalar and array-pointer words reset in IDA order");
    for (std::size_t i = 11; i < runtime.calls.size(); ++i)
        Check(runtime.calls[i] == "zero",
              "scalar reset follows both text arrays");
}

void CheckResetOnly() {
    Runtime runtime;
    pal4::GameDataMonsterRecord::ResetScalarAndArrayFields(0x6000, runtime);
    Check(runtime.constructed.empty() &&
              runtime.cleared == std::vector<std::uint32_t>{
                                     0x6040, 0x60f8, 0x6014, 0x6018,
                                     0x601c},
          "scalar helper does not construct text and uses exact addresses");
}

void CheckSubrecordDestruction() {
    struct DestroyRuntime final : pal4::IGameDataMonsterRecordDestroyRuntime {
        std::vector<std::uint32_t> destroyed;
        void DestroyText(std::uint32_t address) override {
            destroyed.push_back(address);
        }
    } runtime;
    pal4::GameDataMonsterRecord::DestroySubrecord(0x7000, runtime);
    Check(runtime.destroyed.size() == 11,
          "subrecord destructor visits all eleven text members");
    for (std::uint32_t i = 0; i < 5; ++i)
        Check(runtime.destroyed[i] == 0x7000 + 168 + (4 - i) * 16,
              "five trailing texts destruct in reverse order");
    for (std::uint32_t i = 0; i < 6; ++i)
        Check(runtime.destroyed[5 + i] == 0x7000 + 72 + (5 - i) * 16,
              "six leading texts destruct in reverse order");

    runtime.destroyed.clear();
    pal4::GameDataMonsterRecord::DestroyFullRecord(0x8000, runtime);
    Check(runtime.destroyed.size() == 11 &&
              runtime.destroyed.front() == 0x8000 + 336 + 232 &&
              runtime.destroyed.back() == 0x8000 + 336 + 72,
          "full 596-byte record destructor forwards to subobject +336");
}

}  // namespace

int main() {
    try {
        CheckInitialize();
        CheckResetOnly();
        CheckSubrecordDestruction();
        std::cout << "game_data_monster_record_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
