#include "pal4/effect_stream_record.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void Check(bool good, const char* message) {
    if (!good) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IEffectStreamRecordRuntime {
    std::vector<std::array<std::uint32_t, 3u>> calls;
    void ReadData(std::uint32_t context, std::uint32_t offset,
                  std::uint32_t bytes,
                  pal4::EffectStreamRecord& record) override {
        Check(context == 0x700u && bytes == 4u,
              "word reads use supplied context and four bytes");
        calls.push_back({1u, offset, bytes});
        record.words[offset] = 0x1000u + offset;
    }
    void ReadString(std::uint32_t context, std::uint32_t offset,
                    pal4::EffectStreamRecord& record) override {
        Check(context == 0x700u, "string reads use supplied context");
        calls.push_back({2u, offset, 0u});
        record.words[offset] = 0x2000u + offset;
    }
};

}  // namespace

int main() {
    pal4::EffectStreamRecord record;
    record.words.fill(0xdeadbeefu);
    Runtime runtime;
    pal4::EffectStreamRecordReader::Read(record, 0u, runtime);
    Check(runtime.calls.empty() && record.words[0u] == 0xdeadbeefu,
          "null stream leaves record untouched");
    pal4::EffectStreamRecordReader::Read(record, 0x700u, runtime);
    Check(runtime.calls == std::vector<std::array<std::uint32_t, 3u>>{
              {1u, 0u, 4u}, {2u, 1u, 0u}, {1u, 4u, 4u},
              {1u, 5u, 4u}, {2u, 6u, 0u}, {2u, 9u, 0u},
              {2u, 12u, 0u}},
          "stream fields read in native data/string order");
    for (const auto index : {0u, 4u, 5u})
        Check(record.words[index] == 0x1000u + index,
              "scalar field written at native offset");
    for (const auto index : {1u, 6u, 9u, 12u})
        Check(record.words[index] == 0x2000u + index,
              "string field written at native offset");
    Check(record.words[2u] == 0xdeadbeefu &&
              record.words[14u] == 0xdeadbeefu,
          "unread record words preserved");
}
