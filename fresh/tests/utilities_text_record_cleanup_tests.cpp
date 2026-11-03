#include "pal4/utilities_text_record_cleanup.h"

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

struct Runtime final : pal4::IUtilitiesTextRecordRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> freed;
    std::vector<std::uint32_t> released_order;
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        released_order.push_back(address);
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t Free(std::uint32_t address) override {
        freed.push_back(address);
        return address + 0x10000;
    }
    void Set(std::uint32_t offset, std::uint32_t text,
             std::uint8_t references) {
        words[0x1000 + offset] = text;
        words[0x1000 + offset + 4] = text + 4;
        words[0x1000 + offset + 8] = text + 8;
        bytes[text - 1] = references;
    }
    void CheckCleared(std::uint32_t offset) {
        for (std::uint32_t part = 0; part < 12; part += 4)
            Check(words.at(0x1000 + offset + part) == 0,
                  "all three text-triplet words cleared");
    }
};

void CheckTwoFields() {
    Runtime first;
    first.Set(24, 0x5001, 2);
    first.Set(8, 0x6001, 0);
    Check(pal4::UtilitiesTextRecordCleanup::Release8And24(
              0x1000, first) == 0x16000 &&
              first.released_order ==
                  std::vector<std::uint32_t>{0x5000, 0x6000} &&
              first.bytes.at(0x5000) == 1 &&
              first.freed == std::vector<std::uint32_t>{0x6000},
          "24 then 8: decrement live reference and free zero prefix");
    first.CheckCleared(24);
    first.CheckCleared(8);

    Runtime shifted;
    shifted.Set(28, 0x7001, 0xff);
    shifted.Set(12, 0x8001, 1);
    Check(pal4::UtilitiesTextRecordCleanup::Release12And28(
              0x1000, shifted) == 0x8000 &&
              shifted.released_order ==
                  std::vector<std::uint32_t>{0x7000, 0x8000} &&
              shifted.freed == std::vector<std::uint32_t>{0x7000} &&
              shifted.bytes.at(0x8000) == 0,
          "28 then 12: sentinel frees, final live prefix returned");
    shifted.CheckCleared(28);
    shifted.CheckCleared(12);

    Runtime mixed;
    mixed.Set(28, 0x9001, 3);
    mixed.Set(8, 0xa001, 0xff);
    Check(pal4::UtilitiesTextRecordCleanup::Release8And28(
              0x1000, mixed) == 0x1a000 &&
              mixed.released_order ==
                  std::vector<std::uint32_t>{0x9000, 0xa000},
          "28 then 8 layout and final free result");
    mixed.CheckCleared(28);
    mixed.CheckCleared(8);
}

void CheckThreeFieldsAndNull() {
    Runtime runtime;
    runtime.Set(40, 0x5001, 0);
    runtime.Set(24, 0x6001, 0xff);
    runtime.Set(8, 0x7001, 4);
    Check(pal4::UtilitiesTextRecordCleanup::Release8And24And40(
              0x1000, runtime) == 0x7000 &&
              runtime.released_order ==
                  std::vector<std::uint32_t>{0x5000, 0x6000,
                                             0x7000} &&
              runtime.freed ==
                  std::vector<std::uint32_t>{0x5000, 0x6000} &&
              runtime.bytes.at(0x7000) == 3,
          "three text fields release in descending offset order");
    runtime.CheckCleared(40);
    runtime.CheckCleared(24);
    runtime.CheckCleared(8);

    Runtime empty;
    Check(pal4::UtilitiesTextRecordCleanup::Release8And24And40(
              0x1000, empty) == 0 &&
              empty.released_order.empty() && empty.freed.empty(),
          "null text fields need no reference or free calls");
    empty.CheckCleared(40);
    empty.CheckCleared(24);
    empty.CheckCleared(8);
}

}  // namespace

int main() {
    try {
        CheckTwoFields();
        CheckThreeFieldsAndNull();
        std::cout << "utilities_text_record_cleanup_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
