#include "pal4/singleton_manager.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class RegistrationRuntime final : public pal4::ISingletonRegistrationRuntime {
public:
    void ThrowRange() override {
        throw std::out_of_range("registration name range");
    }

    std::string FormatTypeLabel(const char* type_name,
                                std::uint32_t factory,
                                std::uint32_t type_info) override {
        Check(std::string(type_name) == "GameType" &&
                  factory == 0x1234u && type_info == 0x5678u,
              "formatter receives the original three operands");
        events.emplace_back("format");
        return "typeName( GameType ) - 0x1234 - 0x5678";
    }
    void SetErrorFlag(pal4::SingletonRegistrationEntry& entry,
                      std::uint8_t flag) override {
        Check(flag == 0u && entry.text == 0u && entry.singleton_id == 0u,
              "temporary entry starts empty and clears error before ID write");
        events.emplace_back("error");
    }
    bool AllocateText(pal4::SingletonRegistrationEntry& entry,
                      std::uint32_t length, std::uint8_t mode) override {
        Check(entry.singleton_id == 7u &&
                  length == expected_label.size() && mode == 1u,
              "allocation receives label length, mode, and stored ID");
        events.emplace_back("allocate");
        if (!allocate_succeeds) {
            return false;
        }
        entry.text = 0x2001u;
        entry.text_capacity = length + 1u;
        return true;
    }
    void CopyText(std::uint32_t destination,
                  std::string_view text) override {
        Check(destination == 0x2001u && text == expected_label,
              "copy receives allocated pointer and full formatted label");
        events.emplace_back("copy");
        Put(destination, text);
    }
    void SetTextSize(pal4::SingletonRegistrationEntry& entry,
                     std::uint32_t length) override {
        Check(entry.text == 0x2001u && length == expected_label.size(),
              "size setter follows text copy");
        events.emplace_back("size");
        entry.text_length = length;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        events.emplace_back("read:" + std::to_string(address));
        return words.at(address);
    }
    std::uint32_t Create(std::uint32_t factory) override {
        Check(factory == 0x1234u, "factory dispatch uses original address");
        events.emplace_back("create");
        return 0x9000u;
    }
    void Insert(std::uint32_t end, std::uint32_t count,
                const pal4::SingletonRegistrationEntry& entry) override {
        Check(end == expected_end && count == 1u &&
                  entry.singleton_id == 7u && entry.instance == 0x9000u &&
                  entry.type_info == 0x5678u &&
                  entry.text == (allocate_succeeds ? 0x2001u : 0u),
              "insert receives end, one entry, factory result, and RTTI");
        events.emplace_back("insert");
        if (throw_on_insert) {
            throw std::runtime_error("insert failed");
        }
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        if (address == 0x2000u) {
            events.emplace_back("ref-read");
            return reference_byte;
        }
        events.emplace_back("compare-read:" + std::to_string(address));
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == 0x2000u, "reference update targets text header");
        events.emplace_back("ref-write");
        reference_byte = value;
    }
    void FreeText(std::uint32_t address) override {
        Check(address == 0x2000u, "free targets the header, not text data");
        events.emplace_back("free");
    }
    void Put(std::uint32_t address, std::string_view text) {
        for (std::size_t index = 0u; index < text.size(); ++index) {
            bytes[address + static_cast<std::uint32_t>(index)] =
                static_cast<std::uint8_t>(text[index]);
        }
    }

    std::string expected_label =
        "typeName( GameType ) - 0x1234 - 0x5678";
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;
    std::uint32_t expected_end = 0u;
    std::uint8_t reference_byte = 1u;
    bool allocate_succeeds = true;
    bool throw_on_insert = false;
};

class CollectionCleanupRuntime final :
    public pal4::ISingletonCollectionCleanupRuntime {
public:
    void ReleaseTextRecord(std::uint32_t element) override {
        events.emplace_back("release-text:" + std::to_string(element));
    }
    void Free(std::uint32_t address) override {
        events.emplace_back("free:" + std::to_string(address));
    }
    std::vector<std::string> events;
};

void CheckMatchAndScan() {
    pal4::SingletonManagerState state{0u, 0x1000u, 0x1054u, 0x1100u};
    pal4::SingletonManager manager(state);
    RegistrationRuntime runtime;
    runtime.words[0x1010u] = 4u;
    runtime.words[0x102cu] = 7u;
    runtime.words[0x1020u] = 0x3000u;
    runtime.words[0x1024u] = 8u;
    runtime.words[0x1048u] = 7u;
    runtime.words[0x103cu] = 0x3100u;
    runtime.words[0x1040u] =
        static_cast<std::uint32_t>(runtime.expected_label.size());
    runtime.words[0x104cu] = 0xaaaau;
    runtime.Put(0x3000u, "notmatch");
    runtime.Put(0x3100u, runtime.expected_label);
    Check(manager.Register("GameType", 7u, 0x1234u, 0x5678u,
                           runtime) == 0xaaaau,
          "same ID and matching label returns existing instance");
    Check(runtime.events[5] == "read:4112" &&
              runtime.events[6] == "read:4140" &&
              runtime.events[7] == "read:4128" &&
              runtime.events[8] == "read:4132" &&
              runtime.events[9] == "compare-read:12288",
          "different ID skips compare and different label continues scan");
    Check(runtime.events.back() == "ref-write" &&
              runtime.reference_byte == 0u,
          "ordinary reference byte decrements after match");
}

void CheckCreateAndInsert() {
    pal4::SingletonManagerState state{0u, 0x1000u, 0x101cu, 0x1100u};
    pal4::SingletonManager manager(state);
    RegistrationRuntime runtime;
    runtime.words[0x1010u] = 7u;
    runtime.words[0x1004u] = 0x3000u;
    runtime.words[0x1008u] = 6u;
    runtime.Put(0x3000u, "other!");
    runtime.expected_end = state.end;
    runtime.reference_byte = 0u;
    Check(manager.Register("GameType", 7u, 0x1234u, 0x5678u,
                           runtime) == 0x9000u,
          "same ID but different name creates and inserts");
    Check(runtime.events[runtime.events.size() - 3u] == "insert" &&
              runtime.events.back() == "free" &&
              state.end == 0x101cu,
          "factory precedes insertion and zero reference frees header");
}

void CheckEmptyAndFailedAllocation() {
    pal4::SingletonManagerState state{0u, 0x1000u, 0x101cu, 0x1100u};
    pal4::SingletonManager manager(state);
    RegistrationRuntime runtime;
    runtime.allocate_succeeds = false;
    runtime.words[0x1010u] = 7u;
    runtime.words[0x1004u] = 0u;
    runtime.words[0x1008u] = 0u;
    runtime.words[0x1014u] = 0xbeefu;
    Check(manager.Register("GameType", 7u, 0x1234u, 0x5678u,
                           runtime) == 0xbeefu &&
              runtime.events[3] == "read:4112" &&
              runtime.events[4] == "read:4100" &&
              runtime.events[5] == "read:4104" &&
              runtime.events.back() == "read:4116",
          "allocation failure matches zero-length name and skips release");
}

void CheckEmptyCollectionAndReferenceFF() {
    pal4::SingletonManagerState state{0u, 0x1000u, 0x1000u, 0x1100u};
    pal4::SingletonManager manager(state);
    RegistrationRuntime runtime;
    runtime.expected_end = state.end;
    runtime.reference_byte = 0xffu;
    Check(manager.Register("GameType", 7u, 0x1234u, 0x5678u,
                           runtime) == 0x9000u &&
              runtime.events[5] == "create" &&
              runtime.events.back() == "free",
          "empty range inserts and ff reference frees header");
}

void CheckExceptionCleanup() {
    pal4::SingletonManagerState state{0u, 0x1000u, 0x1000u, 0x1100u};
    pal4::SingletonManager manager(state);
    RegistrationRuntime runtime;
    runtime.expected_end = state.end;
    runtime.throw_on_insert = true;
    runtime.reference_byte = 3u;
    bool threw = false;
    try {
        (void)manager.Register("GameType", 7u, 0x1234u, 0x5678u,
                               runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.reference_byte == 2u &&
              runtime.events.back() == "ref-write",
          "exception releases temporary text and propagates failure");
}

void CheckCollectionCleanup() {
    pal4::SingletonManagerState state{0x77u, 0x1000u, 0x1038u,
                                      0x1100u};
    pal4::SingletonManager manager(state);
    CollectionCleanupRuntime runtime;
    Check(manager.CleanupCollection(runtime) == 0u &&
              runtime.events == std::vector<std::string>{
                  "release-text:4096", "release-text:4124", "free:4096"} &&
              state.first_word == 0x77u && state.begin == 0u &&
              state.end == 0u && state.capacity == 0u,
          "collection cleanup removes each entry, frees array, clears pointers");

    runtime.events.clear();
    Check(manager.CleanupCollection(runtime) == 0u &&
              runtime.events == std::vector<std::string>{"free:0"},
          "empty collection still calls free with null begin");
}

}  // namespace

int main() {
    CheckMatchAndScan();
    CheckCreateAndInsert();
    CheckEmptyAndFailedAllocation();
    CheckEmptyCollectionAndReferenceFF();
    CheckExceptionCleanup();
    CheckCollectionCleanup();
    std::cout << "6 singleton registration and cleanup test cases passed\n";
}
