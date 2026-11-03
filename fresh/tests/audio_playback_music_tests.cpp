#include "pal4/audio_playback_handle.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IPlaybackMusicRuntime {
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint8_t playing = 0u;
    std::uint8_t flag = 0u;
    std::uint8_t reference = 0u;
    std::uint32_t initial_data = 0x9000u;
    std::uint32_t played_path_address = 0u;
    std::uint32_t stored_handle = 0u;
    std::uint32_t freed = 0u;
    bool append_allocation = true;
    std::string played_path;

    void Put(std::uint32_t address, std::string_view text) {
        for (const char ch : text)
            bytes[address++] = static_cast<std::uint8_t>(ch);
        bytes[address] = 0u;
    }
    std::string Get(std::uint32_t address) {
        std::string result;
        while (bytes.at(address) != 0u)
            result.push_back(static_cast<char>(bytes.at(address++)));
        return result;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == pal4::AudioPlaybackHandle::kAddress,
              "audio handle address");
        calls.emplace_back("store_handle");
        stored_handle = value;
    }
    std::uint8_t IsPlaying(std::uint32_t address) override {
        Check(address == pal4::AudioPlaybackHandle::kAddress,
              "is-playing handle address");
        calls.emplace_back("is_playing");
        return playing;
    }
    void InitializePath(pal4::AudioPathState& path,
                        std::uint8_t new_flag,
                        std::string_view prefix) override {
        calls.emplace_back("prefix");
        flag = new_flag;
        Check(prefix == "gamedata\\palSound\\", "audio path prefix");
        path.data = initial_data;
        if (initial_data != 0u) {
            path.length = static_cast<std::uint32_t>(prefix.size());
            path.capacity = path.length;
            Put(initial_data, prefix);
            bytes[initial_data - 1u] = reference;
        }
    }
    bool AllocateText(pal4::AudioPathState& path,
                      std::uint32_t length,
                      std::uint8_t mode) override {
        Check(mode == 0u, "append allocation mode");
        calls.emplace_back("allocate");
        if (!append_allocation) return false;
        const auto prefix = path.data != 0u ? Get(path.data) : std::string{};
        path.data = 0xa000u;
        path.capacity = length;
        Put(path.data, prefix);
        bytes[path.data - 1u] = reference;
        return true;
    }
    void CopyText(std::uint32_t destination,
                  std::string_view text) override {
        calls.emplace_back("copy");
        for (const char ch : text)
            bytes[destination++] = static_cast<std::uint8_t>(ch);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.emplace_back("byte");
        bytes[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    std::uint32_t EmptyString() override {
        calls.emplace_back("empty_string");
        bytes[0xb000u] = 0u;
        return 0xb000u;
    }
    std::uint32_t Play(std::uint8_t new_flag,
                       std::uint32_t path,
                       std::uint32_t repeat) override {
        Check(new_flag == flag && repeat == 1u,
              "native play arguments");
        calls.emplace_back("play");
        played_path_address = path;
        played_path = Get(path);
        return 0x1234u;
    }
    std::uint8_t Free(std::uint32_t address) override {
        calls.emplace_back("free");
        freed = address;
        return 0x5au;
    }
};

void CheckPlaybackAndSkip() {
    Runtime runtime;
    Check(pal4::AudioPlaybackHandle::PlayMusic(
              "wa04.wav", 1u, runtime) == 0x5au &&
              runtime.played_path == "gamedata\\palSound\\wa04.wav" &&
              runtime.played_path_address == 0xa000u &&
              runtime.stored_handle == 0x1234u &&
              runtime.freed == 0x9fffu &&
              runtime.calls == (std::vector<std::string>{
                  "is_playing", "prefix", "allocate", "copy", "byte",
                  "play", "store_handle", "free"}),
          "new playback appends the filename and stores the native handle");

    Runtime already_playing;
    already_playing.playing = 2u;
    Check(pal4::AudioPlaybackHandle::PlayMusic(
              "ignored.wav", 1u, already_playing) == 2u &&
              already_playing.calls ==
                  (std::vector<std::string>{"is_playing"}),
          "nonzero flag preserves current playback and returns status byte");

    Runtime forced;
    forced.playing = 1u;
    Check(pal4::AudioPlaybackHandle::PlayMusic(
              "forced.wav", 0u, forced) == 0x5au &&
              forced.played_path ==
                  "gamedata\\palSound\\forced.wav",
          "zero flag starts playback even when music is already playing");
}

void CheckPathBranchesAndCleanup() {
    Runtime failed_append;
    failed_append.append_allocation = false;
    Check(pal4::AudioPlaybackHandle::PlayMusic(
              "wa03.wav", 1u, failed_append) == 0x5au &&
              failed_append.played_path == "gamedata\\palSound\\" &&
              failed_append.freed == 0x8fffu,
          "failed append still plays and releases the original prefix");

    Runtime shared;
    shared.reference = 2u;
    Check(pal4::AudioPlaybackHandle::PlayMusic(
              "wa03.wav", 1u, shared) == 0xffu &&
              shared.bytes.at(0x9fffu) == 1u && shared.freed == 0u,
          "shared path decrements reference and returns header low byte");

    Runtime no_data;
    no_data.initial_data = 0u;
    no_data.append_allocation = false;
    Check(pal4::AudioPlaybackHandle::PlayMusic(
              "missing.wav", 1u, no_data) == 0u &&
              no_data.played_path_address == 0xb000u &&
              no_data.played_path.empty() && no_data.freed == 0u,
          "missing path storage uses empty-string fallback and returns zero");

    Runtime empty_file;
    Check(pal4::AudioPlaybackHandle::PlayMusic(
              "", 1u, empty_file) == 0x5au &&
              empty_file.played_path == "gamedata\\palSound\\" &&
              empty_file.calls == (std::vector<std::string>{
                  "is_playing", "prefix", "play", "store_handle", "free"}),
          "empty filename skips append allocation");

    Runtime embedded_null;
    Check(pal4::AudioPlaybackHandle::PlayMusic(
              std::string_view("a.wav\0ignored", 13u),
              1u, embedded_null) == 0x5au &&
              embedded_null.played_path ==
                  "gamedata\\palSound\\a.wav",
          "filename processing stops at the first NUL byte");
}

}  // namespace

int main() {
    try {
        CheckPlaybackAndSkip();
        CheckPathBranchesAndCleanup();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
