#include "pal4/data_stream_buffer.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IDataStreamBufferRuntime {
public:
    void Free(std::uint8_t* buffer) override {
        Check(buffer == old.data(), "old pointer freed");
        events.push_back("free");
    }
    void Read(void* stream, void* buffer, std::uint32_t size) override {
        Check(stream == this, "stream forwarded");
        if (events.empty() || events.back() != "allocate") {
            Check(buffer != nullptr, "length destination");
            std::memcpy(buffer, &length, 4);
            events.push_back("length");
        } else {
            Check(buffer == allocated.data() && size ==
                      static_cast<std::uint32_t>(length), "data read size");
            std::memcpy(buffer, payload.data(), size);
            events.push_back("data");
        }
    }
    std::uint8_t* Allocate(std::uint32_t size) override {
        Check(size == static_cast<std::uint32_t>(length), "allocation size");
        events.push_back("allocate");
        return allocated.data();
    }

    std::array<std::uint8_t, 4> old{};
    std::array<std::uint8_t, 8> allocated{};
    std::array<std::uint8_t, 8> payload{'A', 'B', 'C', 'D'};
    std::int32_t length = 4;
    std::vector<std::string> events;
};

}  // namespace

int main() {
    Runtime runtime;
    std::uint8_t* output = runtime.old.data();
    pal4::DataStreamBuffer::ReadInto(nullptr, output, runtime);
    Check(output == runtime.old.data() && runtime.events.empty(),
          "null stream leaves output untouched");

    pal4::DataStreamBuffer::ReadInto(&runtime, output, runtime);
    Check(runtime.events == std::vector<std::string>{
              "free", "length", "allocate", "data"},
          "free, length, allocation and data read order");
    Check(output == runtime.allocated.data() &&
              output[0] == 'A' && output[1] == 'B' &&
              output[2] == 'C' && output[3] == 0,
          "last byte forced to terminator");

    for (const auto length : {0, -1}) {
        runtime.events.clear();
        runtime.length = length;
        output = runtime.old.data();
        pal4::DataStreamBuffer::ReadInto(&runtime, output, runtime);
        Check(runtime.events == std::vector<std::string>{"free", "length"} &&
                  output == runtime.old.data(),
              "nonpositive length retains previous pointer without reading data");
    }
}
