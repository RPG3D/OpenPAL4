#include "pal4/game_db_archive_writer.h"

#include <algorithm>
#include <bit>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(const bool okay, const char* message) {
    if (!okay) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IGameDbArchiveRuntime {
    std::vector<std::string> events;
    std::vector<std::uint32_t> words_written;
    std::map<std::uint32_t, pal4::ArchiveTextRecordState> texts;
    std::map<std::uint32_t, std::string> data;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> property_entries;
    std::vector<std::uint32_t> object_properties;
    std::map<std::uint32_t, std::uint32_t> integer_values;
    std::map<std::uint32_t, std::uint32_t> text_values;
    std::map<std::uint32_t, float> float_values;
    std::vector<std::uint32_t> properties;
    bool open_succeeds = true;
    bool property_succeeds = true;
    bool object_succeeds = true;

    std::uint32_t Open(const char* filename, const char* mode) override {
        events.push_back(std::string("open:") + filename + ":" + mode);
        return open_succeeds ? 0x8000u : 0u;
    }
    std::uint32_t CreateFileStream(std::uint32_t file) override {
        events.push_back("stream:" + std::to_string(file));
        return 0x9000u;
    }
    pal4::ArchiveTextRecordState TextRecord(
        std::uint32_t address) override {
        events.push_back("text-record:" + std::to_string(address));
        return texts.at(address);
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    std::uint32_t PropertyEntry(std::uint32_t property,
                                std::uint32_t index) override {
        events.push_back("entry:" + std::to_string(property) + ":" +
                         std::to_string(index));
        return property_entries.at(index);
    }
    std::uint32_t ObjectProperty(std::uint32_t object,
                                 std::uint32_t index) override {
        events.push_back("object-property:" + std::to_string(object) +
                         ":" + std::to_string(index));
        return object_properties.at(index);
    }
    std::uint32_t IntegerProperty(std::uint32_t property,
                                  std::uint32_t index) override {
        events.push_back("integer:" + std::to_string(index));
        return integer_values.at(property + index);
    }
    std::uint32_t TextProperty(std::uint32_t property,
                               std::uint32_t index) override {
        events.push_back("text-value:" + std::to_string(index));
        return text_values.at(property + index);
    }
    float FloatProperty(std::uint32_t property,
                        std::uint32_t index) override {
        events.push_back("float:" + std::to_string(index));
        return float_values.at(property + index);
    }
    std::uint32_t FirstProperty(
        const pal4::GameDbRecordIndexState&,
        std::uint32_t& cursor) override {
        events.push_back("first-property");
        cursor = 0u;
        return properties.empty() ? 0u : properties[0u];
    }
    std::uint32_t NextProperty(
        const pal4::GameDbRecordIndexState&,
        std::uint32_t& cursor) override {
        events.push_back("next-property");
        ++cursor;
        return cursor < properties.size() ? properties[cursor] : 0u;
    }
    bool WriteProperty(std::uint32_t property,
                       const pal4::ArchiveStreamAdapterState& stream) override {
        events.push_back("property:" + std::to_string(property));
        return property_succeeds &&
            pal4::GameDbArchiveWriter::WritePropertyRecord(
                property, stream, *this);
    }
    bool WriteObject(std::uint32_t object,
                     const pal4::ArchiveStreamAdapterState&) override {
        events.push_back("object:" + std::to_string(object));
        return object_succeeds;
    }
    void Flush(std::uint32_t file) override {
        events.push_back("flush:" + std::to_string(file));
    }
    void Close(std::uint32_t file) override {
        events.push_back("close:" + std::to_string(file));
    }
    void FinishAdapter(const pal4::ArchiveStreamAdapterState&) override {
        events.push_back("finish");
    }
    std::uint32_t InvokeVirtual(std::uint32_t stream,
                                std::uint32_t slot,
                                std::uintptr_t buffer,
                                std::uint32_t bytes) override {
        Check(stream == 0x9000u, "archive writes use the constructed stream");
        Check(slot == 16u, "archive writes dispatch the write slot");
        events.push_back("write:" + std::to_string(bytes));
        if (bytes == 4u) {
            std::uint32_t value = 0u;
            std::memcpy(&value, reinterpret_cast<const void*>(buffer), 4u);
            words_written.push_back(value);
        }
        return 1u;
    }
    const void* DataAt(std::uint32_t address) override {
        return data.at(address).data();
    }
    const void* EmptyData() override { return ""; }
};

}  // namespace

int main() {
    Runtime runtime;
    pal4::GameDbRecordIndexState index;
    pal4::GameDbFlagStaticState flag;
    flag.data = 0x4000u;
    flag.length = 12u;
    runtime.data[0x4000u] = "GAME_DB_FLAG";
    runtime.data[0x4100u] = "property";
    runtime.data[0x4200u] = "record";
    runtime.texts[0x2010u] = {0u, 0x4100u, 8u};
    runtime.texts[0x2020u] = {0u, 0x4100u, 8u};
    runtime.texts[0x3000u] = {0u, 0x4200u, 6u};
    runtime.properties = {0x2000u};
    runtime.words[0x2004u] = 0u;
    index.words[7u] = 1u;
    index.words[1u] = 0x3000u;
    index.words[2u] = 0x3030u;
    runtime.words[0x3014u] = 0x6000u;
    runtime.words[0x302cu] = 0u;

    Check(!pal4::GameDbArchiveWriter::WriteToFile(
              nullptr, index, flag, runtime) && runtime.events.empty(),
          "null filename skips file operations");
    runtime.open_succeeds = false;
    Check(!pal4::GameDbArchiveWriter::WriteToFile(
              "save.dat", index, flag, runtime) &&
              runtime.events == std::vector<std::string>{
                  "open:save.dat:wb+"},
          "file-open failure exits before stream construction");

    runtime.open_succeeds = true;
    runtime.events.clear();
    Check(pal4::GameDbArchiveWriter::WriteToFile(
              "save.dat", index, flag, runtime) &&
              runtime.events == std::vector<std::string>{
                  "open:save.dat:wb+", "stream:32768", "write:4",
                  "write:12", "write:4", "write:4", "first-property",
                  "text-record:8208", "write:4", "write:8",
                  "property:8192", "write:4", "text-record:8224",
                  "write:4", "write:8", "read:8196", "write:4",
                  "write:4", "next-property", "write:4",
                  "read:12308", "text-record:12288", "write:4",
                  "write:6", "object:24576", "read:12332",
                  "flush:32768", "close:32768", "finish"} &&
              runtime.words_written ==
                  std::vector<std::uint32_t>{
                      12u, 20060228u, 1u, 8u, 20060303u, 8u, 0u,
                      20060304u, 2u, 6u},
          "archive writes header, properties and active objects in order");

    runtime.events.clear();
    runtime.property_succeeds = false;
    Check(!pal4::GameDbArchiveWriter::WriteToFile(
              "save.dat", index, flag, runtime) &&
              runtime.events.back() == "finish" &&
              runtime.events[runtime.events.size() - 2u] == "property:8192",
          "property failure stops before record serialization");
    runtime.property_succeeds = true;
    runtime.object_succeeds = false;
    runtime.events.clear();
    Check(!pal4::GameDbArchiveWriter::WriteToFile(
              "save.dat", index, flag, runtime) &&
              runtime.events.back() == "finish" &&
              runtime.events[runtime.events.size() - 2u] == "object:24576",
          "object failure stops without flush or close");

    runtime.events.clear();
    runtime.words_written.clear();
    runtime.words[0x2004u] = 0x7000u;
    runtime.words[0x2008u] = 0x7030u;
    runtime.property_entries = {0x7000u, 0x7018u};
    runtime.words[0x7000u] = 10u;
    runtime.words[0x7018u] = 20u;
    runtime.texts[0x7004u] = {0u, 0x4300u, 3u};
    runtime.texts[0x701cu] = {0u, 0x4400u, 3u};
    runtime.data[0x4300u] = "one";
    runtime.data[0x4400u] = "two";
    pal4::ArchiveStreamAdapterState stream{0x9000u, 0u, {}};
    Check(pal4::GameDbArchiveWriter::WritePropertyRecord(
              0x2000u, stream, runtime) &&
              runtime.words_written == std::vector<std::uint32_t>{
                  20060303u, 8u, 2u, 10u, 3u, 20u, 3u, 20060304u} &&
              runtime.events == std::vector<std::string>{
                  "write:4", "text-record:8224", "write:4", "write:8",
                  "read:8196", "read:8200", "write:4",
                  "entry:8192:0", "read:28672", "write:4",
                  "text-record:28676", "write:4", "write:3",
                  "entry:8192:1", "read:28696", "write:4",
                  "text-record:28700", "write:4", "write:3", "write:4"},
          "property writer emits markers and every 24-byte entry");

    runtime.events.clear();
    runtime.words_written.clear();
    runtime.words[0x6028u] = 0x7000u;
    runtime.words[0x602cu] = 0x7010u;
    runtime.words[0x6018u] = 0xa000u;
    runtime.words[0x601cu] = 0xa0b4u;
    runtime.words[0x6054u] = 99u;
    runtime.words[0xa010u] = 1u;
    runtime.words[0xa04cu] = 2u;
    runtime.words[0xa088u] = 3u;
    runtime.object_properties = {0x7000u};
    runtime.integer_values[0x7000u] = 42u;
    runtime.text_values[0x7001u] = 0xb000u;
    runtime.float_values[0x7002u] = 1.5f;
    runtime.data[0x4500u] = "obj";
    runtime.data[0x4600u] = "tail";
    runtime.data[0x4700u] = "field";
    runtime.data[0x4800u] = "caption";
    runtime.data[0x4900u] = "memo";
    runtime.data[0x4a00u] = "value";
    runtime.texts[0x6034u] = {0u, 0x4500u, 3u};
    runtime.texts[0x6044u] = {0u, 0x4600u, 4u};
    for (const auto offset : {0u, 60u, 120u}) {
        runtime.texts[0xa000u + offset] = {0u, 0x4700u, 5u};
        runtime.texts[0xa014u + offset] = {0u, 0x4800u, 7u};
        runtime.texts[0xa024u + offset] = {0u, 0x4900u, 4u};
    }
    runtime.texts[0xb000u] = {0u, 0x4a00u, 5u};
    const auto object_written = pal4::GameDbArchiveWriter::WriteObjectRecord(
        0x6000u, stream, runtime);
    Check(object_written &&
              runtime.words_written.front() == 20060225u &&
              runtime.words_written[4u] == 99u &&
              runtime.words_written[5u] == 3u &&
              runtime.words_written[6u] == 1u &&
              runtime.words_written[runtime.words_written.size() - 6u] ==
                  20060223u &&
              runtime.words_written[runtime.words_written.size() - 5u] ==
                  42u &&
              runtime.words_written[runtime.words_written.size() - 3u] ==
                  std::bit_cast<std::uint32_t>(1.5f) &&
              runtime.words_written[runtime.words_written.size() - 2u] ==
                  20060224u &&
              runtime.words_written.back() == 20060226u,
          "object writer emits typed row and all native markers");
    Check(runtime.events.back() == "write:4" &&
              runtime.events.end() !=
                  std::find(runtime.events.begin(), runtime.events.end(),
                            "integer:0") &&
              runtime.events.end() !=
                  std::find(runtime.events.begin(), runtime.events.end(),
                            "text-value:1") &&
              runtime.events.end() !=
                  std::find(runtime.events.begin(), runtime.events.end(),
                            "float:2"),
          "object writer dispatches integer, text and float fields");
    runtime.words_written.clear();
    runtime.events.clear();
    runtime.words[0xa088u] = 4u;
    Check(!pal4::GameDbArchiveWriter::WriteObjectRecord(
              0x6000u, stream, runtime) &&
              runtime.words_written.back() == 5u &&
              runtime.words_written.end() !=
                  std::find(runtime.words_written.begin(),
                            runtime.words_written.end(), 20060223u) &&
              runtime.words_written.end() ==
                  std::find(runtime.words_written.begin(),
                            runtime.words_written.end(), 20060224u) &&
              runtime.words_written.end() ==
                  std::find(runtime.words_written.begin(),
                            runtime.words_written.end(), 20060226u),
          "unknown field type stops inside the row before closing markers");

    runtime.words_written.clear();
    runtime.events.clear();
    runtime.words[0x6218u] = 0u;
    runtime.words[0x6228u] = 0u;
    runtime.words[0x6254u] = 7u;
    runtime.texts[0x6234u] = {0u, 0x4500u, 3u};
    runtime.texts[0x6244u] = {0u, 0x4500u, 3u};
    Check(pal4::GameDbArchiveWriter::WriteObjectRecord(
              0x6200u, stream, runtime) &&
              runtime.words_written == std::vector<std::uint32_t>{
                  20060225u, 3u, 3u, 7u, 0u, 0u, 20060226u},
          "empty object arrays still write counts and closing marker");
    std::cout << "GameDB archive writer tests passed\n";
}
