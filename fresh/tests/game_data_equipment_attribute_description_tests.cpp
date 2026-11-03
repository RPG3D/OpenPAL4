#include "pal4/game_data_equipment_attribute_description.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataEquipmentAttributeDescriptionRuntime {
    std::map<std::uint32_t, std::uint32_t> indexed;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, float> floats;
    std::vector<std::uint32_t> indexed_reads;
    std::vector<std::uint32_t> word_reads;
    std::vector<std::uint32_t> float_reads;
    std::vector<std::uint32_t> labels;
    std::string output;
    std::uint32_t byte_reads = 0;
    std::uint32_t copies = 0;

    std::uint32_t IndexedValue(std::uint32_t object,
                               std::uint32_t index) override {
        Check(object == 0x1000, "indexed value owner");
        indexed_reads.push_back(index);
        return indexed[index];
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        word_reads.push_back(address);
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == 0x117c, "first special field byte offset");
        ++byte_reads;
        return static_cast<std::uint8_t>(words[address]);
    }
    float ReadFloat(std::uint32_t address) override {
        float_reads.push_back(address);
        return floats[address];
    }
    std::string Label(std::uint32_t key) override {
        labels.push_back(key);
        return "K" + std::to_string(key);
    }
    void CopyDescription(std::uint32_t destination,
                         const std::string& text) override {
        Check(destination == 0x7000, "text destination");
        output = text;
        ++copies;
    }
};

void CheckEveryField() {
    Runtime runtime;
    constexpr std::array<std::uint32_t, 19> integer_sources{
        0, 1, 2, 3, 4, 5, 6, 7, 31, 32, 33, 34, 35,
        13, 14, 15, 16, 17, 18};
    constexpr std::array<std::uint32_t, 19> integer_keys{
        0x29, 0x2a, 0x2b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
        0x11, 0x12, 0x13, 0x14, 0x15,
        0x42, 0x43, 0x44, 0x45, 0x46, 0x47};
    constexpr std::array<std::uint32_t, 16> percent_sources{
        25, 26, 27, 28, 29, 30, 19, 20,
        21, 22, 23, 24, 9, 10, 11, 12};
    for (std::uint32_t index = 0; index < 8; ++index)
        runtime.indexed[index] = index + 1;
    for (std::uint32_t i = 8; i < integer_sources.size(); ++i)
        runtime.words[0x1000 + integer_sources[i] * 4] =
            integer_sources[i] + 1;
    for (const auto index : percent_sources)
        runtime.floats[0x1000 + index * 4] = 0.125f;
    runtime.words[0x117c] = 0x123;
    runtime.words[0x1184] = 5;

    Check(pal4::GameDataEquipmentAttributeDescription::Build(
              0x1000, 0x7000, runtime) == 0x7000 && runtime.copies == 1,
          "description returns and writes destination once");
    Check(runtime.indexed_reads ==
              std::vector<std::uint32_t>{0, 1, 2, 3, 4, 5, 6, 7},
          "first eight values use indexed lookup");
    for (std::uint32_t i = 8; i < integer_sources.size(); ++i)
        Check(runtime.word_reads[i - 8] ==
                  0x1000 + integer_sources[i] * 4,
              "remaining integer values use direct member offsets");
    for (std::uint32_t i = 0; i < percent_sources.size(); ++i)
        Check(runtime.float_reads[i] ==
                  0x1000 + percent_sources[i] * 4,
              "percent values preserve IDA source order");
    std::string expected;
    for (std::uint32_t i = 0; i < integer_sources.size(); ++i)
        expected += "K" + std::to_string(integer_keys[i]) + "+" +
                    std::to_string(integer_sources[i] + 1) + " ";
    for (std::uint32_t key = 0x48; key <= 0x57; ++key)
        expected += "K" + std::to_string(key) + "+12% ";
    expected += "K89:35 K88:5 ";
    Check(runtime.output == expected && runtime.byte_reads == 1,
          "full description has exact labels, values and punctuation");
    Check(runtime.labels.size() == 37 &&
              runtime.labels.front() == 0x29 &&
              runtime.labels[18] == 0x47 &&
              runtime.labels[19] == 0x48 &&
              runtime.labels[34] == 0x57 &&
              runtime.labels[35] == 0x59 &&
              runtime.labels[36] == 0x58,
          "all 37 GameDB label keys retain their order");
}

void CheckConditionalAndSignedValues() {
    Runtime runtime;
    runtime.indexed[0] = 0xfffffffe;
    runtime.words[0x107c] = 0xffffffff;
    runtime.floats[0x1064] = -0.139f;
    runtime.words[0x117c] = 0xffffffff;
    runtime.words[0x1184] = 0xffffffff;
    Check(pal4::GameDataEquipmentAttributeDescription::Build(
              0x1000, 0x7000, runtime) == 0x7000 &&
              runtime.output == "K41+-2 K17+-1 K72+-13% K89:0 K88:0 " &&
              runtime.byte_reads == 0 && runtime.copies == 1,
          "zero values skip labels, negatives stay signed and percent truncates");
    runtime = Runtime{};
    Check(pal4::GameDataEquipmentAttributeDescription::Build(
              0x1000, 0x7000, runtime) == 0x7000 &&
              runtime.output == "K89:0 K88:0 " && runtime.labels.size() == 2,
          "all-zero object emits only the two unconditional tail fields");
}

}  // namespace

int main() {
    try {
        CheckEveryField();
        CheckConditionalAndSignedValues();
        std::cout << "game_data_equipment_attribute_description_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
