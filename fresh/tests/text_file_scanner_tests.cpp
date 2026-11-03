#include "pal4/text_file_scanner.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class FakeRuntime final : public pal4::ITextFileScannerRuntime {
public:
    explicit FakeRuntime(std::string text) : text_(std::move(text)) {}

    [[nodiscard]] void* OpenRead(const char* file_name) override {
        ++opens;
        path = file_name;
        return open_succeeds ? this : nullptr;
    }

    [[nodiscard]] bool EndOfFile(void* file) override {
        Check(file == this, "EOF query uses the opened handle");
        return eof_;
    }

    void ReadChunk(void* file, char* buffer, int capacity) override {
        Check(file == this && capacity == 0x400,
              "fgets boundary receives the original buffer size");
        ++reads;
        if (position_ == text_.size()) {
            eof_ = true;
            return;
        }
        int count = 0;
        while (position_ < text_.size() && count < capacity - 1) {
            const char value = text_[position_++];
            buffer[count++] = value;
            if (value == '\n') break;
        }
        buffer[count] = 0;
    }

    void Close(void* file) override {
        Check(file == this, "close uses the opened handle");
        ++closes;
    }

    void InsertPair(std::string_view ordinary,
                    std::string_view dollar) override {
        pairs.emplace_back(ordinary, dollar);
    }

    bool open_succeeds = true;
    int opens = 0;
    int reads = 0;
    int closes = 0;
    std::string path;
    std::vector<std::pair<std::string, std::string>> pairs;

private:
    std::string text_;
    std::size_t position_ = 0;
    bool eof_ = false;
};

void NullAndFailedOpenReturnFalse() {
    FakeRuntime null_path("");
    Check(!pal4::TextFileScanner::ParseFile(nullptr, null_path) &&
              null_path.opens == 0 && null_path.closes == 0,
          "null path returns before opening");

    FakeRuntime failed("");
    failed.open_succeeds = false;
    Check(!pal4::TextFileScanner::ParseFile("missing.txt", failed) &&
              failed.opens == 1 && failed.path == "missing.txt" &&
              failed.reads == 0 && failed.closes == 0,
          "failed open returns without reading or closing");
}

void ClosingBraceInsertsOnePair() {
    FakeRuntime runtime("{alpha $beta}\n");
    Check(pal4::TextFileScanner::ParseFile("pairs.txt", runtime) &&
              runtime.pairs == std::vector<std::pair<std::string, std::string>>{
                                   {"alpha ", "beta"}} &&
              runtime.opens == 1 && runtime.closes == 1,
          "brace and dollar markers route text and closing brace inserts");
}

void ClosingBraceStopsTheCurrentChunk() {
    FakeRuntime runtime("{first$one}{second$two}\n");
    Check(pal4::TextFileScanner::ParseFile("pairs.txt", runtime) &&
              runtime.pairs == std::vector<std::pair<std::string, std::string>>{
                                   {"first", "one"}},
          "text after the first closing brace in a chunk is ignored");
}

void ClosingBraceCanInsertEmptyStrings() {
    FakeRuntime runtime("}\n");
    Check(pal4::TextFileScanner::ParseFile("pairs.txt", runtime) &&
              runtime.pairs == std::vector<std::pair<std::string, std::string>>{
                                   {"", ""}},
          "closing brace inserts even without preceding text");
}

void CommentImmediatelyAfterClosingBraceChangesDispatch() {
    FakeRuntime line_comment("{lost$pair}//skip\n");
    Check(pal4::TextFileScanner::ParseFile("pairs.txt", line_comment) &&
              line_comment.pairs.empty(),
          "line comment after closing brace suppresses pair insertion");

    FakeRuntime block_comment("{kept$value}/*skip*/x\n");
    Check(pal4::TextFileScanner::ParseFile("pairs.txt", block_comment) &&
              block_comment.pairs ==
                  std::vector<std::pair<std::string, std::string>>{
                      {"kept", "value"}},
          "block comment after closing brace restores insertion mode");
}

void LineCommentDropsRemainderOfChunk() {
    FakeRuntime runtime("{discard//comment}\n{keep$yes}\n");
    Check(pal4::TextFileScanner::ParseFile("pairs.txt", runtime) &&
              runtime.pairs == std::vector<std::pair<std::string, std::string>>{
                                   {"keep", "yes"}},
          "line comment skips closing brace and remaining chunk text");
}

void BlockCommentRestoresPreviousMode() {
    FakeRuntime runtime("{ab/* hidden */cd$xy/*skip*/z}\n");
    Check(pal4::TextFileScanner::ParseFile("pairs.txt", runtime) &&
              runtime.pairs == std::vector<std::pair<std::string, std::string>>{
                                   {"abcd", "xyz"}},
          "block comment preserves ordinary and dollar modes");
}

void BlockCommentNewlineReadsAgainInsideChunk() {
    FakeRuntime runtime("{ab/*x\nz*/cd$xy}\n");
    Check(pal4::TextFileScanner::ParseFile("pairs.txt", runtime) &&
              runtime.pairs == std::vector<std::pair<std::string, std::string>>{
                                   {"abcd", "xy"}} &&
              runtime.reads == 3,
          "block comment reads the next line and skips its first character");
}

void EveryOuterChunkResetsTemporaryStrings() {
    FakeRuntime runtime("{old\nnew$yes}\n");
    Check(pal4::TextFileScanner::ParseFile("pairs.txt", runtime) &&
              runtime.pairs == std::vector<std::pair<std::string, std::string>>{
                                   {"new", "yes"}},
          "a later fgets chunk replaces both temporary strings");
}

void LongPhysicalLineUsesFgetsSizedChunks() {
    FakeRuntime runtime("{" + std::string(1022, 'a') + "tail}\n");
    Check(pal4::TextFileScanner::ParseFile("pairs.txt", runtime) &&
              runtime.pairs == std::vector<std::pair<std::string, std::string>>{
                                   {"tail", ""}} &&
              runtime.reads == 3,
          "a 1023-byte fgets chunk resets before the next chunk");
}

void EmptyFileStillClosesAndSucceeds() {
    FakeRuntime runtime("");
    Check(pal4::TextFileScanner::ParseFile("empty.txt", runtime) &&
              runtime.reads == 1 && runtime.closes == 1 &&
              runtime.pairs.empty(),
          "empty opened file returns success after EOF and close");
}

class PairSink final : public pal4::ITextFilePairSink {
public:
    void InsertPair(std::string_view ordinary,
                    std::string_view dollar) override {
        pairs.emplace_back(ordinary, dollar);
    }

    std::vector<std::pair<std::string, std::string>> pairs;
};

void PublicCrtFileAdapterReadsAndClosesRealFile() {
    const auto suffix = std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    const auto path = std::filesystem::temp_directory_path() /
                      ("pal4_text_file_scanner_" + suffix + ".txt");
    {
        std::ofstream output(path, std::ios::binary);
        Check(static_cast<bool>(output), "temporary CRT input opens for writing");
        output << "{from$crt}\n";
    }

    PairSink sink;
    pal4::CrtTextFileScannerRuntime runtime(sink);
    const auto path_text = path.string();
    const bool parsed = pal4::TextFileScanner::ParseFile(path_text.c_str(), runtime);
    std::filesystem::remove(path);
    Check(parsed && sink.pairs ==
                        std::vector<std::pair<std::string, std::string>>{
                            {"from", "crt"}},
          "public CRT adapter reads a named file and forwards copied pair text");
}

}  // namespace

int main() {
    NullAndFailedOpenReturnFalse();
    ClosingBraceInsertsOnePair();
    ClosingBraceStopsTheCurrentChunk();
    ClosingBraceCanInsertEmptyStrings();
    CommentImmediatelyAfterClosingBraceChangesDispatch();
    LineCommentDropsRemainderOfChunk();
    BlockCommentRestoresPreviousMode();
    BlockCommentNewlineReadsAgainInsideChunk();
    EveryOuterChunkResetsTemporaryStrings();
    LongPhysicalLineUsesFgetsSizedChunks();
    EmptyFileStillClosesAndSucceeds();
    PublicCrtFileAdapterReadsAndClosesRealFile();
    std::cout << "12 text-file-scanner test cases passed\n";
}
