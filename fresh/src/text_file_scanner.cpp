#include "pal4/text_file_scanner.h"

#include <array>
#include <cstdio>
#include <cstring>
#include <string>

namespace pal4 {
namespace {

enum class ScanMode : unsigned {
    Ordinary = 0,
    Dollar = 1,
    ClosingBrace = 2,
    LineComment = 3,
    BlockComment = 4,
};

}  // namespace

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
void* CrtTextFileScannerRuntime::OpenRead(const char* file_name) {
    return std::fopen(file_name, "r");
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

bool CrtTextFileScannerRuntime::EndOfFile(void* file) {
    return std::feof(static_cast<std::FILE*>(file)) != 0;
}

void CrtTextFileScannerRuntime::ReadChunk(void* file, char* buffer,
                                          const int capacity) {
    static_cast<void>(std::fgets(buffer, capacity,
                                  static_cast<std::FILE*>(file)));
}

void CrtTextFileScannerRuntime::Close(void* file) {
    static_cast<void>(std::fclose(static_cast<std::FILE*>(file)));
}

void CrtTextFileScannerRuntime::InsertPair(std::string_view ordinary,
                                            std::string_view dollar) {
    sink_.InsertPair(ordinary, dollar);
}

bool TextFileScanner::ParseFile(const char* file_name,
                                ITextFileScannerRuntime& runtime) {
    if (file_name == nullptr) return false;
    void* const file = runtime.OpenRead(file_name);
    if (file == nullptr) return false;

    std::array<char, kFgetsCapacity> buffer{};
    std::string ordinary;
    std::string dollar;
    ScanMode mode = ScanMode::Ordinary;
    ScanMode before_block_comment = ScanMode::Ordinary;
    std::size_t index = 0;

    while (!runtime.EndOfFile(file)) {
        runtime.ReadChunk(file, buffer.data(), static_cast<int>(buffer.size()));
        if (runtime.EndOfFile(file)) break;
        if (std::strlen(buffer.data()) == 0) continue;

        auto length = std::strlen(buffer.data());
        ordinary.clear();
        dollar.clear();
        for (; index < length; ++index) {
            char current = buffer[index];
            if (current == '{') {
                mode = ScanMode::Ordinary;
                ++index;
            } else if (current == '$') {
                mode = ScanMode::Dollar;
                ++index;
            } else if (current == '}') {
                mode = ScanMode::ClosingBrace;
                ++index;
            }

            current = buffer[index];
            if (current == '/') {
                if (buffer[index + 1] == '/') mode = ScanMode::LineComment;
                if (buffer[index + 1] == '*') {
                    before_block_comment = mode;
                    mode = ScanMode::BlockComment;
                    ++index;
                }
            }

            switch (mode) {
            case ScanMode::Ordinary:
                ordinary.push_back(buffer[index]);
                break;
            case ScanMode::Dollar:
                dollar.push_back(buffer[index]);
                break;
            case ScanMode::ClosingBrace:
                runtime.InsertPair(ordinary, dollar);
                [[fallthrough]];
            case ScanMode::LineComment:
                mode = ScanMode::Ordinary;
                index = length;
                break;
            case ScanMode::BlockComment:
                if (buffer[index] == '*' && buffer[index + 1] == '/') {
                    mode = before_block_comment;
                    ++index;
                } else if (buffer[index] == '\r' || buffer[index] == '\n') {
                    runtime.ReadChunk(file, buffer.data(),
                                      static_cast<int>(buffer.size()));
                    length = std::strlen(buffer.data());
                    index = 0;
                    if (buffer[0] == '*' && buffer[1] == '/') {
                        mode = before_block_comment;
                        index = length;
                    }
                }
                break;
            }
        }
        index = 0;
    }
    runtime.Close(file);
    return true;
}

}  // namespace pal4
