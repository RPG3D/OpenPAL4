#include "pal4/animation_script_loader.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

class Runtime final : public pal4::IAnimationScriptLoaderRuntime {
public:
    bool open_success = true;
    std::vector<std::string> lines;
    std::size_t cursor = 0;
    std::vector<std::string> calls;
    std::vector<std::pair<std::string, std::string>> records;
    std::vector<std::uint32_t> positions;
    std::map<std::uint32_t, pal4::AnimationScriptRecordState> old_records;

    pal4::AnimationScriptRecordState& RecordAt(
        std::uint32_t address) override {
        calls.emplace_back("record:" + std::to_string(address));
        return old_records.at(address);
    }
    std::uint8_t ReadReference(std::uint32_t) override { return 0; }
    void WriteReference(std::uint32_t, std::uint8_t) override {}
    std::uint32_t Free(std::uint32_t) override { return 0; }
    std::uint32_t Open(std::string_view path,
                       std::string_view mode) override {
        calls.emplace_back("open:" + std::string(path) + ":" +
                           std::string(mode));
        return open_success ? 0x3333u : 0u;
    }
    bool EndOfFile(std::uint32_t file) override {
        Check(file == 0x3333u, "EOF receives open handle");
        return cursor >= lines.size();
    }
    std::string ReadLine(std::uint32_t file,
                         std::size_t buffer_size) override {
        Check(file == 0x3333u && buffer_size == 1024u,
              "read uses open handle and native buffer size");
        calls.emplace_back("read");
        return lines.at(cursor++);
    }
    std::size_t EffectiveLength(std::string_view line) override {
        calls.emplace_back("length");
        return line.size();
    }
    void Insert(pal4::AnimationScriptRecordArrayState& state,
                std::uint32_t position, std::uint32_t count,
                std::string_view first, std::string_view second) override {
        Check(count == 1u && position == state.end,
              "insert appends exactly one record at current end");
        calls.emplace_back("insert");
        positions.push_back(position);
        records.emplace_back(first, second);
        state.end += 32u;
    }
    void Close(std::uint32_t file) override {
        Check(file == 0x3333u, "close receives open handle");
        calls.emplace_back("close");
    }
};

}  // namespace

int main() {
    Runtime runtime;
    pal4::AnimationScriptRecordArrayState state{7u, 0x2000u,
                                                  0x2040u, 0x2080u};
    runtime.old_records[0x2000u] = {};
    runtime.old_records[0x2020u] = {};
    runtime.open_success = false;
    Check(pal4::AnimationScriptLoader::Load(state, "missing.amd", runtime) == 0 &&
              runtime.calls == std::vector<std::string>{
                  "record:8192", "record:8224", "open:missing.amd:rt"} &&
              state.end == state.begin,
          "load clears prior records before failed open and returns zero");

    runtime = {};
    state.end = state.begin;
    runtime.lines = {"{walk$run}\n", "{idle$stand}\n"};
    Check(pal4::AnimationScriptLoader::Load(state, "motion.amd", runtime) == 1 &&
              runtime.records ==
                  std::vector<std::pair<std::string, std::string>>{
                      {"walk", "run"}, {"idle", "stand"}} &&
              runtime.positions == std::vector<std::uint32_t>{0x2000u, 0x2020u} &&
              runtime.calls.back() == "close" && state.end == 0x2040u,
          "two records append in order and file closes");

    runtime = {};
    state.end = state.begin;
    runtime.lines = {"// ignore {x$y}\n", "{a/*skip*/b$c}\n"};
    Check(pal4::AnimationScriptLoader::Load(state, "comments.amd", runtime) == 1 &&
              runtime.records ==
                  std::vector<std::pair<std::string, std::string>>{{"ab", "c"}},
          "line and inline block comments retain correct mode");

    runtime = {};
    state.end = state.begin;
    runtime.lines = {"{ab/*ignore\n", "middle*/cd$ef}\n", ""};
    Check(pal4::AnimationScriptLoader::Load(state, "multi.amd", runtime) == 1 &&
              runtime.records ==
                  std::vector<std::pair<std::string, std::string>>{{"abcd", "ef"}},
          "block comment consumes subsequent file line and restores mode");

    runtime = {};
    state.end = state.begin;
    runtime.lines = {"{ab/*ignore\n", "middle*/cd$ef}\n"};
    Check(pal4::AnimationScriptLoader::Load(state, "eof.amd", runtime) == 1 &&
              runtime.records.empty() && runtime.calls.back() == "close",
          "EOF inside block-comment continuation stops processing that line");

    runtime = {};
    state.end = state.begin;
    runtime.lines = {"{first$\n", "second}\n"};
    Check(pal4::AnimationScriptLoader::Load(state, "reset.amd", runtime) == 1 &&
              runtime.records ==
                  std::vector<std::pair<std::string, std::string>>{{"", "second"}},
          "outer nonempty line clears both temporary strings");

    pal4::AnimationScriptLoaderRuntimeStub stub;
    Check(pal4::AnimationScriptLoader::Load(state, "unused", stub) == 0,
          "native runtime stub is linkable");
    std::cout << "animation_script_loader_tests passed\n";
}
