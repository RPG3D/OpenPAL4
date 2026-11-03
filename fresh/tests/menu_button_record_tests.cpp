#include "pal4/menu_button_record.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IMenuButtonRecordDataRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t grown_string = 0u;
    std::uint32_t grown_length = 0u;
    std::uint32_t assigned_destination = 0u;
    std::uint32_t assigned_source = 0u;
    std::uint32_t assigned_offset = 1u;
    std::uint32_t assigned_length = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write");
        words[address] = value;
    }
    void GrowString(std::uint32_t string,
                    std::uint32_t length) override {
        calls.emplace_back("grow");
        grown_string = string;
        grown_length = length;
        if (length > pal4::MenuButtonRecord::kInlineCapacity) {
            words[string + 4u] = length + 8u;
            words[string + 148u] = 0xd000u;
        }
    }
    void AssignString(std::uint32_t destination,
                      std::uint32_t source,
                      std::uint32_t offset,
                      std::uint32_t length) override {
        calls.emplace_back("assign");
        assigned_destination = destination;
        assigned_source = source;
        assigned_offset = offset;
        assigned_length = length;
    }
};

void SeedSource(Runtime& runtime, std::uint32_t length,
                bool heap_source) {
    constexpr std::uint32_t source = 0x2000u;
    runtime.words[source] = 11u;
    runtime.words[source + 4u] = 22u;
    runtime.words[source + 8u] = 33u;
    runtime.words[source + 12u] = length;
    runtime.words[source + 16u] = heap_source ? length + 8u : 32u;
    if (heap_source) runtime.words[source + 160u] = 0xc000u;
    const auto data = heap_source ? 0xc000u : source + 32u;
    for (std::uint32_t index = 0u; index < length; ++index)
        runtime.words[data + index * 4u] = 0x100u + index;
}

void CheckCopy(std::uint32_t length, bool heap_source) {
    Runtime runtime;
    SeedSource(runtime, length, heap_source);
    constexpr std::uint32_t destination = 0x3000u;
    runtime.words[destination + 160u] = 0xeeeeu;
    Check(pal4::MenuButtonRecord::CopyConstruct(
              destination, 0x2000u, runtime) == length * 4u,
          "copy return byte count");
    Check(runtime.words.at(destination) == 11u &&
          runtime.words.at(destination + 4u) == 22u &&
          runtime.words.at(destination + 8u) == 33u &&
          runtime.words.at(destination + 12u) == length &&
          runtime.words.at(destination + 20u) == 0u &&
          runtime.words.at(destination + 24u) == 0u &&
          runtime.words.at(destination + 28u) == 0u,
          "record fields and initialized string state");
    Check(runtime.grown_string == destination + 12u &&
          runtime.grown_length == length,
          "grow receives source length");
    const auto destination_data = length > 32u
        ? 0xd000u : destination + 32u;
    const auto source_data = heap_source ? 0xc000u : 0x2020u;
    for (std::uint32_t index = 0u; index < length; ++index)
        Check(runtime.words.at(destination_data + index * 4u) ==
                  runtime.words.at(source_data + index * 4u),
              "UTF-32 copy");
    Check(runtime.words.at(destination_data + length * 4u) == 0u,
          "UTF-32 terminator");
    Check(runtime.words.at(destination + 16u) ==
              (length > 32u ? length + 8u : 32u),
          "inline and grown capacities");
    Check(runtime.calls[13] == "grow", "grow follows initialization");
}

void CheckNullDestination() {
    Runtime runtime;
    Check(pal4::MenuButtonRecord::CopyConstruct(
              0u, 0x2000u, runtime) == 0u && runtime.calls.empty(),
          "null destination is inert");
}

void CheckAssign() {
    Runtime runtime;
    runtime.words[0x2000u] = 4u;
    runtime.words[0x2004u] = 5u;
    runtime.words[0x2008u] = 6u;
    runtime.words[0x300cu] = 9u;
    Check(pal4::MenuButtonRecord::Assign(
              0x3000u, 0x2000u, runtime) == 0x3000u,
          "assign returns destination");
    Check(runtime.words.at(0x3000u) == 4u &&
          runtime.words.at(0x3004u) == 5u &&
          runtime.words.at(0x3008u) == 6u &&
          runtime.words.at(0x300cu) == 9u,
          "assign copies header and leaves string to CEGUI");
    Check(runtime.assigned_destination == 0x300cu &&
          runtime.assigned_source == 0x200cu &&
          runtime.assigned_offset == 0u &&
          runtime.assigned_length == pal4::MenuButtonRecord::kNpos &&
          runtime.calls.back() == "assign",
          "CEGUI full-string assignment");
}

}  // namespace

int main() {
    try {
        CheckNullDestination();
        CheckCopy(0u, false);
        CheckCopy(1u, false);
        CheckCopy(31u, false);
        CheckCopy(32u, false);
        CheckCopy(33u, true);
        CheckCopy(50u, true);
        CheckAssign();
        std::cout << "menu_button_record_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
