#include "pal4/text_collection_record.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::ITextCollectionRecordRuntime {
public:
    std::vector<std::string> events;
    pal4::TextCollectionRecord record{};
    std::uint32_t allocation = 0x7000u;
    bool throw_on_init = false;

    std::uint32_t Allocate(const std::uint32_t size) override {
        events.emplace_back("allocate");
        Check(size == 24u, "factory allocates one record");
        return allocation;
    }
    pal4::TextCollectionRecord& Access(
        const std::uint32_t address) override {
        events.emplace_back("access");
        Check(address == 0x7000u, "record address");
        return record;
    }
    void InitializeEmpty(pal4::StringBufferState& text) override {
        events.emplace_back("initialize-empty");
        if (throw_on_init) {
            throw std::runtime_error("init failed");
        }
        text = {0x9000u, 0u, 32u};
    }
    void AssignText(pal4::StringBufferState& text,
                    const std::uint32_t source,
                    const std::uint32_t length) override {
        events.emplace_back("assign-text");
        Check(source == 0x1234u && length == 5u,
              "copy uses source string pointer and length");
        text = {0x9876u, length, 32u};
    }
    void CleanupText(pal4::StringBufferState& text) override {
        events.emplace_back("cleanup-text");
        Check(text.data == 0x9000u, "cleanup sees owned text");
        text.data = 0;
    }
    std::uint32_t Free(const std::uint32_t address) override {
        events.emplace_back("free");
        Check(address == 0x7000u, "free same address");
        return 0xbeefu;
    }
};

void TestCopyRecord() {
    Runtime runtime;
    pal4::TextCollectionRecord source;
    source.text = {0x1234u, 5u, 16u};
    source.attributes = {7u, 8u, 9u};
    pal4::TextCollectionRecord destination;
    destination.attributes = {11u, 12u, 13u};
    Check(pal4::TextCollectionRecordLifecycle::InitializeFromRecord(
              destination, source, runtime) == 8u &&
              destination.text.data == 0x9876u &&
              destination.attributes ==
                  std::array<std::uint32_t, 3>{7u, 8u, 13u} &&
              runtime.events == std::vector<std::string>{"assign-text"},
          "copy initializes text and first two attributes only");
}

void TestFactoryAndExceptionCleanup() {
    Runtime runtime;
    runtime.allocation = 0u;
    Check(pal4::TextCollectionRecordLifecycle::Create(runtime) == 0u &&
              runtime.events == std::vector<std::string>{"allocate"},
          "allocation failure returns null before record access");

    runtime.events.clear();
    runtime.allocation = 0x7000u;
    runtime.record.attributes = {11u, 12u, 13u};
    Check(pal4::TextCollectionRecordLifecycle::Create(runtime) == 0x7000u &&
              runtime.record.text.data == 0x9000u &&
              runtime.record.attributes ==
                  std::array<std::uint32_t, 3>{0u, 0u, 13u} &&
              runtime.events ==
                  std::vector<std::string>{"allocate", "access",
                                           "initialize-empty"},
          "factory initializes text and two observed attributes");

    runtime.events.clear();
    runtime.throw_on_init = true;
    bool threw = false;
    try {
        (void)pal4::TextCollectionRecordLifecycle::Create(runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.events ==
                       std::vector<std::string>{"allocate", "access",
                                                "initialize-empty", "free"},
          "constructor failure releases factory allocation");
}

void TestDestroy() {
    Runtime runtime;
    Check(!pal4::TextCollectionRecordLifecycle::Destroy(0u, runtime)
               .has_value() &&
              runtime.events.empty(),
          "null destruction performs no calls and has no defined result");
    runtime.record.text.data = 0x9000u;
    const auto result =
        pal4::TextCollectionRecordLifecycle::Destroy(0x7000u, runtime);
    Check(result.has_value() && *result == 0xbeefu &&
              runtime.record.text.data == 0u &&
              runtime.events ==
                  std::vector<std::string>{"access", "cleanup-text", "free"},
          "destructor cleans text before freeing record");
}

}  // namespace

int main() {
    TestCopyRecord();
    TestFactoryAndExceptionCleanup();
    TestDestroy();
    return EXIT_SUCCESS;
}
