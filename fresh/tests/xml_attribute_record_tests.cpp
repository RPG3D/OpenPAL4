#include "pal4/xml_attribute_record.h"

#include <algorithm>
#include <bit>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <new>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IXmlAttributeRecordRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> freed_blocks;
    std::vector<std::uint32_t> freed_raw;
    bool fail_allocation = false;
    bool fail_text_allocation = false;
    bool fail_child_allocation = false;
    bool fail_raw_allocation = false;
    bool fail_assign = false;
    std::uint32_t allocated_text_length = 0xffffffffu;
    std::uint32_t allocated_child_size = 0u;
    std::uint32_t expected_reallocate_length = 16u;
    std::uint32_t expected_child_reallocate_length = 0u;
    std::uint32_t allocated_raw_size = 0u;
    std::uint32_t assigned_source = 0u;
    std::uint32_t converted_float_bits = 0u;

    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("word");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back("byte");
    }
    void SetErrorFlag(std::uint32_t record, std::uint8_t flag) override {
        Check((record == 0x3000u || record == 0x6000u) && flag == 1u,
              "initial error-flag call arguments");
        calls.push_back("flag");
    }
    void Reallocate(std::uint32_t record, std::uint32_t length) override {
        Check((record == 0x3000u &&
               length == expected_reallocate_length) ||
                  (record == 0x6000u &&
                   length == expected_child_reallocate_length),
              "name allocation uses exact string length");
        calls.push_back("reallocate");
        if (!fail_allocation)
            words[record + 4u] = record == 0x3000u ? 0x4000u : 0x8000u;
    }
    bool AllocateText(std::uint32_t record, std::uint32_t length,
                      bool null_terminated) override {
        Check((record == 0x3000u || record == 0xb000u) && null_terminated,
              "name text allocation arguments");
        allocated_text_length = length;
        calls.push_back("text");
        if (fail_text_allocation || length == 0u) return false;
        words[record + 4u] = record == 0x3000u ? 0x4000u : 0xc000u;
        return true;
    }
    std::uint32_t AllocateMemory(std::uint32_t bytes) override {
        allocated_child_size = bytes;
        calls.push_back("memory");
        return fail_child_allocation ? 0u : 0x6000u;
    }
    void AssignName(std::uint32_t destination, std::uint32_t source,
                    std::uint32_t start, std::uint32_t end) override {
        Check(destination == 0x3000u && start == 0u && end == 0xffffffffu,
              "copy constructor assigns the entire source name");
        assigned_source = source;
        calls.push_back("assign");
        if (fail_assign) throw std::bad_alloc{};
        const auto source_text = words[source + 4u];
        const auto length = words[source + 8u];
        if (source_text == 0u) return;
        words[destination + 4u] = 0x4000u;
        words[destination + 8u] = length;
        for (std::uint32_t index = 0u; index < length; ++index)
            bytes[0x4000u + index] = bytes[source_text + index];
        bytes[0x4000u + length] = 0u;
    }
    std::uint32_t AllocateRaw(std::uint32_t count) override {
        allocated_raw_size = count;
        calls.push_back("raw");
        return fail_raw_allocation ? 0u : 0x9000u;
    }
    std::uint32_t FreeBlock(std::uint32_t address) override {
        freed_blocks.push_back(address);
        calls.push_back("free-block");
        return address + 0x100u;
    }
    std::uint32_t FreeRaw(std::uint32_t address) override {
        freed_raw.push_back(address);
        calls.push_back("free-raw");
        return address + 0x200u;
    }
    std::int32_t FloatToInt(std::uint32_t bits) override {
        converted_float_bits = bits;
        return static_cast<std::int32_t>(std::bit_cast<float>(bits));
    }
    std::uint8_t ScanChar(std::uint32_t text,
                          std::uint8_t fallback) override {
        const auto source = CString(text);
        char result = static_cast<char>(fallback);
        (void)std::sscanf(source.c_str(), "%c", &result);
        return static_cast<std::uint8_t>(result);
    }
    std::int32_t ScanInt(std::uint32_t text,
                         std::int32_t fallback) override {
        const auto source = CString(text);
        (void)std::sscanf(source.c_str(), "%d", &fallback);
        return fallback;
    }
    std::uint32_t ScanFloatBits(std::uint32_t text,
                                std::uint32_t fallback_bits) override {
        const auto source = CString(text);
        auto result = std::bit_cast<float>(fallback_bits);
        (void)std::sscanf(source.c_str(), "%f", &result);
        return std::bit_cast<std::uint32_t>(result);
    }
    std::string FormatFloat(std::uint32_t bits) override {
        char formatted[128]{};
        (void)std::snprintf(formatted, sizeof(formatted), "%f",
                            static_cast<double>(std::bit_cast<float>(bits)));
        return formatted;
    }
    std::int32_t ParseInt(std::uint32_t text) override {
        const auto source = CString(text);
        std::int32_t result = 0;
        (void)std::sscanf(source.c_str(), "%d", &result);
        return result;
    }
    std::uint32_t ParseFloatBits(std::uint32_t text) override {
        const auto source = CString(text);
        float result = 0.0f;
        (void)std::sscanf(source.c_str(), "%f", &result);
        return std::bit_cast<std::uint32_t>(result);
    }
    std::string CString(std::uint32_t address) {
        std::string result;
        if (address != 0u)
            for (std::uint32_t index = 0u; bytes[address + index] != 0u;
                 ++index)
                result.push_back(static_cast<char>(bytes[address + index]));
        return result;
    }
    std::string Text(std::uint32_t address, std::uint32_t length) {
        std::string result;
        for (std::uint32_t i = 0u; i < length; ++i)
            result.push_back(static_cast<char>(bytes[address + i]));
        return result;
    }
};

void TestDefaultAttributeLayout() {
    Runtime runtime;
    runtime.bytes[0x3000u] = 0x7bu;
    runtime.words[0x3014u] = 0xaaaaaaaau;
    runtime.words[0x3018u] = 0xbbbbbbbbu;
    Check(pal4::XmlAttributeRecord::Initialize(0x3000u, runtime) == 0x3000u &&
              runtime.words[0x3004u] == 0x4000u &&
              runtime.words[0x3008u] == 16u &&
              runtime.words[0x300cu] == 0u &&
              runtime.words[0x3010u] == 4u &&
              runtime.words[0x3014u] == 0u &&
              runtime.words[0x3018u] == 0u &&
              runtime.Text(0x4000u, 16u) == "ATTR_NAME_UNKNOW" &&
              runtime.bytes[0x4010u] == 0u &&
              runtime.bytes[0x3000u] == 0x7bu &&
              runtime.calls[4] == "flag" &&
              runtime.calls[5] == "reallocate",
          "default attribute fields, name and allocation order match IDA");
}

void TestAllocationFailure() {
    Runtime runtime;
    runtime.fail_allocation = true;
    runtime.words[0x3014u] = 0xaaaaaaaau;
    bool caught = false;
    try {
        (void)pal4::XmlAttributeRecord::Initialize(0x3000u, runtime);
    } catch (const std::bad_alloc&) {
        caught = true;
    }
    Check(caught && runtime.words[0x3004u] == 0u &&
              runtime.words[0x3010u] == 4u &&
              runtime.words[0x3014u] == 0xaaaaaaaau,
          "failed name allocation stops before tail field initialization");
}

void TestNamedAttributeWithChild() {
    Runtime runtime;
    constexpr std::string_view name = "sprite-id";
    for (std::uint32_t i = 0u; i < name.size(); ++i)
        runtime.bytes[0x5000u + i] = static_cast<std::uint8_t>(name[i]);
    runtime.bytes[0x5000u + static_cast<std::uint32_t>(name.size())] = 0u;
    runtime.words[0x6000u] = 0xdeadbeefu;
    Check(pal4::XmlAttributeRecord::InitializeWithValue(
              0x3000u, 3u, 0x5000u, runtime) == 0x3000u &&
              runtime.bytes[0x3000u] == 3u &&
              runtime.words[0x3004u] == 0x4000u &&
              runtime.words[0x3008u] == name.size() &&
              runtime.words[0x300cu] == 0u &&
              runtime.words[0x3010u] == 3u &&
              runtime.words[0x3014u] == 0u &&
              runtime.words[0x3018u] == 0x6000u &&
              runtime.Text(0x4000u, static_cast<std::uint32_t>(name.size())) ==
                  name &&
              runtime.bytes[0x4000u + static_cast<std::uint32_t>(name.size())] ==
                  0u &&
              runtime.bytes[0x6000u] == 3u &&
              runtime.words[0x6004u] == 0u &&
              runtime.words[0x6008u] == 0u &&
              runtime.words[0x600cu] == 0u &&
              runtime.allocated_text_length == name.size() &&
              runtime.allocated_child_size == 16u &&
              runtime.calls[5] == "text" &&
              runtime.calls[runtime.calls.size() - 6u] == "memory",
          "named type-three record copies the name and initializes its child");
}

void TestOtherTypeAndEmptyName() {
    Runtime runtime;
    runtime.bytes[0x5000u] = 0u;
    Check(pal4::XmlAttributeRecord::InitializeWithValue(
              0x3000u, 0x103u, 0x5000u, runtime) == 0x3000u &&
              runtime.bytes[0x3000u] == 3u &&
              runtime.words[0x3010u] == 0x103u &&
              runtime.words[0x3008u] == 0u &&
              runtime.words[0x3004u] == 0u &&
              runtime.words[0x3018u] == 0u &&
              runtime.allocated_text_length == 0u &&
              runtime.allocated_child_size == 0u,
          "only exact type three allocates a child; empty names stay empty");
}

void TestNullNameThrowsAfterInitialFields() {
    Runtime runtime;
    runtime.words[0x3010u] = 0xaaaaaaaau;
    runtime.words[0x3014u] = 0xbbbbbbbbu;
    bool caught = false;
    try {
        (void)pal4::XmlAttributeRecord::InitializeWithValue(
            0x3000u, 3u, 0u, runtime);
    } catch (const std::runtime_error& error) {
        caught = std::string(error.what()) == "XmlAttribute's name is null!";
    }
    Check(caught && runtime.bytes[0x3000u] == 3u &&
              runtime.words[0x3004u] == 0u &&
              runtime.words[0x3008u] == 0u &&
              runtime.words[0x300cu] == 0u &&
              runtime.words[0x3010u] == 0xaaaaaaaau &&
              runtime.words[0x3014u] == 0xbbbbbbbbu &&
              runtime.calls.size() == 4u,
          "null name throws before type and tail fields are initialized");
}

void TestNameAllocationFailureStillCreatesChild() {
    Runtime runtime;
    runtime.fail_text_allocation = true;
    runtime.bytes[0x5000u] = 'x';
    runtime.bytes[0x5001u] = 0u;
    Check(pal4::XmlAttributeRecord::InitializeWithValue(
              0x3000u, 3u, 0x5000u, runtime) == 0x3000u &&
              runtime.allocated_text_length == 1u &&
              runtime.words[0x3004u] == 0u &&
              runtime.words[0x3008u] == 0u &&
              runtime.words[0x3010u] == 3u &&
              runtime.words[0x3018u] == 0x6000u &&
              runtime.bytes[0x6000u] == 3u,
          "failed name allocation leaves name empty but continues to child");
}

void TestChildAllocationFailure() {
    Runtime runtime;
    runtime.fail_child_allocation = true;
    runtime.bytes[0x5000u] = 'x';
    runtime.bytes[0x5001u] = 0u;
    Check(pal4::XmlAttributeRecord::InitializeWithValue(
              0x3000u, 3u, 0x5000u, runtime) == 0x3000u &&
              runtime.words[0x3008u] == 1u &&
              runtime.Text(0x4000u, 1u) == "x" &&
              runtime.words[0x3018u] == 0u &&
              runtime.allocated_child_size == 16u,
          "failed child allocation leaves a null child pointer");
}

void TestPointerValueConstructors() {
    for (const std::uint32_t type : {1u, 2u}) {
        Runtime runtime;
        constexpr std::string_view name = "level";
        constexpr std::uint32_t address = 0x507bu;
        runtime.expected_reallocate_length =
            static_cast<std::uint32_t>(name.size());
        for (std::uint32_t index = 0u; index < name.size(); ++index)
            runtime.bytes[address + index] =
                static_cast<std::uint8_t>(name[index]);
        runtime.bytes[address + static_cast<std::uint32_t>(name.size())] = 0u;
        runtime.words[0x7000u] = 0x87654321u;
        const auto result = type == 1u
            ? pal4::XmlAttributeRecord::InitializeWithPointerValue(
                  0x3000u, address, 0x7000u, runtime)
            : pal4::XmlAttributeRecord::InitializeWithPointerValue2(
                  0x3000u, address, 0x7000u, runtime);
        Check(result == 0x3000u &&
                  runtime.bytes[0x3000u] == 0x7bu &&
                  runtime.words[0x3004u] == 0x4000u &&
                  runtime.words[0x3008u] == name.size() &&
                  runtime.words[0x300cu] == 0u &&
                  runtime.words[0x3010u] == type &&
                  runtime.words[0x3014u] == 0u &&
                  runtime.words[0x3018u] == 0x87654321u &&
                  runtime.Text(0x4000u, static_cast<std::uint32_t>(name.size())) ==
                      name &&
                  runtime.bytes[0x4000u + static_cast<std::uint32_t>(name.size())] ==
                      0u &&
                  runtime.calls[5] == "flag" &&
                  runtime.calls[6] == "reallocate" &&
                  runtime.allocated_child_size == 0u,
              "pointer-value constructors preserve distinct types and copy word");
    }
}

void TestEmptyPointerValueName() {
    Runtime runtime;
    runtime.bytes[0x507bu] = 0u;
    runtime.words[0x7000u] = 0x12345678u;
    Check(pal4::XmlAttributeRecord::InitializeWithPointerValue2(
              0x3000u, 0x507bu, 0x7000u, runtime) == 0x3000u &&
              runtime.bytes[0x3000u] == 0x7bu &&
              runtime.words[0x3004u] == 0u &&
              runtime.words[0x3008u] == 0u &&
              runtime.words[0x3010u] == 2u &&
              runtime.words[0x3018u] == 0x12345678u &&
              runtime.calls[5] == "flag" &&
              runtime.calls.size() == 8u,
          "empty pointer-value name resets text without allocation");
}

void TestNullPointerValueName() {
    Runtime runtime;
    runtime.words[0x3010u] = 0xaaaaaaaau;
    bool caught = false;
    try {
        (void)pal4::XmlAttributeRecord::InitializeWithPointerValue(
            0x3000u, 0u, 0x7000u, runtime);
    } catch (const std::runtime_error& error) {
        caught = std::string(error.what()) == "XmlAttribute's name is null!";
    }
    Check(caught && runtime.bytes[0x3000u] == 0u &&
              runtime.words[0x3004u] == 0u &&
              runtime.words[0x3008u] == 0u &&
              runtime.words[0x300cu] == 0u &&
              runtime.words[0x3010u] == 0xaaaaaaaau &&
              runtime.calls.size() == 4u,
          "pointer-value null name throws before type and value writes");
}

void TestTypeThreeTextValue() {
    Runtime runtime;
    constexpr std::uint32_t name_address = 0x507bu;
    for (std::uint32_t i = 0u; i < 4u; ++i)
        runtime.bytes[name_address + i] =
            static_cast<std::uint8_t>("node"[i]);
    runtime.bytes[name_address + 4u] = 0u;
    runtime.bytes[0x7000u] = 'a';
    runtime.bytes[0x7001u] = 'b';
    runtime.bytes[0x7002u] = 'c';
    runtime.bytes[0x7003u] = 0u;
    runtime.expected_child_reallocate_length = 3u;
    Check(pal4::XmlAttributeRecord::InitializeWithValueAndType(
              0x3000u, name_address, 0x7000u, runtime) == 0x3000u &&
              runtime.bytes[0x3000u] == 0x7bu &&
              runtime.words[0x3004u] == 0x4000u &&
              runtime.words[0x3008u] == 4u &&
              runtime.words[0x3010u] == 3u &&
              runtime.words[0x3014u] == 0u &&
              runtime.words[0x3018u] == 0x6000u &&
              runtime.Text(0x4000u, 4u) == "node" &&
              runtime.bytes[0x4004u] == 0u &&
              runtime.bytes[0x6000u] == 0x7bu &&
              runtime.words[0x6004u] == 0x8000u &&
              runtime.words[0x6008u] == 3u &&
              runtime.words[0x600cu] == 0u &&
              runtime.Text(0x8000u, 3u) == "abc" &&
              runtime.bytes[0x8003u] == 0u &&
              runtime.allocated_text_length == 4u &&
              runtime.allocated_child_size == 16u,
          "type-three constructor initializes parent name and child value");
}

void TestEmptyTypeThreeTextValue() {
    Runtime runtime;
    runtime.bytes[0x507bu] = 0u;
    Check(pal4::XmlAttributeRecord::InitializeWithValueAndType(
              0x3000u, 0x507bu, 0u, runtime) == 0x3000u &&
              runtime.words[0x3008u] == 0u &&
              runtime.words[0x3004u] == 0u &&
              runtime.words[0x6004u] == 0u &&
              runtime.words[0x6008u] == 0u &&
              runtime.words[0x3018u] == 0x6000u &&
              runtime.allocated_text_length == 0u &&
              runtime.allocated_child_size == 16u &&
              runtime.calls.back() == "flag",
          "null value uses an empty child string without reallocation");
}

void TestTypeThreeNameAllocationFailure() {
    Runtime runtime;
    runtime.fail_text_allocation = true;
    runtime.bytes[0x507bu] = 'n';
    runtime.bytes[0x507cu] = 0u;
    runtime.bytes[0x7000u] = 'v';
    runtime.bytes[0x7001u] = 0u;
    runtime.expected_child_reallocate_length = 1u;
    Check(pal4::XmlAttributeRecord::InitializeWithValueAndType(
              0x3000u, 0x507bu, 0x7000u, runtime) == 0x3000u &&
              runtime.words[0x3004u] == 0u &&
              runtime.words[0x3008u] == 0u &&
              runtime.words[0x3018u] == 0x6000u &&
              runtime.Text(0x8000u, 1u) == "v",
          "failed parent name allocation still initializes child value");
}

void TestTypeThreeChildAllocationFailure() {
    Runtime runtime;
    runtime.fail_child_allocation = true;
    runtime.bytes[0x507bu] = 'n';
    runtime.bytes[0x507cu] = 0u;
    runtime.words[0x3018u] = 0xaaaaaaaau;
    bool caught = false;
    try {
        (void)pal4::XmlAttributeRecord::InitializeWithValueAndType(
            0x3000u, 0x507bu, 0u, runtime);
    } catch (const std::bad_alloc&) {
        caught = true;
    }
    Check(caught && runtime.words[0x3008u] == 1u &&
              runtime.words[0x3014u] == 0u &&
              runtime.words[0x3018u] == 0xaaaaaaaau &&
              runtime.allocated_child_size == 16u,
          "failed child allocation becomes a defined C++ exception");
}

void TestTypeThreeNullName() {
    Runtime runtime;
    bool caught = false;
    try {
        (void)pal4::XmlAttributeRecord::InitializeWithValueAndType(
            0x3000u, 0u, 0u, runtime);
    } catch (const std::runtime_error& error) {
        caught = std::string(error.what()) == "XmlAttribute's name is null!";
    }
    Check(caught && runtime.calls.size() == 4u &&
              runtime.allocated_child_size == 0u,
          "type-three null name throws before allocation");
}

void PrepareCopySource(Runtime& runtime, std::uint32_t type) {
    constexpr std::uint32_t source = 0x317bu;
    runtime.words[source + 4u] = 0xa000u;
    runtime.words[source + 8u] = 3u;
    runtime.words[source + 16u] = type;
    runtime.bytes[0xa000u] = 'k';
    runtime.bytes[0xa001u] = 'e';
    runtime.bytes[0xa002u] = 'y';
    runtime.bytes[0xa003u] = 0u;
}

void TestCopyScalarRecords() {
    for (const auto type : {0u, 1u, 2u}) {
        Runtime runtime;
        PrepareCopySource(runtime, type);
        runtime.bytes[0x317bu + 24u] = 0xa5u;
        runtime.words[0x317bu + 24u] = 0x12345678u;
        Check(pal4::XmlAttributeRecord::CopyConstruct(
                  0x3000u, 0x317bu, runtime) == 0x3000u &&
                  runtime.bytes[0x3000u] == 0x7bu &&
                  runtime.words[0x3004u] == 0x4000u &&
                  runtime.words[0x3008u] == 3u &&
                  runtime.Text(0x4000u, 3u) == "key" &&
                  runtime.words[0x3010u] == type &&
                  runtime.assigned_source == 0x317bu &&
                  runtime.calls[5] == "assign" &&
                  (type == 0u
                      ? runtime.bytes[0x3018u] == 0xa5u
                      : runtime.words[0x3018u] == 0x12345678u) &&
                  runtime.allocated_child_size == 0u &&
                  runtime.allocated_raw_size == 0u,
              "copy constructor preserves scalar type and value width");
    }
}

void TestCopyChildText() {
    Runtime runtime;
    PrepareCopySource(runtime, 3u);
    runtime.words[0x317bu + 24u] = 0x7100u;
    runtime.words[0x7104u] = 0x7200u;
    runtime.bytes[0x7200u] = 'v';
    runtime.bytes[0x7201u] = 'a';
    runtime.bytes[0x7202u] = 'l';
    runtime.bytes[0x7203u] = 0u;
    runtime.expected_child_reallocate_length = 3u;
    Check(pal4::XmlAttributeRecord::CopyConstruct(
              0x3000u, 0x317bu, runtime) == 0x3000u &&
              runtime.words[0x3018u] == 0x6000u &&
              runtime.bytes[0x6000u] == 0x7bu &&
              runtime.words[0x6004u] == 0x8000u &&
              runtime.words[0x6008u] == 3u &&
              runtime.words[0x600cu] == 0u &&
              runtime.Text(0x8000u, 3u) == "val" &&
              runtime.bytes[0x8003u] == 0u &&
              runtime.allocated_child_size == 16u,
          "type-three copy owns a distinct child text buffer");
}

void TestCopyEmptyChildText() {
    Runtime runtime;
    PrepareCopySource(runtime, 3u);
    runtime.words[0x317bu + 24u] = 0x7100u;
    Check(pal4::XmlAttributeRecord::CopyConstruct(
              0x3000u, 0x317bu, runtime) == 0x3000u &&
              runtime.words[0x3018u] == 0x6000u &&
              runtime.words[0x6004u] == 0u &&
              runtime.words[0x6008u] == 0u &&
              runtime.calls.back() == "flag",
          "copy constructor preserves an empty child string");
}

void TestCopyRawValue() {
    Runtime runtime;
    PrepareCopySource(runtime, 4u);
    runtime.words[0x317bu + 20u] = 4u;
    runtime.words[0x317bu + 24u] = 0x7300u;
    runtime.bytes[0x7300u] = 1u;
    runtime.bytes[0x7301u] = 2u;
    runtime.bytes[0x7302u] = 3u;
    runtime.bytes[0x7303u] = 4u;
    Check(pal4::XmlAttributeRecord::CopyConstruct(
              0x3000u, 0x317bu, runtime) == 0x3000u &&
              runtime.words[0x3014u] == 4u &&
              runtime.words[0x3018u] == 0x9000u &&
              runtime.allocated_raw_size == 4u &&
              runtime.bytes[0x9000u] == 1u &&
              runtime.bytes[0x9001u] == 2u &&
              runtime.bytes[0x9002u] == 3u &&
              runtime.bytes[0x9003u] == 4u,
          "other types deep-copy the positive-length raw payload");
}

void TestCopyNonpositiveRawLength() {
    for (const auto length : {0u, 0xffffffffu}) {
        Runtime runtime;
        PrepareCopySource(runtime, 9u);
        runtime.words[0x317bu + 20u] = length;
        runtime.words[0x3018u] = 0xaaaaaaaau;
        Check(pal4::XmlAttributeRecord::CopyConstruct(
                  0x3000u, 0x317bu, runtime) == 0x3000u &&
                  runtime.words[0x3014u] == length &&
                  runtime.words[0x3018u] == 0xaaaaaaaau &&
                  runtime.allocated_raw_size == 0u,
              "nonpositive raw length leaves destination pointer untouched");
    }
}

void TestCopyFailures() {
    Runtime assign_failure;
    PrepareCopySource(assign_failure, 1u);
    assign_failure.fail_assign = true;
    bool caught_assign = false;
    try {
        (void)pal4::XmlAttributeRecord::CopyConstruct(
            0x3000u, 0x317bu, assign_failure);
    } catch (const std::bad_alloc&) {
        caught_assign = true;
    }
    Check(caught_assign && assign_failure.words[0x3010u] == 1u &&
              assign_failure.words[0x3018u] == 0u,
          "name assignment failure stops before value copy");

    Runtime child_failure;
    PrepareCopySource(child_failure, 3u);
    child_failure.fail_child_allocation = true;
    bool caught_child = false;
    try {
        (void)pal4::XmlAttributeRecord::CopyConstruct(
            0x3000u, 0x317bu, child_failure);
    } catch (const std::bad_alloc&) {
        caught_child = true;
    }
    Check(caught_child && child_failure.allocated_child_size == 16u,
          "failed child copy allocation throws");

    Runtime raw_failure;
    PrepareCopySource(raw_failure, 4u);
    raw_failure.words[0x317bu + 20u] = 4u;
    raw_failure.fail_raw_allocation = true;
    bool caught_raw = false;
    try {
        (void)pal4::XmlAttributeRecord::CopyConstruct(
            0x3000u, 0x317bu, raw_failure);
    } catch (const std::bad_alloc&) {
        caught_raw = true;
    }
    Check(caught_raw && raw_failure.words[0x3014u] == 4u &&
              raw_failure.allocated_raw_size == 4u,
          "failed raw copy allocation throws");
}

void TestFreeTypeThreeValue() {
    Runtime runtime;
    runtime.words[0x3010u] = 3u;
    runtime.words[0x3018u] = 0x6000u;
    runtime.words[0x6004u] = 0x8000u;
    runtime.words[0x6008u] = 3u;
    runtime.words[0x600cu] = 31u;
    runtime.bytes[0x7fffu] = 0u;
    Check(pal4::XmlAttributeRecord::FreeValue(0x3000u, runtime) ==
              0x6100u &&
              runtime.freed_blocks ==
                  std::vector<std::uint32_t>{0x7fffu, 0x6000u} &&
              runtime.words[0x6004u] == 0u &&
              runtime.words[0x6008u] == 0u &&
              runtime.words[0x600cu] == 0u &&
              runtime.words[0x3018u] == 0u,
          "type-three cleanup frees child text before child object");
    Check(pal4::XmlAttributeRecord::FreeValue(0x3000u, runtime) == 3u &&
              runtime.freed_blocks.size() == 2u,
          "type-three cleanup of an empty child keeps native return type");
}

void TestFreeSharedChildText() {
    Runtime runtime;
    runtime.words[0x3010u] = 3u;
    runtime.words[0x3018u] = 0x6000u;
    runtime.words[0x6004u] = 0x8000u;
    runtime.bytes[0x7fffu] = 2u;
    Check(pal4::XmlAttributeRecord::FreeValue(0x3000u, runtime) ==
              0x6100u &&
              runtime.bytes[0x7fffu] == 1u &&
              runtime.freed_blocks == std::vector<std::uint32_t>{0x6000u},
          "shared child text decrements its header and frees the child");
}

void TestFreeRawAndOtherValues() {
    Runtime positive;
    positive.words[0x3010u] = 4u;
    positive.words[0x3014u] = 5u;
    positive.words[0x3018u] = 0x9000u;
    Check(pal4::XmlAttributeRecord::FreeValue(0x3000u, positive) ==
              0x9200u &&
              positive.freed_raw == std::vector<std::uint32_t>{0x9000u} &&
              positive.words[0x3014u] == 0u &&
              positive.words[0x3018u] == 0u,
          "positive type-four size frees raw payload and resets size");

    for (const auto size : {0u, 0xffffffffu}) {
        Runtime nonpositive;
        nonpositive.words[0x3010u] = 4u;
        nonpositive.words[0x3014u] = size;
        nonpositive.words[0x3018u] = 0x9000u;
        Check(pal4::XmlAttributeRecord::FreeValue(0x3000u, nonpositive) ==
                  size &&
                  nonpositive.freed_raw.empty() &&
                  nonpositive.words[0x3014u] == size &&
                  nonpositive.words[0x3018u] == 0u,
              "nonpositive type-four size only clears its pointer");
    }

    Runtime other;
    other.words[0x3010u] = 2u;
    other.words[0x3018u] = 0x9000u;
    Check(pal4::XmlAttributeRecord::FreeValue(0x3000u, other) == 2u &&
              other.words[0x3018u] == 0u &&
              other.freed_raw.empty(),
          "other types simply clear the value pointer");
}

void TestDestroyReleasesValueBeforeName() {
    Runtime runtime;
    runtime.words[0x3010u] = 3u;
    runtime.words[0x3018u] = 0x6000u;
    runtime.words[0x6004u] = 0x8000u;
    runtime.bytes[0x7fffu] = 0xffu;
    runtime.words[0x3004u] = 0x4000u;
    runtime.words[0x3008u] = 4u;
    runtime.words[0x300cu] = 31u;
    runtime.bytes[0x3fffu] = 0u;
    Check(pal4::XmlAttributeRecord::Destroy(0x3000u, runtime) ==
              0x40ffu &&
              runtime.freed_blocks ==
                  std::vector<std::uint32_t>{0x7fffu, 0x6000u, 0x3fffu} &&
              runtime.words[0x3018u] == 0u &&
              runtime.words[0x3004u] == 0u &&
              runtime.words[0x3008u] == 0u &&
              runtime.words[0x300cu] == 0u,
          "destructor releases child and parent text in order");
}

void TestDestroySharedAndAbsentName() {
    Runtime shared;
    shared.words[0x3010u] = 1u;
    shared.words[0x3004u] = 0x4000u;
    shared.bytes[0x3fffu] = 3u;
    Check(pal4::XmlAttributeRecord::Destroy(0x3000u, shared) ==
              0x3fffu &&
              shared.bytes[0x3fffu] == 2u &&
              shared.freed_blocks.empty() &&
              shared.words[0x3004u] == 0u,
          "shared parent name decrements its reference byte");

    Runtime absent;
    absent.words[0x3010u] = 1u;
    Check(pal4::XmlAttributeRecord::Destroy(0x3000u, absent) == 0u &&
              absent.freed_blocks.empty(),
          "absent parent name leaves a zero destructor result");
}

void TestValueSizeByType() {
    Runtime runtime;
    for (const auto [type, expected] :
         {std::pair{0u, 1u}, std::pair{1u, 4u}, std::pair{2u, 4u}}) {
        runtime.words[0x3010u] = type;
        Check(pal4::XmlAttributeRecord::ValueSize(0x3000u, runtime) ==
                  expected,
              "scalar value sizes match their type");
    }
    runtime.words[0x3010u] = 3u;
    runtime.words[0x3018u] = 0x6000u;
    runtime.words[0x6008u] = 17u;
    Check(pal4::XmlAttributeRecord::ValueSize(0x3000u, runtime) == 17u,
          "type-three size comes from the child text length");
    runtime.words[0x3010u] = 9u;
    runtime.words[0x3014u] = 0xfffffffeu;
    Check(pal4::XmlAttributeRecord::ValueSize(0x3000u, runtime) ==
              0xfffffffeu,
          "other types return their raw length without interpretation");
}

void TestAssignReleasesOldChild() {
    Runtime runtime;
    PrepareCopySource(runtime, 1u);
    runtime.words[0x317bu + 24u] = 0x12345678u;
    runtime.words[0x3010u] = 3u;
    runtime.words[0x3018u] = 0x6200u;
    runtime.words[0x6204u] = 0x8100u;
    runtime.bytes[0x80ffu] = 0u;
    Check(pal4::XmlAttributeRecord::Assign(
              0x3000u, 0x317bu, runtime) == 0x3000u &&
              runtime.freed_blocks ==
                  std::vector<std::uint32_t>{0x80ffu, 0x6200u} &&
              runtime.words[0x3010u] == 1u &&
              runtime.words[0x3018u] == 0x12345678u &&
              runtime.Text(0x4000u, 3u) == "key" &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "free-block") <
                  std::find(runtime.calls.begin(), runtime.calls.end(),
                            "assign"),
          "assignment releases old child before copying scalar value");
}

void TestAssignRawToChild() {
    Runtime runtime;
    PrepareCopySource(runtime, 3u);
    runtime.words[0x317bu + 24u] = 0x7100u;
    runtime.words[0x7104u] = 0x7200u;
    runtime.bytes[0x7200u] = 'o';
    runtime.bytes[0x7201u] = 'k';
    runtime.bytes[0x7202u] = 0u;
    runtime.words[0x3010u] = 4u;
    runtime.words[0x3014u] = 8u;
    runtime.words[0x3018u] = 0x9000u;
    runtime.expected_child_reallocate_length = 2u;
    Check(pal4::XmlAttributeRecord::Assign(
              0x3000u, 0x317bu, runtime) == 0x3000u &&
              runtime.freed_raw == std::vector<std::uint32_t>{0x9000u} &&
              runtime.words[0x3014u] == 0u &&
              runtime.words[0x3010u] == 3u &&
              runtime.words[0x3018u] == 0x6000u &&
              runtime.Text(0x8000u, 2u) == "ok" &&
              runtime.bytes[0x8002u] == 0u,
          "assignment frees raw storage and deep-copies the child string");
}

void TestAssignFailureAfterOldValueReleased() {
    Runtime runtime;
    PrepareCopySource(runtime, 2u);
    runtime.words[0x3010u] = 4u;
    runtime.words[0x3014u] = 4u;
    runtime.words[0x3018u] = 0x9000u;
    runtime.fail_assign = true;
    bool caught = false;
    try {
        (void)pal4::XmlAttributeRecord::Assign(0x3000u, 0x317bu, runtime);
    } catch (const std::bad_alloc&) {
        caught = true;
    }
    Check(caught && runtime.freed_raw ==
                        std::vector<std::uint32_t>{0x9000u} &&
              runtime.words[0x3014u] == 0u &&
              runtime.words[0x3018u] == 0u &&
              runtime.words[0x3010u] == 2u,
          "assignment failure occurs after old value release and type write");
}

void TestToCharByType() {
    Runtime runtime;
    runtime.bytes[0x7000u] = 'Z';
    runtime.words[0x3010u] = 0u;
    runtime.bytes[0x3018u] = 0x80u;
    Check(pal4::XmlAttributeRecord::ToChar(0x3000u, 0x7000u, runtime) ==
              0x80u,
          "type-zero character conversion returns its stored byte");
    runtime.words[0x3010u] = 1u;
    Check(pal4::XmlAttributeRecord::ToChar(0x3000u, 0x7000u, runtime) ==
              0x80u,
          "type-one character conversion returns the low byte");
    runtime.words[0x3010u] = 2u;
    runtime.words[0x3018u] = std::bit_cast<std::uint32_t>(4.0f);
    Check(pal4::XmlAttributeRecord::ToChar(0x3000u, 0x7000u, runtime) == 4u &&
              runtime.converted_float_bits ==
                  std::bit_cast<std::uint32_t>(4.0f),
          "type-two character conversion uses CRT float-to-int");
    runtime.words[0x3010u] = 3u;
    runtime.words[0x3018u] = 0x6000u;
    runtime.words[0x6004u] = 0x7200u;
    runtime.bytes[0x7200u] = ' ';
    runtime.bytes[0x7201u] = 'a';
    runtime.bytes[0x7202u] = 0u;
    Check(pal4::XmlAttributeRecord::ToChar(0x3000u, 0x7000u, runtime) ==
              ' ',
          "text character scan includes leading whitespace");
    runtime.words[0x6004u] = 0u;
    Check(pal4::XmlAttributeRecord::ToChar(0x3000u, 0x7000u, runtime) ==
              'Z',
          "empty text preserves the supplied character");
    runtime.words[0x3010u] = 4u;
    runtime.words[0x3014u] = 1u;
    runtime.words[0x3018u] = 0x7300u;
    runtime.bytes[0x7300u] = 'R';
    Check(pal4::XmlAttributeRecord::ToChar(0x3000u, 0x7000u, runtime) ==
              'R',
          "positive raw data returns its first byte");
    runtime.words[0x3014u] = 0xffffffffu;
    Check(pal4::XmlAttributeRecord::ToChar(0x3000u, 0x7000u, runtime) ==
              'Z',
          "nonpositive raw size preserves character fallback");
}

void TestToIntByType() {
    Runtime runtime;
    runtime.words[0x7000u] = 0x11223344u;
    runtime.words[0x3010u] = 0u;
    runtime.bytes[0x3018u] = 0x80u;
    Check(pal4::XmlAttributeRecord::ToInt(0x3000u, 0x7000u, runtime) ==
              -128,
          "type-zero integer conversion sign-extends its byte");
    runtime.words[0x3010u] = 1u;
    runtime.words[0x3018u] = 0x87654321u;
    Check(static_cast<std::uint32_t>(pal4::XmlAttributeRecord::ToInt(
              0x3000u, 0x7000u, runtime)) == 0x87654321u,
          "type-one integer conversion returns the complete word");
    runtime.words[0x3010u] = 2u;
    runtime.words[0x3018u] = std::bit_cast<std::uint32_t>(7.0f);
    Check(pal4::XmlAttributeRecord::ToInt(0x3000u, 0x7000u, runtime) == 7,
          "type-two integer conversion uses CRT float-to-int");
    runtime.words[0x3010u] = 3u;
    runtime.words[0x3018u] = 0x6000u;
    runtime.words[0x6004u] = 0x7200u;
    runtime.bytes[0x7200u] = '-';
    runtime.bytes[0x7201u] = '1';
    runtime.bytes[0x7202u] = '7';
    runtime.bytes[0x7203u] = 0u;
    Check(pal4::XmlAttributeRecord::ToInt(0x3000u, 0x7000u, runtime) == -17,
          "text integer conversion scans decimal digits");
    runtime.words[0x6004u] = 0u;
    Check(static_cast<std::uint32_t>(pal4::XmlAttributeRecord::ToInt(
              0x3000u, 0x7000u, runtime)) == 0x11223344u,
          "empty text preserves integer fallback");
    runtime.words[0x3010u] = 4u;
    runtime.words[0x3014u] = 2u;
    runtime.words[0x3018u] = 0x7300u;
    runtime.bytes[0x7300u] = 0xaau;
    runtime.bytes[0x7301u] = 0xbbu;
    Check(static_cast<std::uint32_t>(pal4::XmlAttributeRecord::ToInt(
              0x3000u, 0x7000u, runtime)) == 0x1122bbaau,
          "short raw data overwrites only low fallback bytes");
    runtime.words[0x3014u] = 4u;
    runtime.words[0x7300u] = 0x55667788u;
    Check(static_cast<std::uint32_t>(pal4::XmlAttributeRecord::ToInt(
              0x3000u, 0x7000u, runtime)) == 0x55667788u,
          "four-byte raw data replaces the whole integer");
}

void TestToFloatByType() {
    Runtime runtime;
    runtime.words[0x7000u] = std::bit_cast<std::uint32_t>(1.0f);
    runtime.words[0x3010u] = 0u;
    runtime.bytes[0x3018u] = 0xfeu;
    Check(pal4::XmlAttributeRecord::ToFloat(0x3000u, 0x7000u, runtime) ==
              -2.0,
          "type-zero float conversion sign-extends its byte");
    runtime.words[0x3010u] = 1u;
    runtime.words[0x3018u] = 7u;
    Check(pal4::XmlAttributeRecord::ToFloat(0x3000u, 0x7000u, runtime) ==
              7.0,
          "type-one float conversion uses signed integer value");
    runtime.words[0x3010u] = 2u;
    runtime.words[0x3018u] = std::bit_cast<std::uint32_t>(2.5f);
    Check(pal4::XmlAttributeRecord::ToFloat(0x3000u, 0x7000u, runtime) ==
              2.5,
          "type-two float conversion uses stored IEEE bits");
    runtime.words[0x3010u] = 3u;
    runtime.words[0x3018u] = 0x6000u;
    runtime.words[0x6004u] = 0x7200u;
    runtime.bytes[0x7200u] = '2';
    runtime.bytes[0x7201u] = '.';
    runtime.bytes[0x7202u] = '2';
    runtime.bytes[0x7203u] = '5';
    runtime.bytes[0x7204u] = 0u;
    Check(pal4::XmlAttributeRecord::ToFloat(0x3000u, 0x7000u, runtime) ==
              2.25,
          "type-three float conversion scans text");
    runtime.words[0x6004u] = 0u;
    Check(pal4::XmlAttributeRecord::ToFloat(0x3000u, 0x7000u, runtime) ==
              1.0,
          "empty text preserves float fallback");
    runtime.words[0x3010u] = 4u;
    runtime.words[0x3014u] = 3u;
    runtime.words[0x3018u] = 0x7300u;
    runtime.bytes[0x7300u] = 0u;
    runtime.bytes[0x7301u] = 0u;
    runtime.bytes[0x7302u] = 0u;
    Check(pal4::XmlAttributeRecord::ToFloat(0x3000u, 0x7000u, runtime) ==
              0.5,
          "short raw float data retains the fallback high byte");
    runtime.words[0x3014u] = 4u;
    runtime.words[0x7300u] = std::bit_cast<std::uint32_t>(3.5f);
    Check(pal4::XmlAttributeRecord::ToFloat(0x3000u, 0x7000u, runtime) ==
              3.5,
          "four-byte raw float data replaces the fallback bits");
}

void PutCString(Runtime& runtime, std::uint32_t address,
                std::string_view value) {
    for (std::uint32_t index = 0u; index < value.size(); ++index)
        runtime.bytes[address + index] =
            static_cast<std::uint8_t>(value[index]);
    runtime.bytes[address + static_cast<std::uint32_t>(value.size())] = 0u;
}

void CheckStringResult(Runtime& runtime, std::string_view expected) {
    const auto data = runtime.words[0xb004u];
    Check(runtime.bytes[0xb000u] == 0x7bu &&
              runtime.words[0xb008u] == expected.size() &&
              runtime.words[0xb00cu] == 0u &&
              (expected.empty()
                   ? data == 0u
                   : data == 0xc000u &&
                         runtime.Text(data,
                                      static_cast<std::uint32_t>(expected.size())) ==
                             expected &&
                         runtime.bytes[data +
                                       static_cast<std::uint32_t>(expected.size())] ==
                             0u),
          "string result layout and bytes match expected output");
}

void TestToStringScalarTypes() {
    for (const auto type : {0u, 1u, 2u}) {
        Runtime runtime;
        PutCString(runtime, 0x507bu, "before");
        runtime.words[0x3010u] = type;
        if (type == 0u) runtime.bytes[0x3018u] = 'Q';
        if (type == 1u) runtime.words[0x3018u] =
            static_cast<std::uint32_t>(-42);
        if (type == 2u)
            runtime.words[0x3018u] = std::bit_cast<std::uint32_t>(1.25f);
        Check(pal4::XmlAttributeRecord::ToString(
                  0x3000u, 0xb000u, 0x507bu, runtime) == 0xb000u,
              "scalar string conversion returns output record");
        CheckStringResult(runtime,
                          type == 0u ? "Q" :
                          type == 1u ? "-42" : "1.250000");
    }
    Runtime zero;
    PutCString(zero, 0x507bu, "before");
    zero.words[0x3010u] = 0u;
    zero.bytes[0x3018u] = 0u;
    (void)pal4::XmlAttributeRecord::ToString(
        0x3000u, 0xb000u, 0x507bu, zero);
    CheckStringResult(zero, "");
}

void TestToStringChildAndFallback() {
    Runtime child;
    PutCString(child, 0x507bu, "before");
    PutCString(child, 0x7200u, "dialogue");
    child.words[0x3010u] = 3u;
    child.words[0x3018u] = 0x6000u;
    child.words[0x6004u] = 0x7200u;
    (void)pal4::XmlAttributeRecord::ToString(
        0x3000u, 0xb000u, 0x507bu, child);
    CheckStringResult(child, "dialogue");

    Runtime empty_child;
    PutCString(empty_child, 0x507bu, "before");
    empty_child.words[0x3010u] = 3u;
    empty_child.words[0x3018u] = 0x6000u;
    (void)pal4::XmlAttributeRecord::ToString(
        0x3000u, 0xb000u, 0x507bu, empty_child);
    CheckStringResult(empty_child, "");

    Runtime fallback;
    PutCString(fallback, 0x507bu, "before");
    fallback.words[0x3010u] = 4u;
    fallback.words[0x3014u] = 0xffffffffu;
    (void)pal4::XmlAttributeRecord::ToString(
        0x3000u, 0xb000u, 0x507bu, fallback);
    CheckStringResult(fallback, "before");
}

void TestToStringRawPointerOverwrite() {
    Runtime runtime;
    PutCString(runtime, 0x507bu, "before");
    PutCString(runtime, 0x7300u, "raw");
    runtime.words[0x3010u] = 4u;
    runtime.words[0x3014u] = 4u;
    runtime.words[0x3018u] = 0x9000u;
    runtime.bytes[0x9000u] = 0x00u;
    runtime.bytes[0x9001u] = 0x73u;
    runtime.bytes[0x9002u] = 0x00u;
    runtime.bytes[0x9003u] = 0x00u;
    (void)pal4::XmlAttributeRecord::ToString(
        0x3000u, 0xb000u, 0x507bu, runtime);
    CheckStringResult(runtime, "raw");
    Check(runtime.allocated_child_size == 4u &&
              runtime.freed_blocks == std::vector<std::uint32_t>{0x7300u},
          "raw branch uses overwritten pointer for scan and free");
}

void TestToStringUnsafeRawLength() {
    Runtime runtime;
    PutCString(runtime, 0x507bu, "before");
    runtime.words[0x3010u] = 4u;
    runtime.words[0x3014u] = 5u;
    bool caught = false;
    try {
        (void)pal4::XmlAttributeRecord::ToString(
            0x3000u, 0xb000u, 0x507bu, runtime);
    } catch (const std::length_error&) {
        caught = true;
    }
    Check(caught && runtime.allocated_child_size == 5u &&
              runtime.freed_blocks.empty() &&
              runtime.words.find(0xb004u) == runtime.words.end(),
          "raw copy beyond the four-byte pointer slot fails explicitly");
}

void TestGetScalarValues() {
    Runtime runtime;
    runtime.words[0x3010u] = 0u;
    runtime.bytes[0x3018u] = 0xfau;
    Check(pal4::XmlAttributeRecord::GetValue(
              0x3000u, 0xb000u, 0xa000u, runtime) == 1u &&
              runtime.bytes[0xb000u] == 0xfau,
          "type-zero value copy writes one byte");
    for (const auto type : {1u, 2u}) {
        runtime.words[0x3010u] = type;
        runtime.words[0x3018u] = 0x87654321u;
        Check(pal4::XmlAttributeRecord::GetValue(
                  0x3000u, 0xb000u, 0xa000u, runtime) == 4u &&
                  runtime.words[0xb000u] == 0x87654321u,
              "type-one and type-two value copies write a full word");
    }
}

void TestGetChildTextIgnoresCapacity() {
    Runtime runtime;
    runtime.words[0x3010u] = 3u;
    runtime.words[0x3018u] = 0x6000u;
    runtime.words[0x6004u] = 0x7200u;
    runtime.words[0x6008u] = 3u;
    runtime.words[0xa000u] = 1u;
    PutCString(runtime, 0x7200u, "abc");
    Check(pal4::XmlAttributeRecord::GetValue(
              0x3000u, 0xb000u, 0xa000u, runtime) == 3u &&
              runtime.Text(0xb000u, 3u) == "abc",
          "child text copies its full stored length regardless of capacity");
    runtime.words[0x6004u] = 0u;
    runtime.words[0x6008u] = 0u;
    Check(pal4::XmlAttributeRecord::GetValue(
              0x3000u, 0xb000u, 0xa000u, runtime) == 0u,
          "empty child text returns zero without copying");
}

void TestGetRawValueCapacity() {
    Runtime runtime;
    runtime.words[0x3010u] = 4u;
    runtime.words[0x3014u] = 5u;
    runtime.words[0x3018u] = 0x7300u;
    PutCString(runtime, 0x7300u, "ABCDE");
    runtime.words[0xa000u] = 3u;
    runtime.bytes[0xb003u] = 0x55u;
    Check(pal4::XmlAttributeRecord::GetValue(
              0x3000u, 0xb000u, 0xa000u, runtime) == 3u &&
              runtime.Text(0xb000u, 3u) == "ABC" &&
              runtime.bytes[0xb003u] == 0x55u,
          "raw copy truncates to positive caller capacity");
    runtime.words[0xa000u] = 9u;
    Check(pal4::XmlAttributeRecord::GetValue(
              0x3000u, 0xb000u, 0xa000u, runtime) == 5u &&
              runtime.Text(0xb000u, 5u) == "ABCDE",
          "raw copy returns the stored size when capacity is larger");
    runtime.words[0xa000u] = 0xffffffffu;
    runtime.bytes[0xb000u] = 0x55u;
    Check(pal4::XmlAttributeRecord::GetValue(
              0x3000u, 0xb000u, 0xa000u, runtime) == 0u &&
              runtime.bytes[0xb000u] == 0x55u,
          "nonpositive capacity leaves raw destination untouched");
    runtime.words[0x3014u] = 0u;
    runtime.words[0xa000u] = 9u;
    Check(pal4::XmlAttributeRecord::GetValue(
              0x3000u, 0xb000u, 0xa000u, runtime) == 0u,
          "zero raw size returns without copying");
}

void TestSetCharScalarTypes() {
    for (const auto type : {0u, 1u, 2u}) {
        Runtime runtime;
        runtime.words[0x3010u] = type;
        runtime.bytes[0x7000u] = 0xfeu;
        Check(pal4::XmlAttributeRecord::SetChar(
                  0x3000u, 0x7000u, runtime) == 1u &&
                  (type == 0u
                      ? runtime.bytes[0x3018u] == 0xfeu
                      : type == 1u
                            ? runtime.words[0x3018u] == 0xfffffffeu
                            : runtime.words[0x3018u] ==
                                  std::bit_cast<std::uint32_t>(-2.0f)),
              "character setter preserves signed conversion by target type");
    }
}

void TestSetIntScalarTypes() {
    for (const auto type : {0u, 1u, 2u}) {
        Runtime runtime;
        runtime.words[0x3010u] = type;
        runtime.words[0x7000u] = 0xfffffffdu;
        Check(pal4::XmlAttributeRecord::SetInt(
                  0x3000u, 0x7000u, runtime) == 1u &&
                  (type == 0u
                      ? runtime.bytes[0x3018u] == 0xfdu
                      : type == 1u
                            ? runtime.words[0x3018u] == 0xfffffffdu
                            : runtime.words[0x3018u] ==
                                  std::bit_cast<std::uint32_t>(-3.0f)),
              "integer setter narrows, copies or converts by target type");
    }
}

void TestSetFloatScalarTypes() {
    for (const auto type : {0u, 1u, 2u}) {
        Runtime runtime;
        runtime.words[0x3010u] = type;
        runtime.words[0x7000u] = std::bit_cast<std::uint32_t>(4.0f);
        Check(pal4::XmlAttributeRecord::SetFloat(
                  0x3000u, 0x7000u, runtime) == 1u &&
                  (type == 0u
                      ? runtime.bytes[0x3018u] == 4u
                      : type == 1u
                            ? runtime.words[0x3018u] == 4u
                            : runtime.words[0x3018u] ==
                                  std::bit_cast<std::uint32_t>(4.0f)),
              "float setter converts or copies IEEE bits by target type");
    }
}

void TestSetChildTextFromScalars() {
    Runtime character;
    character.words[0x3010u] = 3u;
    character.words[0x3018u] = 0x6000u;
    character.bytes[0x7000u] = 'A';
    character.expected_child_reallocate_length = 1u;
    Check(pal4::XmlAttributeRecord::SetChar(
              0x3000u, 0x7000u, character) == 1u &&
              character.words[0x6008u] == 1u &&
              character.Text(0x8000u, 1u) == "A" &&
              character.bytes[0x8001u] == 0u &&
              character.calls[0] == "flag" &&
              character.calls[1] == "reallocate",
          "character setter allocates child text for nonzero byte");

    Runtime zero;
    zero.words[0x3010u] = 3u;
    zero.words[0x3018u] = 0x6000u;
    zero.bytes[0x7000u] = 0u;
    Check(pal4::XmlAttributeRecord::SetChar(
              0x3000u, 0x7000u, zero) == 1u &&
              zero.words[0x6004u] == 0u &&
              zero.calls == std::vector<std::string>{"flag"},
          "zero character clears child text without allocation");

    Runtime integer;
    integer.words[0x3010u] = 3u;
    integer.words[0x3018u] = 0x6000u;
    integer.words[0x6004u] = 0x8000u;
    integer.words[0x600cu] = 31u;
    integer.words[0x7000u] = 0xffffffd6u;
    Check(pal4::XmlAttributeRecord::SetInt(
              0x3000u, 0x7000u, integer) == 1u &&
              integer.Text(0x8000u, 3u) == "-42" &&
              integer.words[0x6008u] == 3u &&
              integer.calls.front() == "byte",
          "integer setter reuses an unshared child text buffer");

    Runtime floating;
    floating.words[0x3010u] = 3u;
    floating.words[0x3018u] = 0x6000u;
    floating.words[0x6004u] = 0x8100u;
    floating.bytes[0x80ffu] = 2u;
    floating.words[0x7000u] = std::bit_cast<std::uint32_t>(1.25f);
    floating.expected_child_reallocate_length = 8u;
    Check(pal4::XmlAttributeRecord::SetFloat(
              0x3000u, 0x7000u, floating) == 1u &&
              floating.Text(0x8000u, 8u) == "1.250000" &&
              floating.words[0x6008u] == 8u &&
              floating.calls.front() == "reallocate",
          "float setter detaches shared child text before formatting");
}

void TestSetRawValues() {
    Runtime character;
    character.words[0x3010u] = 4u;
    character.words[0x3014u] = 2u;
    character.words[0x3018u] = 0x9100u;
    character.bytes[0x7000u] = 0xabu;
    Check(pal4::XmlAttributeRecord::SetChar(
              0x3000u, 0x7000u, character) == 1u &&
              character.freed_raw ==
                  std::vector<std::uint32_t>{0x9100u} &&
              character.allocated_raw_size == 1u &&
              character.words[0x3014u] == 1u &&
              character.words[0x3018u] == 0x9000u &&
              character.bytes[0x9000u] == 0xabu,
          "raw character setter frees old data and copies one byte");

    Runtime integer;
    integer.words[0x3010u] = 4u;
    integer.words[0x7000u] = 0x12345678u;
    integer.bytes[0x7000u] = 0x78u;
    integer.bytes[0x7001u] = 0x56u;
    integer.bytes[0x7002u] = 0x34u;
    integer.bytes[0x7003u] = 0x12u;
    Check(pal4::XmlAttributeRecord::SetInt(
              0x3000u, 0x7000u, integer) == 1u &&
              integer.allocated_raw_size == 4u &&
              integer.Text(0x9000u, 4u) ==
                  std::string("\x78\x56\x34\x12", 4u),
          "raw integer setter copies four source bytes");

    Runtime floating;
    floating.words[0x3010u] = 4u;
    floating.words[0x7000u] = std::bit_cast<std::uint32_t>(2.0f);
    floating.bytes[0x7000u] = 0u;
    floating.bytes[0x7001u] = 0u;
    floating.bytes[0x7002u] = 0u;
    floating.bytes[0x7003u] = 0x40u;
    Check(pal4::XmlAttributeRecord::SetFloat(
              0x3000u, 0x7000u, floating) == 1u &&
              floating.allocated_raw_size == 4u &&
              floating.bytes[0x9003u] == 0x40u,
          "raw float setter preserves four IEEE source bytes");
}

void TestSetChildEmptySharedAndOversizedBuffer() {
    Runtime shared;
    shared.words[0x3010u] = 3u;
    shared.words[0x3018u] = 0x6000u;
    shared.words[0x6004u] = 0x8100u;
    shared.words[0x6008u] = 3u;
    shared.words[0x600cu] = 31u;
    shared.bytes[0x80ffu] = 2u;
    shared.bytes[0x7000u] = 0u;
    Check(pal4::XmlAttributeRecord::SetChar(
              0x3000u, 0x7000u, shared) == 1u &&
              shared.bytes[0x80ffu] == 1u &&
              shared.words[0x6004u] == 0u &&
              shared.words[0x6008u] == 0u &&
              shared.words[0x600cu] == 0u &&
              shared.calls == std::vector<std::string>{"byte", "word",
                                                        "word", "word"},
          "empty assignment decrements a shared child text header");

    Runtime oversized;
    oversized.words[0x3010u] = 3u;
    oversized.words[0x3018u] = 0x6000u;
    oversized.words[0x6004u] = 0x8100u;
    oversized.words[0x600cu] = 64u;
    oversized.words[0x7000u] = 7u;
    oversized.expected_child_reallocate_length = 1u;
    Check(pal4::XmlAttributeRecord::SetInt(
              0x3000u, 0x7000u, oversized) == 1u &&
              oversized.calls[0] == "flag" &&
              oversized.calls[1] == "reallocate" &&
              oversized.Text(0x8000u, 1u) == "7",
          "oversized unshared capacity is reset before reallocation");
}

void TestSetRawAllocationFailure() {
    Runtime runtime;
    runtime.words[0x3010u] = 4u;
    runtime.words[0x3014u] = 2u;
    runtime.words[0x3018u] = 0x9100u;
    runtime.bytes[0x7000u] = 'x';
    runtime.fail_raw_allocation = true;
    bool caught = false;
    try {
        (void)pal4::XmlAttributeRecord::SetChar(
            0x3000u, 0x7000u, runtime);
    } catch (const std::bad_alloc&) {
        caught = true;
    }
    Check(caught && runtime.freed_raw ==
                        std::vector<std::uint32_t>{0x9100u} &&
              runtime.words[0x3014u] == 1u &&
              runtime.words[0x3018u] == 0u,
          "raw allocation failure follows old-value release and size write");
}

void TestSetStringScalarAndChildTypes() {
    Runtime character;
    PutCString(character, 0x7000u, "alpha");
    character.words[0x3010u] = 0u;
    Check(pal4::XmlAttributeRecord::SetString(
              0x3000u, 0x7000u, character) == 1u &&
              character.bytes[0x3018u] == 'a',
          "string setter uses first character for type zero");

    Runtime integer;
    PutCString(integer, 0x7000u, " -12tail");
    integer.words[0x3010u] = 1u;
    Check(pal4::XmlAttributeRecord::SetString(
              0x3000u, 0x7000u, integer) == 1u &&
              integer.words[0x3018u] == 0xfffffff4u,
          "string setter parses signed decimal integer");

    Runtime floating;
    PutCString(floating, 0x7000u, "2.75tail");
    floating.words[0x3010u] = 2u;
    Check(pal4::XmlAttributeRecord::SetString(
              0x3000u, 0x7000u, floating) == 1u &&
              floating.words[0x3018u] ==
                  std::bit_cast<std::uint32_t>(2.75f),
          "string setter parses float text into IEEE bits");

    Runtime child;
    PutCString(child, 0x7000u, "hello");
    child.words[0x3010u] = 3u;
    child.words[0x3018u] = 0x6000u;
    child.expected_child_reallocate_length = 5u;
    Check(pal4::XmlAttributeRecord::SetString(
              0x3000u, 0x7000u, child) == 1u &&
              child.Text(0x8000u, 5u) == "hello" &&
              child.words[0x6008u] == 5u &&
              child.bytes[0x8005u] == 0u,
          "string setter copies type-three child text");
}

void TestSetStringRawValue() {
    Runtime runtime;
    PutCString(runtime, 0x7000u, "hello");
    runtime.words[0x3010u] = 4u;
    runtime.words[0x3014u] = 2u;
    runtime.words[0x3018u] = 0x9100u;
    runtime.bytes[0x9005u] = 0x55u;
    Check(pal4::XmlAttributeRecord::SetString(
              0x3000u, 0x7000u, runtime) == 1u &&
              runtime.freed_raw ==
                  std::vector<std::uint32_t>{0x9100u} &&
              runtime.allocated_raw_size == 5u &&
              runtime.words[0x3014u] == 5u &&
              runtime.words[0x3018u] == 0x9000u &&
              runtime.Text(0x9000u, 5u) == "hello" &&
              runtime.bytes[0x9005u] == 0x55u,
          "raw string copy excludes the terminating NUL");
}

void TestSetStringNullAndEmptyRawValues() {
    Runtime missing;
    missing.words[0x3010u] = 4u;
    missing.words[0x3014u] = 4u;
    missing.words[0x3018u] = 0x9100u;
    Check(pal4::XmlAttributeRecord::SetString(
              0x3000u, 0u, missing) == 1u &&
              missing.freed_raw ==
                  std::vector<std::uint32_t>{0x9100u} &&
              missing.words[0x3014u] == 0u &&
              missing.words[0x3018u] == 0u &&
              missing.allocated_raw_size == 0u,
          "null raw source only releases old positive-size storage");

    Runtime empty;
    PutCString(empty, 0x7000u, "");
    empty.words[0x3010u] = 4u;
    Check(pal4::XmlAttributeRecord::SetString(
              0x3000u, 0x7000u, empty) == 1u &&
              empty.words[0x3014u] == 0u &&
              empty.words[0x3018u] == 0x9000u &&
              empty.allocated_raw_size == 0u,
          "empty raw string still calls zero-byte allocator");

    Runtime invalid;
    invalid.words[0x3010u] = 3u;
    bool caught = false;
    try {
        (void)pal4::XmlAttributeRecord::SetString(
            0x3000u, 0u, invalid);
    } catch (const std::invalid_argument&) {
        caught = true;
    }
    Check(caught && invalid.calls.empty(),
          "null text source fails before child mutation");
}

void TestSetStringRawAllocationFailure() {
    Runtime runtime;
    PutCString(runtime, 0x7000u, "value");
    runtime.words[0x3010u] = 4u;
    runtime.words[0x3014u] = 3u;
    runtime.words[0x3018u] = 0x9100u;
    runtime.fail_raw_allocation = true;
    bool caught = false;
    try {
        (void)pal4::XmlAttributeRecord::SetString(
            0x3000u, 0x7000u, runtime);
    } catch (const std::bad_alloc&) {
        caught = true;
    }
    Check(caught && runtime.freed_raw ==
                        std::vector<std::uint32_t>{0x9100u} &&
              runtime.words[0x3014u] == 5u &&
              runtime.words[0x3018u] == 0u,
          "raw string allocation failure follows old-value release");
}

void TestSetValueScalarAndChildTypes() {
    Runtime character;
    character.words[0x3010u] = 0u;
    character.words[0xa000u] = 1u;
    character.bytes[0x7000u] = 0xe1u;
    Check(pal4::XmlAttributeRecord::SetValue(
              0x3000u, 0x7000u, 0xa000u, character) == 1u &&
              character.bytes[0x3018u] == 0xe1u,
          "type-zero set-value copies a source byte");

    for (const auto type : {1u, 2u}) {
        Runtime word;
        word.words[0x3010u] = type;
        word.words[0xa000u] = 1u;
        word.words[0x7000u] = 0x87654321u;
        Check(pal4::XmlAttributeRecord::SetValue(
                  0x3000u, 0x7000u, 0xa000u, word) == 1u &&
                  word.words[0x3018u] == 0x87654321u,
              "type-one/two set-value reads a word even when size is one");
    }

    Runtime child;
    child.words[0x3010u] = 3u;
    child.words[0x3018u] = 0x6000u;
    child.words[0xa000u] = 1u;
    child.expected_child_reallocate_length = 5u;
    PutCString(child, 0x7000u, "hello");
    Check(pal4::XmlAttributeRecord::SetValue(
              0x3000u, 0x7000u, 0xa000u, child) == 1u &&
              child.words[0x6008u] == 5u &&
              child.Text(0x8000u, 5u) == "hello",
          "type-three set-value uses full C-string length after size gate");
    child.words[0xa000u] = 0u;
    child.calls.clear();
    Check(pal4::XmlAttributeRecord::SetValue(
              0x3000u, 0x7000u, 0xa000u, child) == 0u &&
              child.calls.empty() &&
              child.words[0x6008u] == 5u,
          "nonpositive size leaves scalar and child values unchanged");
}

void TestSetValueRawReturnAndCleanup() {
    Runtime runtime;
    runtime.words[0x3010u] = 4u;
    runtime.words[0x3014u] = 5u;
    runtime.words[0x3018u] = 0x9100u;
    runtime.words[0xa000u] = 3u;
    runtime.bytes[0x7000u] = 1u;
    runtime.bytes[0x7001u] = 2u;
    runtime.bytes[0x7002u] = 3u;
    Check(pal4::XmlAttributeRecord::SetValue(
              0x3000u, 0x7000u, 0xa000u, runtime) == 0u &&
              runtime.freed_raw ==
                  std::vector<std::uint32_t>{0x9100u} &&
              runtime.allocated_raw_size == 3u &&
              runtime.words[0x3014u] == 3u &&
              runtime.words[0x3018u] == 0x9000u &&
              runtime.bytes[0x9000u] == 1u &&
              runtime.bytes[0x9001u] == 2u &&
              runtime.bytes[0x9002u] == 3u,
          "successful raw set-value still returns zero");

    runtime.words[0xa000u] = 0xffffffffu;
    Check(pal4::XmlAttributeRecord::SetValue(
              0x3000u, 0x7000u, 0xa000u, runtime) == 0u &&
              runtime.freed_raw ==
                  (std::vector<std::uint32_t>{0x9100u, 0x9000u}) &&
              runtime.words[0x3014u] == 0u &&
              runtime.words[0x3018u] == 0u,
          "nonpositive raw size releases an old positive-size payload");
}

void TestSetValueRawFailure() {
    Runtime runtime;
    runtime.words[0x3010u] = 4u;
    runtime.words[0x3014u] = 2u;
    runtime.words[0x3018u] = 0x9100u;
    runtime.words[0xa000u] = 4u;
    runtime.fail_raw_allocation = true;
    bool caught = false;
    try {
        (void)pal4::XmlAttributeRecord::SetValue(
            0x3000u, 0x7000u, 0xa000u, runtime);
    } catch (const std::bad_alloc&) {
        caught = true;
    }
    Check(caught && runtime.freed_raw ==
                        std::vector<std::uint32_t>{0x9100u} &&
              runtime.words[0x3014u] == 4u &&
              runtime.words[0x3018u] == 0u,
          "raw allocation failure follows old-value release and size write");
}

void TestGetTypeName() {
    const std::vector<std::pair<std::string, std::uint32_t>> known{
        {"XML_CHAR", 0u}, {"XML_INT", 1u}, {"XML_FLOAT", 2u},
        {"XML_STRING", 3u}, {"XML_BINARY", 4u}};
    for (const auto& [name, type] : known) {
        Runtime runtime;
        runtime.words[0x3004u] = 0x4000u;
        runtime.words[0x3008u] = static_cast<std::uint32_t>(name.size());
        for (std::uint32_t index = 0u; index < name.size(); ++index)
            runtime.bytes[0x4000u + index] =
                static_cast<std::uint8_t>(name[index]);
        Check(pal4::XmlAttributeRecord::GetTypeName(0x3000u, runtime) == type &&
                  runtime.calls.empty(),
              "known XML type name maps to its native ordinal without writes");
    }

    for (const std::string name : {
             "XML", "XML_CHAR_EXTRA", "xml_int", "XML_FLOAT ",
             "XML_STRIN", "OTHER"}) {
        Runtime runtime;
        runtime.words[0x3004u] = 0x4000u;
        runtime.words[0x3008u] = static_cast<std::uint32_t>(name.size());
        for (std::uint32_t index = 0u; index < name.size(); ++index)
            runtime.bytes[0x4000u + index] =
                static_cast<std::uint8_t>(name[index]);
        Check(pal4::XmlAttributeRecord::GetTypeName(0x3000u, runtime) ==
                  0x4000u,
              "unrecognized type preserves its source text address");
    }

    Runtime explicit_length;
    explicit_length.words[0x3004u] = 0x4000u;
    explicit_length.words[0x3008u] = 7u;
    const std::string text = "XML_CHAR";
    for (std::uint32_t index = 0u; index < text.size(); ++index)
        explicit_length.bytes[0x4000u + index] =
            static_cast<std::uint8_t>(text[index]);
    Check(pal4::XmlAttributeRecord::GetTypeName(
              0x3000u, explicit_length) == 0x4000u,
          "explicit short length prevents a prefix from matching");
    explicit_length.words[0x3008u] = 8u;
    Check(pal4::XmlAttributeRecord::GetTypeName(
              0x3000u, explicit_length) == 0u,
          "exact length recognizes the same stored bytes");
}

}  // namespace

int main() {
    TestDefaultAttributeLayout();
    TestAllocationFailure();
    TestNamedAttributeWithChild();
    TestOtherTypeAndEmptyName();
    TestNullNameThrowsAfterInitialFields();
    TestNameAllocationFailureStillCreatesChild();
    TestChildAllocationFailure();
    TestPointerValueConstructors();
    TestEmptyPointerValueName();
    TestNullPointerValueName();
    TestTypeThreeTextValue();
    TestEmptyTypeThreeTextValue();
    TestTypeThreeNameAllocationFailure();
    TestTypeThreeChildAllocationFailure();
    TestTypeThreeNullName();
    TestCopyScalarRecords();
    TestCopyChildText();
    TestCopyEmptyChildText();
    TestCopyRawValue();
    TestCopyNonpositiveRawLength();
    TestCopyFailures();
    TestFreeTypeThreeValue();
    TestFreeSharedChildText();
    TestFreeRawAndOtherValues();
    TestDestroyReleasesValueBeforeName();
    TestDestroySharedAndAbsentName();
    TestValueSizeByType();
    TestAssignReleasesOldChild();
    TestAssignRawToChild();
    TestAssignFailureAfterOldValueReleased();
    TestToCharByType();
    TestToIntByType();
    TestToFloatByType();
    TestToStringScalarTypes();
    TestToStringChildAndFallback();
    TestToStringRawPointerOverwrite();
    TestToStringUnsafeRawLength();
    TestGetScalarValues();
    TestGetChildTextIgnoresCapacity();
    TestGetRawValueCapacity();
    TestSetCharScalarTypes();
    TestSetIntScalarTypes();
    TestSetFloatScalarTypes();
    TestSetChildTextFromScalars();
    TestSetRawValues();
    TestSetChildEmptySharedAndOversizedBuffer();
    TestSetRawAllocationFailure();
    TestSetStringScalarAndChildTypes();
    TestSetStringRawValue();
    TestSetStringNullAndEmptyRawValues();
    TestSetStringRawAllocationFailure();
    TestSetValueScalarAndChildTypes();
    TestSetValueRawReturnAndCleanup();
    TestSetValueRawFailure();
    TestGetTypeName();
}
