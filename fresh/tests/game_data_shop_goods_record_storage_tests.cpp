#include "pal4/game_data_shop_goods_record_storage.h"

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

struct Runtime final : pal4::IGameDataShopGoodsRecordStorageRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<Call> calls;
    std::uint32_t next_result = 0x20;
    bool fail_destroy = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back({"write", address, value});
    }
    std::uint32_t DestroyRecord(std::uint32_t address) override {
        calls.push_back({"destroy", address});
        if (fail_destroy) throw std::runtime_error("destroy failure");
        return next_result++;
    }
    std::uint32_t ClearEffectData(std::uint32_t object) override {
        calls.push_back({"effect", object});
        return 0x11;
    }
    void Free(std::uint32_t address) override {
        calls.push_back({"free", address});
    }
};

void CheckArray() {
    constexpr std::uint32_t array = 0x9000;
    Runtime runtime;
    runtime.words[array + 4] = 0x5000;
    runtime.words[array + 8] = 0x5000 + 2 * 36;
    runtime.words[array + 12] = 0x6000;
    Check(pal4::GameDataShopGoodsRecordStorage::ClearAndFree(
              array, runtime) == 0 &&
              runtime.calls == std::vector<Call>{
                  {"destroy", 0x5000}, {"destroy", 0x5000 + 36},
                  {"free", 0x5000}, {"write", array + 4, 0},
                  {"write", array + 8, 0},
                  {"write", array + 12, 0}},
          "Good array destroys 36-byte records before releasing storage");
    Runtime empty;
    Check(pal4::GameDataShopGoodsRecordStorage::ClearAndFree(
              array, empty) == 0 &&
              empty.calls == std::vector<Call>{
                  {"free", 0}, {"write", array + 4, 0},
                  {"write", array + 8, 0},
                  {"write", array + 12, 0}},
          "empty Good array forwards null to free");
}

void CheckObject() {
    constexpr std::uint32_t object = 0x9000;
    Runtime runtime;
    runtime.words[object + 96] = 0x5000;
    runtime.words[object + 100] = 0x5000 + 2 * 36;
    runtime.words[object + 104] = 0x6000;
    Check(pal4::GameDataShopGoodsRecordStorage::ClearObjectRecords(
              object, runtime) == 0x21 &&
              runtime.calls == std::vector<Call>{
                  {"effect", object}, {"destroy", 0x5000},
                  {"destroy", 0x5000 + 36},
                  {"write", object + 100, 0x5000}} &&
              runtime.words[object + 104] == 0x6000,
          "object cleanup retains capacity and last destructor result");
    Runtime empty;
    Check(pal4::GameDataShopGoodsRecordStorage::ClearObjectRecords(
              object, empty) == 0x11 &&
              empty.calls == std::vector<Call>{
                  {"effect", object}, {"write", object + 100, 0}},
          "empty object forwards Effect cleanup result");

    Runtime interrupted;
    interrupted.words[object + 96] = 0x5000;
    interrupted.words[object + 100] = 0x5000 + 36;
    interrupted.fail_destroy = true;
    try {
        static_cast<void>(pal4::GameDataShopGoodsRecordStorage::
                              ClearObjectRecords(object, interrupted));
        Check(false, "record failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "destroy failure" &&
                  interrupted.calls == std::vector<Call>{
                      {"effect", object}, {"destroy", 0x5000}} &&
                  interrupted.words[object + 100] == 0x5000 + 36,
              "record failure stops before active-end reset");
    }
}

void CheckDestroyRange() {
    Runtime runtime;
    Check(pal4::GameDataShopGoodsRecordStorage::DestroyRange(
              0x5000, 0x5000 + 3 * 36, 0xabcdef, runtime) == 0x22 &&
              runtime.calls == std::vector<Call>{
                  {"destroy", 0x5000},
                  {"destroy", 0x5000 + 36},
                  {"destroy", 0x5000 + 72}},
          "standalone Good range destroys 36-byte records in order");
    runtime.calls.clear();
    Check(pal4::GameDataShopGoodsRecordStorage::DestroyRange(
              0x5000, 0x5000, 0xabcdef, runtime) == 0xabcdef &&
              runtime.calls.empty(),
          "empty Good range preserves incoming EAX residue");
}

}  // namespace

int main() {
    try {
        CheckArray();
        CheckObject();
        CheckDestroyRange();
        std::cout << "game_data_shop_goods_record_storage_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
