#include "pal4/game_data_shop_record_storage.h"

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

struct Call {
    std::string name;
    std::uint32_t address;
    std::uint32_t value = 0;
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::IGameDataShopRecordStorageRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<Call> calls;
    std::uint32_t effect_result = 0x11;
    std::uint32_t next_destroy_result = 0x20;
    std::uint32_t fail_on_destroy = 0;
    std::uint32_t destroy_calls = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back({"write", address, value});
    }
    std::uint32_t DestroyRecord(std::uint32_t record) override {
        calls.push_back({"destroy", record});
        if (++destroy_calls == fail_on_destroy)
            throw std::runtime_error("record destroy failed");
        return next_destroy_result++;
    }
    std::uint32_t ClearEffectData(std::uint32_t object) override {
        calls.push_back({"effect", object});
        return effect_result;
    }
    void Free(std::uint32_t address) override {
        calls.push_back({"free", address});
    }
};

void CheckClearAndFree() {
    Runtime runtime;
    constexpr std::uint32_t array = 0x9000;
    runtime.words[array + 4] = 0x5000;
    runtime.words[array + 8] = 0x5000 + 2 * 56;
    runtime.words[array + 12] = 0x6000;
    Check(pal4::GameDataShopRecordStorage::ClearAndFree(array, runtime) == 0 &&
              runtime.calls == std::vector<Call>{
                  {"destroy", 0x5000}, {"destroy", 0x5000 + 56},
                  {"free", 0x5000}, {"write", array + 4, 0},
                  {"write", array + 8, 0}, {"write", array + 12, 0}},
          "array clear destroys 56-byte records before freeing and resetting");
    Runtime empty;
    Check(pal4::GameDataShopRecordStorage::ClearAndFree(array, empty) == 0 &&
              empty.calls.front() == Call{"free", 0} &&
              empty.destroy_calls == 0,
          "empty array still forwards null to native free");
    Runtime interrupted;
    interrupted.words[array + 4] = 0x5000;
    interrupted.words[array + 8] = 0x5000 + 56;
    interrupted.fail_on_destroy = 1;
    try {
        static_cast<void>(
            pal4::GameDataShopRecordStorage::ClearAndFree(array,
                                                          interrupted));
        Check(false, "array record failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "record destroy failed" &&
                  interrupted.calls == std::vector<Call>{
                      {"destroy", 0x5000}} &&
                  interrupted.words[array + 4] == 0x5000,
              "array failure stops before storage release");
    }
}

void CheckObjectClear() {
    Runtime runtime;
    constexpr std::uint32_t object = 0x9000;
    runtime.words[object + 96] = 0x5000;
    runtime.words[object + 100] = 0x5000 + 2 * 56;
    Check(pal4::GameDataShopRecordStorage::ClearObjectRecords(
              object, runtime) == 0x21 &&
              runtime.calls == std::vector<Call>{
                  {"effect", object}, {"destroy", 0x5000},
                  {"destroy", 0x5000 + 56},
                  {"write", object + 100, 0x5000}} &&
              runtime.words[object + 96] == 0x5000,
          "object clear forwards last destructor result and retains capacity");
    Runtime empty;
    Check(pal4::GameDataShopRecordStorage::ClearObjectRecords(
              object, empty) == empty.effect_result &&
              empty.calls == std::vector<Call>{
                  {"effect", object}, {"write", object + 100, 0}},
          "empty object clear returns Effect cleanup result");
}

void CheckDestroyRange() {
    Runtime runtime;
    Check(pal4::GameDataShopRecordStorage::DestroyRange(
              0x5000, 0x5000 + 3 * 56, 0xabcdef, runtime) == 0x22 &&
              runtime.calls == std::vector<Call>{
                  {"destroy", 0x5000},
                  {"destroy", 0x5000 + 56},
                  {"destroy", 0x5000 + 112}},
          "range cleanup returns final destructor residue");
    runtime.calls.clear();
    Check(pal4::GameDataShopRecordStorage::DestroyRange(
              0x5000, 0x5000, 0xabcdef, runtime) == 0xabcdef &&
              runtime.calls.empty(),
          "empty range preserves incoming EAX residue");
}

}  // namespace

int main() {
    try {
        CheckClearAndFree();
        CheckObjectClear();
        CheckDestroyRange();
        std::cout << "game_data_shop_record_storage_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
