#include "pal4/game_data_shop_goods_record_text.h"

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

struct Runtime final : pal4::IGameDataShopGoodsRecordTextRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<Call> calls;
    std::uint32_t free_result = 0x12345678;
    bool fail_free = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back({"read-byte", address});
        return bytes[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back({"write-word", address, value});
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back({"write-byte", address, value});
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.push_back({"free", address});
        if (fail_free) throw std::runtime_error("free failed");
        return free_result;
    }
};

constexpr std::uint32_t kRecord = 0x9000;

void CheckReleaseBranches() {
    for (std::uint8_t references : {std::uint8_t{0}, std::uint8_t{0xff}}) {
        Runtime runtime;
        runtime.words[kRecord + 24] = 0x5001;
        runtime.words[kRecord + 28] = 7;
        runtime.words[kRecord + 32] = 8;
        runtime.bytes[0x5000] = references;
        Check(pal4::GameDataShopGoodsRecordText::Release(
                  kRecord, runtime) == runtime.free_result &&
                  runtime.calls == std::vector<Call>{
                      {"read-byte", 0x5000}, {"free", 0x5000},
                      {"write-word", kRecord + 24, 0},
                      {"write-word", kRecord + 28, 0},
                      {"write-word", kRecord + 32, 0}},
              "zero and sentinel refcounts free header before reset");
    }
    Runtime shared;
    shared.words[kRecord + 24] = 0x5001;
    shared.bytes[0x5000] = 3;
    Check(pal4::GameDataShopGoodsRecordText::Release(
              kRecord, shared) == 0x5000 &&
              shared.bytes[0x5000] == 2 &&
              shared.calls == std::vector<Call>{
                  {"read-byte", 0x5000},
                  {"write-byte", 0x5000, 2},
                  {"write-word", kRecord + 24, 0},
                  {"write-word", kRecord + 28, 0},
                  {"write-word", kRecord + 32, 0}},
          "shared buffer decrements byte and returns header address");
}

void CheckEmptyAndFailure() {
    Runtime empty;
    empty.words[kRecord + 28] = 7;
    empty.words[kRecord + 32] = 8;
    Check(pal4::GameDataShopGoodsRecordText::Release(
              kRecord, empty) == 0 &&
              empty.calls == std::vector<Call>{
                  {"write-word", kRecord + 24, 0},
                  {"write-word", kRecord + 28, 0},
                  {"write-word", kRecord + 32, 0}},
          "null data skips reference access and clears metadata");

    Runtime interrupted;
    interrupted.words[kRecord + 24] = 0x5001;
    interrupted.bytes[0x5000] = 0;
    interrupted.fail_free = true;
    try {
        static_cast<void>(pal4::GameDataShopGoodsRecordText::Release(
            kRecord, interrupted));
        Check(false, "free failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "free failed" &&
                  interrupted.calls == std::vector<Call>{
                      {"read-byte", 0x5000}, {"free", 0x5000}} &&
                  interrupted.words[kRecord + 24] == 0x5001,
              "failed free leaves record metadata intact");
    }
}

}  // namespace

int main() {
    try {
        CheckReleaseBranches();
        CheckEmptyAndFailure();
        std::cout << "game_data_shop_goods_record_text_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
