#include "pal4/ui_cegui_string_record.h"

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

struct Runtime final : pal4::IUiCeguiStringRecordRuntime {
    static constexpr std::uint32_t record = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    bool throw_on_destroy = false;

    Runtime() {
        words[record + 156u] = 0u;
        words[record + 160u] = 2u;
        words[record + 164u] = 3u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back("word " + std::to_string(address));
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back("byte " + std::to_string(address));
    }
    void FreeMemory(std::uint32_t address) override {
        calls.push_back("free " + std::to_string(address));
    }
    void DestroyString(std::uint32_t address) override {
        calls.push_back("destroy " + std::to_string(address));
        if (throw_on_destroy) throw std::runtime_error("CEGUI destructor");
    }
};

void CheckEmptyAndShared() {
    Runtime empty;
    pal4::UiCeguiStringRecord::Free(empty.record, empty);
    Check(empty.calls == (std::vector<std::string>{
              "word 4252", "word 4256", "word 4260", "destroy 4096"}),
          "empty text clears fields and destroys string");

    Runtime shared;
    shared.words[shared.record + 156u] = 0x8001u;
    shared.bytes[0x8000u] = 2u;
    pal4::UiCeguiStringRecord::Free(shared.record, shared);
    Check(shared.bytes.at(0x8000u) == 1u &&
              shared.calls == (std::vector<std::string>{
                  "byte 32768", "word 4252", "word 4256",
                  "word 4260", "destroy 4096"}),
          "shared text decremented before destructor");
}

void CheckFinalOwnersAndFailure() {
    for (const auto count : {0u, 0xffu}) {
        Runtime r;
        r.words[r.record + 156u] = 0x8001u;
        r.bytes[0x8000u] = static_cast<std::uint8_t>(count);
        pal4::UiCeguiStringRecord::Free(r.record, r);
        Check(r.calls.front() == "free 32768" &&
                  r.calls.back() == "destroy 4096" &&
                  r.words.at(r.record + 156u) == 0u &&
                  r.words.at(r.record + 160u) == 0u &&
                  r.words.at(r.record + 164u) == 0u,
              "zero and sentinel counts release text header");
    }

    Runtime failing;
    failing.throw_on_destroy = true;
    bool threw = false;
    try {
        pal4::UiCeguiStringRecord::Free(failing.record, failing);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && failing.words.at(failing.record + 156u) == 0u &&
              failing.words.at(failing.record + 160u) == 0u &&
              failing.words.at(failing.record + 164u) == 0u,
          "destructor failure occurs after fields are cleared");
}

}  // namespace

int main() {
    try {
        CheckEmptyAndShared();
        CheckFinalOwnersAndFailure();
        std::cout << "ui_cegui_string_record_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
