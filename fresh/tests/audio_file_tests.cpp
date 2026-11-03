#include "pal4/audio_file.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool okay, const char* message) {
    if (!okay) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct FileRuntime final : pal4::IAudioFileDestroyRuntime {
    std::vector<std::string> events;
    std::uint32_t read_result = 0x2000u;
    std::uint32_t type = 1u;
    bool decompress_success = true;
    void InitializePath(pal4::StringBufferState& path) override {
        events.push_back("path-init");
        path = {};
    }
    std::uint32_t CleanupPath(pal4::StringBufferState& path) override {
        events.push_back("path-clean:" + std::to_string(path.length));
        return 0x77u;
    }
    void AssignPath(pal4::StringBufferState& path, const char* text,
                    std::uint32_t length) override {
        events.push_back("path-assign:" + std::string(text) + ":" +
                         std::to_string(length));
        path.length = length;
    }
    std::uint32_t FileSize(const char* path) override {
        events.push_back("size:" + std::string(path));
        return 0x120u;
    }
    std::uint32_t FileRead(const char* path,
                           std::uint32_t flags) override {
        events.push_back("read:" + std::string(path) + ":" +
                         std::to_string(flags));
        return read_result;
    }
    std::uint32_t FileType(std::uint32_t data,
                           std::uint32_t bytes) override {
        events.push_back("type:" + std::to_string(data) + ":" +
                         std::to_string(bytes));
        return type;
    }
    void WaveInfo(std::uint32_t data, pal4::AudioWaveInfo& info) override {
        events.push_back("wave-info:" + std::to_string(data));
        info[0] = 0x42u;
    }
    bool DecompressAdpcm(const pal4::AudioWaveInfo& info,
                          std::uint32_t& output,
                          std::uint32_t& bytes) override {
        events.push_back("adpcm:" + std::to_string(info[0]));
        if (decompress_success) {
            output = 0x3000u;
            bytes = 0x240u;
        }
        return decompress_success;
    }
    bool DecompressAsi(std::uint32_t data, std::uint32_t bytes,
                        const char* extension, std::uint32_t& output,
                        std::uint32_t& output_bytes,
                        std::uint32_t flags) override {
        events.push_back("asi:" + std::to_string(data) + ":" +
                         std::to_string(bytes) + ":" + extension + ":" +
                         std::to_string(flags));
        if (decompress_success) {
            output = 0x4000u;
            output_bytes = 0x480u;
        }
        return decompress_success;
    }
    void FreeLocked(std::uint32_t data) override {
        events.push_back("free:" + std::to_string(data));
    }
    std::uint32_t FreeFile(std::uint32_t address) override {
        events.push_back("free-file:" + std::to_string(address));
        return 0x66u;
    }
    void Log(std::uint32_t message_id) override {
        events.push_back("log:" + std::to_string(message_id));
    }
    std::uint32_t ComputeCrc(const char* path,
                             std::uint32_t length) override {
        events.push_back("crc:" + std::string(path) + ":" +
                         std::to_string(length));
        return 0x1234u;
    }
};

void Lifecycle() {
    FileRuntime runtime;
    pal4::AudioFileState file{};
    file.path_crc = 0xaau;
    file.data = 0xbbu;
    file.bytes = 0xccu;
    file.reference_count = 0xddu;
    file.path = {0x5000u, 5u, 7u};
    Check(&pal4::AudioFile::Initialize(file, runtime) == &file &&
              file.path_crc == 0xaau && file.data == 0u &&
              file.bytes == 0xccu && file.reference_count == 0u &&
              file.path.data == 0u && file.path.length == 0u &&
              runtime.events == std::vector<std::string>{"path-init"},
          "initializer clears only selected words then initializes path");
    runtime.events.clear();
    Check(pal4::AudioFile::Cleanup(file, runtime) == 0x77u &&
              runtime.events == std::vector<std::string>{"path-clean:0"},
          "empty file cleanup skips locked-memory free");
    runtime.events.clear();
    file.data = 0x3000u;
    file.path.length = 4u;
    Check(pal4::AudioFile::Cleanup(file, runtime) == 0x77u &&
              runtime.events == std::vector<std::string>{
                  "free:12288", "path-clean:4"} &&
              file.data == 0x3000u,
          "populated cleanup frees locked data before path without clearing it");
}

void LoadBranches() {
    FileRuntime runtime;
    pal4::AudioFileState file{};
    runtime.read_result = 0u;
    Check(pal4::AudioFile::Load(file, "sound.wav", runtime) == 0u &&
              runtime.events == std::vector<std::string>{
                  "size:sound.wav", "read:sound.wav:0", "log:97"},
          "read failure logs 97 without querying type or freeing data");

    runtime.events.clear();
    runtime.read_result = 0x2000u;
    runtime.type = 7u;
    Check(pal4::AudioFile::Load(file, "sound.wav", runtime) == 0u &&
              runtime.events == std::vector<std::string>{
                  "size:sound.wav", "read:sound.wav:0",
                  "type:8192:288", "free:8192"},
          "unsupported file type frees raw input without logging");

    runtime.events.clear();
    runtime.type = 1u;
    Check(pal4::AudioFile::Load(file, "sound.wav", runtime) == 1u &&
              file.data == 0x2000u && file.bytes == 0x120u &&
              file.path_crc == 0x1234u && file.path.length == 9u &&
              runtime.events == std::vector<std::string>{
                  "size:sound.wav", "read:sound.wav:0",
                  "type:8192:288", "crc:sound.wav:9",
                  "path-assign:sound.wav:9"},
          "raw type retains read buffer and stores CRC/path metadata");

    runtime.events.clear();
    file = {};
    runtime.type = 2u;
    runtime.decompress_success = false;
    Check(pal4::AudioFile::Load(file, "packed.wav", runtime) == 0u &&
              runtime.events == std::vector<std::string>{
                  "size:packed.wav", "read:packed.wav:0",
                  "type:8192:288", "wave-info:8192", "adpcm:66",
                  "log:99"} && file.path_crc == 0u,
          "ADPCM failure logs 99 without releasing input or storing path");

    runtime.events.clear();
    runtime.decompress_success = true;
    Check(pal4::AudioFile::Load(file, "packed.wav", runtime) == 1u &&
              file.data == 0x3000u && file.bytes == 0x240u &&
              runtime.events == std::vector<std::string>{
                  "size:packed.wav", "read:packed.wav:0",
                  "type:8192:288", "wave-info:8192", "adpcm:66",
                  "free:8192", "crc:packed.wav:10",
                  "path-assign:packed.wav:10"},
          "successful ADPCM decompress frees source before path metadata");

    runtime.events.clear();
    file = {};
    runtime.type = 13u;
    runtime.decompress_success = false;
    Check(pal4::AudioFile::Load(file, "music.mp3", runtime) == 0u &&
              runtime.events == std::vector<std::string>{
                  "size:music.mp3", "read:music.mp3:0",
                  "type:8192:288", "asi:8192:288:.mp3:0", "log:98"},
          "MP3 failure logs 98 without releasing input or storing metadata");

    runtime.events.clear();
    runtime.decompress_success = true;
    Check(pal4::AudioFile::Load(file, "music.mp3", runtime) == 1u &&
              file.data == 0x4000u && file.bytes == 0x480u &&
              file.path_crc == 0x1234u && file.path.length == 9u &&
              runtime.events == std::vector<std::string>{
                  "size:music.mp3", "read:music.mp3:0",
                  "type:8192:288", "asi:8192:288:.mp3:0",
                  "free:8192", "crc:music.mp3:9",
                  "path-assign:music.mp3:9"},
          "successful MP3 decompress uses exact extension and frees source");
}

void DestroyBranches() {
    FileRuntime runtime;
    pal4::AudioFileState file{};
    file.data = 0x3000u;
    file.path.length = 4u;
    Check(pal4::AudioFile::DestroyAt(
              file, 0x1000u, 0u, runtime) == 0x1000u &&
              runtime.events == std::vector<std::string>{
                  "free:12288", "path-clean:4"},
          "flag zero cleans audio file without freeing its object");
    runtime.events.clear();
    Check(pal4::AudioFile::DestroyAt(
              file, 0x1000u, 2u, runtime) == 0x1000u &&
              runtime.events == std::vector<std::string>{
                  "free:12288", "path-clean:4"},
          "only delete flag bit zero controls object freeing");
    runtime.events.clear();
    Check(pal4::AudioFile::DestroyAt(
              file, 0x1000u, 1u, runtime) == 0x1000u &&
              runtime.events == std::vector<std::string>{
                  "free:12288", "path-clean:4", "free-file:4096"},
          "delete flag frees object after nested audio file cleanup");
}

}  // namespace

int main() {
    Lifecycle();
    LoadBranches();
    DestroyBranches();
    std::cout << "Audio file tests passed\n";
}
