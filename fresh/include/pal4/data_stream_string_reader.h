#pragma once

#include <cstddef>
#include <cstdint>

namespace pal4 {

class IDataStreamStringRuntime {
public:
    virtual ~IDataStreamStringRuntime() = default;
    virtual void ReadData(std::uint32_t context, void* output,
                          std::uint32_t size) = 0;
    [[nodiscard]] virtual char* Allocate(std::uint32_t size) = 0;
    virtual void InitializeString(std::uint32_t output,
                                  const char* bytes,
                                  std::size_t length) = 0;
    virtual void Free(char* bytes) = 0;
};

class DataStreamStringRuntimeStub final : public IDataStreamStringRuntime {
public:
    void ReadData(std::uint32_t, void*, std::uint32_t) override {}
    [[nodiscard]] char* Allocate(std::uint32_t) override { return nullptr; }
    void InitializeString(std::uint32_t, const char*,
                          std::size_t) override {}
    void Free(char*) override {}
};

class DataStreamStringReader final {
public:
    // IDA 0x5fa0b0.
    static void Read(std::uint32_t context, std::uint32_t output,
                     IDataStreamStringRuntime& runtime);
};

}  // namespace pal4
