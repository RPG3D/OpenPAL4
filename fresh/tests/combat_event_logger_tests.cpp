#include "pal4/combat_event_logger.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatEventLoggerRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> messages;
    int opaque_reads = 0;
    int close_result = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("read-byte:" + std::to_string(address));
        return bytes.at(address);
    }

    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.push_back("byte:" + std::to_string(address) + ":" +
                        std::to_string(value));
        bytes[address] = value;
    }
    std::uint8_t OpaqueStackByte() override {
        calls.emplace_back("opaque");
        ++opaque_reads;
        return 0x5au;
    }
    void InitializeIos(std::uint32_t ios,
                       std::uint32_t buffer,
                       std::uint32_t mode) override {
        Check(mode == 0u, "ios initializer mode");
        calls.push_back("ios:" + std::to_string(ios) + ":" +
                        std::to_string(buffer));
    }
    void ConstructFileBuffer(std::uint32_t buffer,
                             std::uint32_t mode) override {
        Check(buffer == 0x1000u + 8u && mode == 0u,
              "file buffer constructor arguments");
        calls.emplace_back("filebuf");
    }
    void InitializeStreamBuffer(std::uint32_t buffer) override {
        Check(buffer == 0x1000u + 164u,
              "secondary stream buffer address");
        calls.emplace_back("streambuf");
    }
    void InitializeExtendedBuffer(std::uint32_t buffer,
                                  std::uint32_t first,
                                  std::uint32_t second,
                                  std::uint8_t converted_flags) override {
        Check(buffer == 0x1000u + 164u && first == 0u &&
                  second == 0u && converted_flags == 4u,
              "extended buffer init arguments");
        calls.emplace_back("extended-buffer");
    }
    void LogEvent(std::uint32_t logger, std::uint32_t level,
                  std::uint32_t message,
                  std::uint32_t format) override {
        Check(logger == 0x1000u && level == 1u && format == 0u,
              "startup log arguments");
        calls.emplace_back("log");
        messages.push_back(message);
    }
    int CloseFile(std::uint32_t handle) override {
        calls.push_back("close:" + std::to_string(handle));
        return close_result;
    }
    void InitializeFileBuffer(std::uint32_t buffer,
                              std::uint32_t handle,
                              std::uint32_t mode) override {
        calls.push_back("init-filebuf:" + std::to_string(buffer) + ":" +
                        std::to_string(handle) + ":" + std::to_string(mode));
    }
    void ClearIos(std::uint32_t ios, std::uint32_t state,
                  bool raise) override {
        calls.push_back("clear:" + std::to_string(ios) + ":" +
                        std::to_string(state) + ":" + std::to_string(raise));
    }
    void CleanupBuffer(std::uint32_t buffer) override {
        calls.push_back("cleanup-buffer:" + std::to_string(buffer));
    }
    void DestroyStreamBuffer(std::uint32_t buffer) override {
        calls.push_back("destroy-streambuf:" + std::to_string(buffer));
    }
    void DestroyIos(std::uint32_t ios) override {
        calls.push_back("destroy-ios:" + std::to_string(ios));
    }
    void SetErrorFlag(std::uint32_t record,
                      std::uint8_t flag) override {
        calls.push_back("error:" + std::to_string(record) + ":" +
                        std::to_string(flag));
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
    void CloseFileBuffer(std::uint32_t buffer) override {
        calls.push_back("close-filebuf:" + std::to_string(buffer));
    }
    void DestroyTextRecord(std::uint32_t record,
                           std::uint32_t flags) override {
        calls.push_back("destroy-text:" + std::to_string(record) + ":" +
                        std::to_string(flags));
    }
    void DestroyLocale(std::uint32_t locale) override {
        calls.push_back("destroy-locale:" + std::to_string(locale));
    }
};

void PrepareCleanup(Runtime& runtime, std::uint32_t handle,
                    std::uint32_t count, bool filebuf_open,
                    std::uint32_t text) {
    constexpr std::uint32_t kLogger = 0x1000u;
    runtime.words[kLogger + 88u] = handle;
    runtime.words[kLogger + 4u] = 0x8480d0u;
    runtime.words[0x8480d4u] = 88u;
    runtime.words[kLogger + 160u] = 0x848180u;
    runtime.words[0x848184u] = 80u;
    runtime.words[kLogger + 148u] = count ? 0x2000u : 0u;
    runtime.words[kLogger + 152u] = count ? 0x2000u + count * 20u : 0u;
    runtime.words[kLogger + 156u] = count ? 0x2100u : 0u;
    runtime.words[kLogger + 76u] = text;
    runtime.bytes[kLogger + 80u] = filebuf_open ? 1u : 0u;
}

std::size_t FindCall(const Runtime& runtime, const std::string& call) {
    for (std::size_t i = 0; i < runtime.calls.size(); ++i)
        if (runtime.calls[i] == call) return i;
    throw std::runtime_error("missing call: " + call);
}

void CleanupAfterSuccessfulClose() {
    Runtime runtime;
    PrepareCleanup(runtime, 0x3000u, 3u, true, 0x4000u);
    pal4::CombatEventLogger::Cleanup(0x1000u, runtime);
    Check(FindCall(runtime, "close:12288") <
              FindCall(runtime, "init-filebuf:4104:0:2"),
          "successful fclose reinitializes file buffer");
    Check(FindCall(runtime, "write:4336:8683848") <
              FindCall(runtime, "cleanup-buffer:4260") &&
              FindCall(runtime, "cleanup-buffer:4260") <
              FindCall(runtime, "destroy-streambuf:4260") &&
              FindCall(runtime, "destroy-streambuf:4260") <
              FindCall(runtime, "destroy-ios:4336"),
          "secondary stream destruction order");
    Check(FindCall(runtime, "destroy-ios:4336") <
              FindCall(runtime, "error:8192:1") &&
              FindCall(runtime, "error:8192:1") <
              FindCall(runtime, "error:8212:1") &&
              FindCall(runtime, "error:8212:1") <
              FindCall(runtime, "error:8232:1") &&
              FindCall(runtime, "error:8232:1") <
              FindCall(runtime, "free:8192"),
          "20-byte records released in forward order");
    Check(FindCall(runtime, "free:8192") <
              FindCall(runtime, "write:4244:0") &&
              FindCall(runtime, "write:4244:0") <
              FindCall(runtime, "write:4248:0") &&
              FindCall(runtime, "write:4248:0") <
              FindCall(runtime, "write:4252:0"),
          "collection cleared after release");
    Check(FindCall(runtime, "write:4188:8683724") <
              FindCall(runtime, "write:4104:8815320") &&
              FindCall(runtime, "write:4104:8815320") <
              FindCall(runtime, "close-filebuf:4104") &&
              FindCall(runtime, "close-filebuf:4104") <
              FindCall(runtime, "destroy-text:16384:1") &&
              FindCall(runtime, "destroy-text:16384:1") <
              FindCall(runtime, "destroy-locale:4180") &&
              FindCall(runtime, "destroy-locale:4180") <
              FindCall(runtime, "destroy-streambuf:4104") &&
              FindCall(runtime, "destroy-streambuf:4104") <
              FindCall(runtime, "destroy-ios:4188"),
          "primary stream destruction order");
    Check(runtime.words[0x1000u + 148u] == 0u &&
              runtime.words[0x1000u + 152u] == 0u &&
              runtime.words[0x1000u + 156u] == 0u &&
              runtime.words[0x1000u + 92u] == 0x8682d0u &&
              runtime.words[0x1000u + 240u] == 0x8682d0u,
          "cleanup final field values");
}

void CleanupAfterFailedClose() {
    for (const auto attached : {0u, 0x5000u}) {
        Runtime runtime;
        PrepareCleanup(runtime, 0x3000u, 0u, false, 0u);
        runtime.close_result = -1;
        runtime.words[0x1000u + 96u] = 0x120u;
        runtime.words[0x1000u + 132u] = attached;
        pal4::CombatEventLogger::Cleanup(0x1000u, runtime);
        Check(FindCall(runtime, attached ? "clear:4188:290:0" :
                                          "clear:4188:294:0") <
                  FindCall(runtime, "cleanup-buffer:4260"),
              "failed fclose sets badbit and absent-buffer failbit");
        Check(FindCall(runtime, "free:0") <
                  FindCall(runtime, "destroy-locale:4180"),
              "empty collection still frees null pointer");
        Check(runtime.words[0x1000u + 148u] == 0u &&
                  runtime.words[0x1000u + 152u] == 0u,
              "failed close still cleans collection");
    }
}

void CleanupWithoutFileHandle() {
    Runtime runtime;
    PrepareCleanup(runtime, 0u, 0u, false, 0u);
    pal4::CombatEventLogger::Cleanup(0x1000u, runtime);
    Check(runtime.calls.front() == "read:4184" &&
              runtime.calls[1] == "read:4256",
          "null file handle skips close and state handling");
    Check(FindCall(runtime, "destroy-ios:4336") <
              FindCall(runtime, "destroy-ios:4188"),
          "both streams still destroyed");
}

void ConstructorOrder() {
    Runtime runtime;
    Check(pal4::CombatEventLogger::Construct(0x1000u, runtime) == 0x1000u,
          "logger constructor returns self");
    Check(runtime.calls == std::vector<std::string>{
              "write:4096:1", "write:4100:8683728", "write:4224:0",
              "write:4188:8815312", "write:4188:8815304",
              "ios:4188:4104", "filebuf", "write:4188:8683724",
              "opaque", "byte:4240:90", "write:4244:0",
              "write:4248:0", "write:4252:0", "write:4256:8683904",
              "write:4372:0", "write:4336:8815312",
              "write:4336:8815304", "ios:4336:4260", "streambuf",
              "write:4260:8683852", "extended-buffer",
              "write:4336:8683848", "byte:4388:1",
              "log", "log", "log", "log", "log", "log", "log"},
          "logger field writes, stream initialization and log order");
    Check(runtime.opaque_reads == 1 &&
              runtime.bytes[0x1000u + 144u] == 0x5au &&
              runtime.bytes[0x1000u + 292u] == 1u &&
              runtime.words[0x1000u + 92u] == 0x8480ccu &&
              runtime.words[0x1000u + 240u] == 0x848148u &&
              runtime.words[0x1000u + 148u] == 0u &&
              runtime.words[0x1000u + 152u] == 0u &&
              runtime.words[0x1000u + 156u] == 0u,
          "logger final fields and opaque byte");
    Check(runtime.messages == std::vector<std::uint32_t>{
              0x8b8f40u, 0x8b8ef0u, 0x8b8ea0u, 0x8b8e50u,
              0x8b8e00u, 0x8b8da8u, 0x8b8d88u},
          "seven fixed combat log messages");
}

}  // namespace

int main() {
    try {
        ConstructorOrder();
        CleanupAfterSuccessfulClose();
        CleanupAfterFailedClose();
        CleanupWithoutFileHandle();
        std::cout << "combat event logger tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
