#include "pal4/inventory_archive.h"

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

struct Runtime final : pal4::IInventoryArchiveRuntime,
                       pal4::IInventorySlotStreamRuntime {
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint32_t> words;
    std::uint32_t attribute = 0x7000u;
    bool valid_flag = true;
    bool valid_document = true;
    bool fail_slots = false;
    bool fail_document_cleanup = false;
    std::vector<std::uint32_t> stream_words;
    std::size_t stream_read = 0u;

    std::uint32_t CreateFlag(const char* text) override {
        calls.emplace_back(std::string("create flag:") + text);
        return 0x6000u;
    }
    void ReleaseFlag(std::uint32_t flag) override {
        Check(flag == 0x6000u, "flag handle");
        calls.emplace_back("release flag");
    }
    void WriteFlag(std::uint32_t stream,
                   std::uint32_t flag) override {
        Check(stream == 0x4000u && flag == 0x6000u,
              "write flag arguments");
        calls.emplace_back("write flag");
    }
    bool ReadFlag(std::uint32_t stream, std::uint32_t flag,
                  const char* expected) override {
        Check(stream == 0x4000u && flag == 0x6000u &&
                  std::string(expected) == "IY_SERIALIZE_FLAG",
              "read flag arguments");
        calls.emplace_back("read flag");
        return valid_flag;
    }
    void MarkFlagError(std::uint32_t flag) override {
        Check(flag == 0x6000u, "flag error handle");
        calls.emplace_back("mark error");
    }
    std::uint32_t CreateDocument(const char* type) override {
        Check(std::string(type) == "INVENTORY", "inventory document type");
        calls.emplace_back("create document");
        return 0x6100u;
    }
    void DestroyDocument(std::uint32_t document) override {
        Check(document == 0x6100u, "document handle");
        calls.emplace_back("destroy document");
        if (fail_document_cleanup)
            throw std::runtime_error("document cleanup");
    }
    std::uint32_t FindAttribute(std::uint32_t document,
                                 const char* name) override {
        Check(document == 0x6100u && std::string(name) == "money",
              "money attribute lookup");
        calls.emplace_back("find money");
        return attribute;
    }
    void SetAttributeFromField(std::uint32_t found,
                               std::uint32_t field) override {
        Check(found == 0x7000u && field == 0x5134u,
              "existing money attribute and field");
        calls.emplace_back("set money");
    }
    void AddAttributeFromField(std::uint32_t document,
                               const char* name,
                               std::uint32_t field) override {
        Check(document == 0x6100u && std::string(name) == "money" &&
                  field == 0x5134u,
              "new money attribute and field");
        calls.emplace_back("add money");
    }
    std::uint32_t ReadAttributeInt(std::uint32_t found) override {
        Check(found == 0x7000u, "read existing money attribute");
        calls.emplace_back("read money");
        return 0x12345678u;
    }
    void WriteDocument(std::uint32_t document,
                       std::uint32_t stream) override {
        Check(document == 0x6100u && stream == 0x4000u,
              "write document arguments");
        calls.emplace_back("write document");
    }
    bool ReadDocument(std::uint32_t document,
                      std::uint32_t stream) override {
        Check(document == 0x6100u && stream == 0x4000u,
              "read document arguments");
        calls.emplace_back("read document");
        return valid_document;
    }
    pal4::IInventorySlotStreamRuntime& ArrayRuntime() override {
        calls.emplace_back("array runtime");
        return *this;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::uint32_t AllocateMemory(std::uint32_t) override {
        throw std::runtime_error("unexpected array growth");
    }
    void FreeMemory(std::uint32_t) override {
        throw std::runtime_error("unexpected array free");
    }
    void WriteStreamWord(std::uint32_t stream,
                         std::uint32_t value) override {
        Check(stream == 0x4000u, "array stream address");
        calls.emplace_back("write stream word");
        stream_words.push_back(value);
    }
    std::uint32_t ReadStreamWord(std::uint32_t stream) override {
        Check(stream == 0x4000u && stream_read < stream_words.size(),
              "array stream read");
        calls.emplace_back("read stream word");
        return stream_words[stream_read++];
    }
    void SerializeSlots(std::uint32_t slots,
                        std::uint32_t stream) override {
        Check(slots == 0x507cu && stream == 0x4000u,
              "serialize slots offset");
        calls.emplace_back("serialize slots");
        if (fail_slots) throw std::runtime_error("slots");
    }
    std::uint8_t SerializeBase(std::uint32_t object,
                                std::uint32_t stream) override {
        Check(object == 0x5000u && stream == 0x4000u,
              "serialize base arguments");
        calls.emplace_back("serialize base");
        return 0x81u;
    }
    void DeserializeSlots(std::uint32_t slots,
                          std::uint32_t stream) override {
        Check(slots == 0x507cu && stream == 0x4000u,
              "deserialize slots offset");
        calls.emplace_back("deserialize slots");
        if (fail_slots) throw std::runtime_error("slots");
    }
    std::uint8_t DeserializeBase(std::uint32_t object,
                                  std::uint32_t stream) override {
        Check(object == 0x5000u && stream == 0x4000u,
              "deserialize base arguments");
        calls.emplace_back("deserialize base");
        return 0x95u;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write word");
    }
    void ResetSlots(std::uint32_t slots) override {
        Check(slots == 0x507cu, "reset slots offset");
        calls.emplace_back("reset slots");
    }
    void ResetArray(std::uint32_t array) override {
        Check(array == 0x5104u, "reset array offset");
        calls.emplace_back("reset array");
    }
    std::uint32_t ResetBase(std::uint32_t object) override {
        Check(object == 0x5000u, "reset base address");
        calls.emplace_back("reset base");
        return 0xbeefu;
    }
};

void CheckSerialize() {
    Runtime runtime;
    runtime.words[0x5108u] = 0x9000u;
    runtime.words[0x510cu] = 0x9008u;
    runtime.words[0x9000u] = 11u;
    runtime.words[0x9004u] = 22u;
    Check(pal4::InventoryArchive::Serialize(0x5000u, 0x4000u,
                                            runtime) == 0x81u &&
              runtime.stream_words ==
                  std::vector<std::uint32_t>{2u, 11u, 22u, 0u, 0u} &&
              runtime.calls == std::vector<std::string>{
                  "create flag:IY_SERIALIZE_FLAG", "write flag",
                  "create document", "find money", "set money",
                  "write document", "array runtime",
                  "write stream word", "write stream word",
                  "write stream word", "write stream word",
                  "write stream word", "serialize slots",
                  "serialize base", "destroy document", "release flag"},
          "writer updates existing money and serializes three regions");
    Runtime missing;
    missing.attribute = 0u;
    Check(pal4::InventoryArchive::Serialize(0x5000u, 0x4000u,
                                            missing) == 0x81u &&
              missing.calls[4] == "add money",
          "writer adds missing money attribute");
}

void CheckDeserialize() {
    Runtime mismatch;
    mismatch.valid_flag = false;
    Check(pal4::InventoryArchive::Deserialize(0x5000u, 0x4000u,
                                              mismatch) == 0u &&
              mismatch.calls == std::vector<std::string>{
                  "create flag:", "read flag", "mark error",
                  "release flag"},
          "flag mismatch stops before resetting inventory");

    Runtime bad_document;
    bad_document.valid_document = false;
    Check(pal4::InventoryArchive::Deserialize(0x5000u, 0x4000u,
                                              bad_document) == 0u &&
              bad_document.calls == std::vector<std::string>{
                  "create flag:", "read flag", "write word",
                  "reset slots", "write word", "reset array", "reset base",
                  "write word", "create document", "read document",
                  "destroy document", "release flag"},
          "invalid document returns after reset and cleans temporaries");

    Runtime valid;
    valid.words[0x5108u] = 0x9000u;
    valid.words[0x5110u] = 0x9008u;
    valid.stream_words = {2u, 11u, 22u, 0u, 0u};
    Check(pal4::InventoryArchive::Deserialize(0x5000u, 0x4000u,
                                              valid) == 0x95u &&
              valid.words[0x5134u] == 0x12345678u &&
              valid.words[0x9000u] == 11u &&
              valid.words[0x9004u] == 22u &&
              valid.words[0x510cu] == 0x9008u &&
              valid.stream_read == 5u &&
              valid.calls == std::vector<std::string>{
                  "create flag:", "read flag", "write word",
                  "reset slots", "write word", "reset array", "reset base",
                  "write word", "create document", "read document",
                  "find money", "read money", "write word",
                  "array runtime", "write word", "read stream word",
                  "read stream word", "write word", "write word",
                  "read stream word", "write word", "write word",
                  "write word", "read stream word", "write word",
                  "read stream word",
                  "deserialize slots",
                  "deserialize base", "destroy document", "release flag"},
          "reader restores money and three regions in order");
    Runtime no_money;
    no_money.attribute = 0u;
    no_money.stream_words = {0u, 0u, 0u};
    Check(pal4::InventoryArchive::Deserialize(0x5000u, 0x4000u,
                                              no_money) == 0x95u &&
              no_money.words[0x5134u] == 0u,
          "missing money attribute keeps reset value zero");
}

void CheckExceptionalCleanup() {
    Runtime writer;
    writer.fail_slots = true;
    bool threw = false;
    try {
        (void)pal4::InventoryArchive::Serialize(0x5000u, 0x4000u,
                                                 writer);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && writer.calls.size() == 13u &&
              writer.calls[11] == "destroy document" &&
              writer.calls[12] == "release flag",
          "write failure destroys document and flag once");

    Runtime reader;
    reader.fail_slots = true;
    reader.stream_words = {0u, 0u, 0u};
    threw = false;
    try {
        (void)pal4::InventoryArchive::Deserialize(0x5000u, 0x4000u,
                                                   reader);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && reader.calls[reader.calls.size() - 2u] ==
                       "destroy document" &&
              reader.calls.back() == "release flag",
          "read failure destroys document and flag once");

    Runtime cleanup_failure;
    cleanup_failure.fail_slots = true;
    cleanup_failure.fail_document_cleanup = true;
    std::string error;
    try {
        (void)pal4::InventoryArchive::Serialize(
            0x5000u, 0x4000u, cleanup_failure);
    } catch (const std::runtime_error& caught) {
        error = caught.what();
    }
    Check(error == "slots" &&
              cleanup_failure.calls[cleanup_failure.calls.size() - 2u] ==
                  "destroy document" &&
              cleanup_failure.calls.back() == "release flag",
          "cleanup failure retains primary error and still releases flag");
}

}  // namespace

int main() {
    try {
        CheckSerialize();
        CheckDeserialize();
        CheckExceptionalCleanup();
        std::cout << "inventory_archive_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
