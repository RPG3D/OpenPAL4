#pragma once

#include <cstddef>
#include <string_view>

namespace pal4 {

class ITextFileScannerRuntime {
public:
    virtual ~ITextFileScannerRuntime() = default;
    [[nodiscard]] virtual void* OpenRead(const char* file_name) = 0;
    [[nodiscard]] virtual bool EndOfFile(void* file) = 0;
    virtual void ReadChunk(void* file, char* buffer, int capacity) = 0;
    virtual void Close(void* file) = 0;
    // The collection must copy both views before returning.
    virtual void InsertPair(std::string_view ordinary,
                            std::string_view dollar) = 0;
};

class ITextFilePairSink {
public:
    virtual ~ITextFilePairSink() = default;
    virtual void InsertPair(std::string_view ordinary,
                            std::string_view dollar) = 0;
};

class CrtTextFileScannerRuntime final : public ITextFileScannerRuntime {
public:
    explicit CrtTextFileScannerRuntime(ITextFilePairSink& sink) : sink_(sink) {}

    [[nodiscard]] void* OpenRead(const char* file_name) override;
    [[nodiscard]] bool EndOfFile(void* file) override;
    void ReadChunk(void* file, char* buffer, int capacity) override;
    void Close(void* file) override;
    void InsertPair(std::string_view ordinary,
                    std::string_view dollar) override;

private:
    ITextFilePairSink& sink_;
};

class TextFileScanner final {
public:
    static constexpr std::size_t kFgetsCapacity = 0x400;

    [[nodiscard]] static bool ParseFile(const char* file_name,
                                        ITextFileScannerRuntime& runtime);
};

}  // namespace pal4
