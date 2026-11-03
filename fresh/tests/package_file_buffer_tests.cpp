#include "pal4/package_file_buffer.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IPackageFileBufferRuntime {
public:
    std::uint32_t Allocate(std::uint32_t bytes) override {
        events.push_back("alloc:" + std::to_string(bytes));
        return allocation;
    }
    std::uint32_t Read(std::uint32_t handle, std::uint32_t destination,
                       std::uint32_t bytes) override {
        events.push_back("read:" + std::to_string(handle) + ":" +
                         std::to_string(destination) + ":" +
                         std::to_string(bytes));
        return read_result;
    }
    std::uint32_t Decrypt(std::uint32_t data, std::uint32_t word_count,
                          const char* key) override {
        events.push_back("decrypt:" + std::to_string(data) + ":" +
                         std::to_string(word_count));
        Check(std::strcmp(key, pal4::PackageFileBuffer::kDecryptionKey) == 0,
              "game package key is forwarded unchanged");
        return decrypt_result;
    }
    std::uint32_t Free(std::uint32_t data) override {
        events.push_back("free:" + std::to_string(data));
        return free_result;
    }

    std::uint32_t allocation = 0x3000u;
    std::uint32_t read_result = 0x1234u;
    std::uint32_t decrypt_result = 0x5678u;
    std::uint32_t free_result = 0x9abcu;
    std::vector<std::string> events;
};

void CheckInitializeAndRelease() {
    pal4::PackageFileBufferState state;
    state.loaded = 1u;
    state.data = 10u;
    state.auxiliary = 20u;
    state.bytes = 30u;
    state.unobserved_tail = 40u;
    Check(&pal4::PackageFileBuffer::Initialize(state) == &state &&
          state.loaded == 0u && state.data == 0u &&
          state.auxiliary == 0u && state.bytes == 0u &&
          state.unobserved_tail == 40u,
          "initializer returns this and leaves the fifth word untouched");
    Runtime runtime;
    Check(pal4::PackageFileBuffer::ReleaseThunk(state, runtime) == 0u &&
          runtime.events.empty(), "empty release forwards without freeing");
    state.data = 0x2000u;
    state.bytes = 99u;
    state.loaded = 1u;
    Check(pal4::PackageFileBuffer::ReleaseThunk(state, runtime) == 0x9abcu &&
          runtime.events == std::vector<std::string>({"free:8192"}) &&
          state.data == 0u && state.bytes == 0u && state.loaded == 0u,
          "release thunk forwards free result and resets fields");
}

void CheckLoadAndReload() {
    pal4::PackageFileBufferState state;
    Runtime runtime;
    Check(pal4::PackageFileBuffer::LoadFromHandle(
              state, 7u, 13u, false, runtime) == 0x1200u &&
          state.loaded == 1u && state.data == 0x3000u &&
          state.bytes == 13u &&
          runtime.events == std::vector<std::string>({
              "alloc:13", "read:7:12288:13"}),
          "plain load clears result low byte and tracks requested byte count");
    runtime.events.clear();
    Check(pal4::PackageFileBuffer::LoadFromHandle(
              state, 9u, 11u, true, runtime) == 0x5678u &&
          runtime.events == std::vector<std::string>({
              "free:12288", "alloc:11", "read:9:12288:11",
              "decrypt:12288:2"}),
          "reload releases first, decrypts allocated words with truncated count");
}

void CheckFailureBehavior() {
    pal4::PackageFileBufferState state;
    Runtime runtime;
    runtime.allocation = 0u;
    runtime.read_result = 0xa5b6c7d8u;
    Check(pal4::PackageFileBuffer::LoadFromHandle(
              state, 4u, 64u, true, runtime) == 0xa5b6c7d8u &&
          state.loaded == 1u && state.data == 0u && state.bytes == 64u &&
          runtime.events == std::vector<std::string>({
              "alloc:64", "read:4:0:64"}),
          "allocation failure still calls read and marks loaded");
}

}  // namespace

int main() {
    CheckInitializeAndRelease();
    CheckLoadAndReload();
    CheckFailureBehavior();
    std::cout << "3 package file buffer test cases passed\n";
}
