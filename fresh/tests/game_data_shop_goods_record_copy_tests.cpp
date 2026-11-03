#include "pal4/game_data_shop_goods_record_copy.h"

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
    std::string kind;
    std::uint32_t destination;
    std::uint32_t value;
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::IGameDataShopGoodsRecordCopyRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<Call> calls;
    unsigned text_calls = 0;
    unsigned fail_text = 0;
    bool fail_compare = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back({"word", address, value});
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back({"byte", address, value});
    }
    void AssignText(std::uint32_t destination,
                    std::uint32_t source) override {
        calls.push_back({"text", destination, source});
        if (++text_calls == fail_text)
            throw std::runtime_error("text failed");
        for (std::uint32_t offset = 0; offset < 16; offset += 4)
            words[destination + offset] = words[source + offset];
    }
    bool RecordsEqual(std::uint32_t left,
                      std::uint32_t right) override {
        calls.push_back({"compare", left, right});
        if (fail_compare) throw std::runtime_error("compare failed");
        return words[left] == words[right];
    }
};

void Fill(Runtime& runtime, std::uint32_t address, std::uint32_t seed) {
    for (std::uint32_t i = 0; i < 9; ++i)
        runtime.words[address + 4 * i] = seed + i;
}

void ExpectRecord(Runtime& runtime, std::uint32_t address,
                  std::uint32_t seed) {
    for (std::uint32_t i = 0; i < 9; ++i)
        Check(runtime.words[address + 4 * i] == seed + i,
              "all 36 bytes copied");
}

void CheckRepeated() {
    Runtime runtime;
    Fill(runtime, 0x9000, 100);
    Fill(runtime, 0x5000, 1);
    Fill(runtime, 0x5000 + 36, 2);
    Check(pal4::GameDataShopGoodsRecordCopy::AssignRepeated(
              0x5000, 0x5000 + 72, 0x9000, runtime) == 0x5000 + 72 &&
              runtime.calls.size() == 12 &&
              runtime.calls[0] == Call{"word", 0x5000, 100} &&
              runtime.calls[4] == Call{"word", 0x5000 + 16, 104} &&
              runtime.calls[5] == Call{"text", 0x5000 + 20, 0x9000 + 20} &&
              runtime.calls[6] == Call{"word", 0x5000 + 36, 100},
          "repeated assignment writes five words then text for each record");
    ExpectRecord(runtime, 0x5000, 100);
    ExpectRecord(runtime, 0x5000 + 36, 100);
    runtime.calls.clear();
    Check(pal4::GameDataShopGoodsRecordCopy::AssignRepeated(
              0x5000, 0x5000, 0x9000, runtime) == 0x5000 &&
              runtime.calls.empty(), "empty range does not assign");
    Check(pal4::GameDataShopGoodsRecordCopy::AssignRepeated(
              0x5000, 0x5000 + 36, 0x5000, runtime) == 0x5000 + 36 &&
              runtime.calls.back() ==
                  Call{"text", 0x5000 + 20, 0x5000 + 20},
          "self-assignment reaches text ownership runtime");
}

void CheckBackward() {
    Runtime runtime;
    Fill(runtime, 0x5000, 100);
    Fill(runtime, 0x5000 + 36, 200);
    Fill(runtime, 0x5000 + 72, 300);
    Check(pal4::GameDataShopGoodsRecordCopy::AssignBackward(
              0x5000, 0x5000 + 72, 0x5000 + 108, runtime) ==
              0x5000 + 36 &&
              runtime.calls.size() == 12 &&
              runtime.calls.front() ==
                  Call{"word", 0x5000 + 72, 200} &&
              runtime.calls[6] == Call{"word", 0x5000 + 36, 100},
          "backward assignment begins with final source record");
    ExpectRecord(runtime, 0x5000, 100);
    ExpectRecord(runtime, 0x5000 + 36, 100);
    ExpectRecord(runtime, 0x5000 + 72, 200);
    runtime.calls.clear();
    Check(pal4::GameDataShopGoodsRecordCopy::AssignBackward(
              0x5000, 0x5000, 0x7000, runtime) == 0x7000 &&
              runtime.calls.empty(), "empty backward range returns end");
}

void CheckInterruption() {
    Runtime runtime;
    Fill(runtime, 0x9000, 100);
    Fill(runtime, 0x5000, 1);
    runtime.fail_text = 1;
    try {
        static_cast<void>(
            pal4::GameDataShopGoodsRecordCopy::AssignRepeated(
                0x5000, 0x5000 + 36, 0x9000, runtime));
        Check(false, "text failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "text failed" &&
                  runtime.calls.size() == 6 &&
                  runtime.words[0x5000 + 16] == 104 &&
                  runtime.words[0x5000 + 20] == 6,
              "failure leaves text and later records untouched");
    }
}

void CheckConstruct() {
    Runtime runtime;
    Fill(runtime, 0x9000, 100);
    runtime.bytes[0x9000 + 20] = 7;
    pal4::GameDataShopGoodsRecordCopy::Construct(0x5000, 0x9000, runtime);
    Check(runtime.calls.size() == 10 &&
              runtime.calls[0] == Call{"word", 0x5000, 100} &&
              runtime.calls[4] == Call{"word", 0x5000 + 16, 104} &&
              runtime.calls[5] == Call{"byte", 0x5000 + 20, 7} &&
              runtime.calls[6] == Call{"word", 0x5000 + 24, 0} &&
              runtime.calls[9] ==
                  Call{"text", 0x5000 + 20, 0x9000 + 20} &&
              runtime.bytes[0x5000 + 20] == 7,
          "Good construction initializes text after five scalar words");
    runtime.calls.clear();
    pal4::GameDataShopGoodsRecordCopy::Construct(0, 0x9000, runtime);
    Check(runtime.calls.empty(), "null destination has no side effects");

    Runtime interrupted;
    Fill(interrupted, 0x9000, 100);
    interrupted.fail_text = 1;
    try {
        pal4::GameDataShopGoodsRecordCopy::Construct(
            0x5000, 0x9000, interrupted);
        Check(false, "constructor text failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "text failed" &&
                  interrupted.calls.size() == 10 &&
                  interrupted.words[0x5000 + 16] == 104 &&
                  interrupted.words[0x5000 + 24] == 0,
              "constructor text failure follows field initialization");
    }
}

void CheckAssign() {
    Runtime runtime;
    Fill(runtime, 0x9000, 100);
    Check(pal4::GameDataShopGoodsRecordCopy::Assign(
              0x5000, 0x9000, runtime) == 0x5000 &&
              runtime.calls.size() == 6 &&
              runtime.calls[5] ==
                  Call{"text", 0x5000 + 20, 0x9000 + 20},
          "single Good assignment returns destination and copies text last");
    ExpectRecord(runtime, 0x5000, 100);
}

void CheckUnique() {
    Runtime runtime;
    for (std::uint32_t i = 0; i < 4; ++i)
        Fill(runtime, 0x8000 + 36 * i, 100 + i * 100);
    runtime.words[0x8000 + 36] = 100;
    runtime.words[0x8000 + 108] = 100;
    Check(pal4::GameDataShopGoodsRecordCopy::CopyUnique(
              0x8000, 0x8000 + 4 * 36, 0x5000, runtime) ==
              0x5000 + 3 * 36 &&
              runtime.calls[6] == Call{"compare", 0x8000, 0x8000 + 36} &&
              runtime.calls[7] == Call{"compare", 0x8000, 0x8000 + 72} &&
              runtime.calls[14] ==
                  Call{"compare", 0x8000 + 72, 0x8000 + 108},
          "unique comparison follows the last retained source record");
    Check(runtime.words[0x5000] == 100 &&
              runtime.words[0x5000 + 36] == 300 &&
              runtime.words[0x5000 + 72] == 100,
          "later recurrence is retained after a different key");
    runtime.calls.clear();
    Check(pal4::GameDataShopGoodsRecordCopy::CopyUnique(
              0x8000, 0x8000, 0x5000, runtime) == 0x5000 &&
              runtime.calls.empty(), "empty unique range preserves output");

    runtime.fail_compare = true;
    try {
        static_cast<void>(
            pal4::GameDataShopGoodsRecordCopy::CopyUnique(
                0x8000, 0x8000 + 72, 0x6000, runtime));
        Check(false, "comparison failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "compare failed" &&
                  runtime.calls.size() == 7 &&
                  runtime.words[0x6000] == 100 &&
                  runtime.words[0x6000 + 36] == 0,
              "comparison failure follows first copy only");
    }
}

}  // namespace

int main() {
    try {
        CheckRepeated();
        CheckBackward();
        CheckInterruption();
        CheckConstruct();
        CheckAssign();
        CheckUnique();
        std::cout << "game_data_shop_goods_record_copy_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
