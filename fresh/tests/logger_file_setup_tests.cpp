#include "pal4/logger_object.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct PrintCall final {
    std::uint32_t stream;
    std::string format;
    std::string argument;

    bool operator==(const PrintCall&) const = default;
};

class FakeRuntime final : public pal4::ILoggerFileSetupRuntime {
public:
    std::uint32_t Allocate(const std::uint32_t size) override {
        events.push_back("allocate:" + std::to_string(size));
        return allocation;
    }
    std::uint32_t Free(const std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
        return 0;
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        return memory[address];
    }
    std::uint32_t ReadDword(const std::uint32_t address) override {
        return static_cast<std::uint32_t>(memory[address]) |
               (static_cast<std::uint32_t>(memory[address + 1]) << 8U) |
               (static_cast<std::uint32_t>(memory[address + 2]) << 16U) |
               (static_cast<std::uint32_t>(memory[address + 3]) << 24U);
    }
    void Copy(const std::uint32_t destination, const std::uint32_t source,
              const std::uint32_t size) override {
        events.push_back("copy:" + std::to_string(destination) + ":" +
                         std::to_string(source) + ":" +
                         std::to_string(size));
        std::vector<std::uint8_t> copied;
        copied.reserve(size);
        for (std::uint32_t i = 0; i < size; ++i) copied.push_back(memory[source + i]);
        for (std::uint32_t i = 0; i < size; ++i) memory[destination + i] = copied[i];
    }
    void WriteDword(const std::uint32_t address,
                    const std::uint32_t value) override {
        for (std::uint32_t i = 0; i < 4; ++i) {
            memory[address + i] = static_cast<std::uint8_t>(value >> (i * 8U));
        }
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        memory[address] = value;
    }
    std::uint32_t FindSubstring(std::uint32_t, std::uint32_t) override { return 0; }
    std::uint32_t FindCharacter(std::uint32_t, std::int32_t) override { return 0; }
    std::uint32_t FindLastCharacter(std::uint32_t, std::int32_t) override { return 0; }
    std::int32_t CompareCaseInsensitive(std::uint32_t, std::uint32_t) override {
        return 0;
    }
    void ConvertUppercase(std::uint32_t) override {}
    void ConvertLowercase(std::uint32_t) override {}

    std::string ReadCString(const std::uint32_t address) override {
        events.push_back("strlen:" + std::to_string(address));
        std::string result;
        for (std::uint32_t i = 0; i < 1024 && memory[address + i] != 0; ++i) {
            result.push_back(static_cast<char>(memory[address + i]));
        }
        return result;
    }
    std::uint32_t OpenWriteFile(const std::uint32_t filename_address,
                                const std::string_view mode) override {
        events.push_back("open:" + std::to_string(filename_address) + ":" +
                         std::string(mode));
        return stream;
    }
    std::string GetModuleFileNameA(const std::uint32_t capacity) override {
        events.push_back("module:" + std::to_string(capacity));
        return module_path;
    }
    std::string GetDateText() override {
        events.push_back("date");
        return "09/25/26";
    }
    std::string GetTimeText() override {
        events.push_back("time");
        return "12:34:56";
    }
    void Fprintf(const std::uint32_t handle, const std::string_view format,
                 const std::string_view argument) override {
        prints.push_back({handle, std::string(format), std::string(argument)});
    }
    void CloseFile(const std::uint32_t handle) override {
        events.push_back("close:" + std::to_string(handle));
    }

    void PutDword(const std::uint32_t address, const std::uint32_t value) {
        WriteDword(address, value);
    }
    void PutCString(const std::uint32_t address, const std::string_view value) {
        for (std::size_t i = 0; i < value.size(); ++i) {
            memory[address + static_cast<std::uint32_t>(i)] =
                static_cast<std::uint8_t>(value[i]);
        }
        memory[address + static_cast<std::uint32_t>(value.size())] = 0;
    }

    std::unordered_map<std::uint32_t, std::uint8_t> memory;
    std::vector<std::string> events;
    std::vector<PrintCall> prints;
    std::uint32_t allocation = 0x42424242;
    std::uint32_t stream = 0x9000;
    std::string module_path = "C:\\Game\\PAL4.exe";
};

FakeRuntime PreparedRuntime() {
    FakeRuntime runtime;
    runtime.PutDword(0x4000, 0x41414141);
    runtime.PutDword(0x4008, 64);
    runtime.PutCString(0x3000, "trace.log");
    return runtime;
}

void OpenFailureStillUpdatesBufferButSkipsHeader() {
    auto runtime = PreparedRuntime();
    runtime.stream = 0;
    pal4::LoggerObject::SetFilePathAndWriteHeader(0x4000, 0x3000, runtime);

    Check(runtime.ReadDword(0x4004) == 9 &&
              runtime.ReadCString(0x41414141) == "trace.log",
          "path buffer is initialized before attempting fopen");
    Check(runtime.events[0] == "strlen:12288" &&
              runtime.events[1] == "copy:1094795585:12288:9" &&
              runtime.events[2] == "open:1094795585:wt",
          "fopen receives the initialized string data pointer");
    Check(runtime.prints.empty() && runtime.events.size() == 4,
          "open failure skips module lookup, writes, and fclose");
}

void SuccessfulOpenWritesObservedHeaderSequence() {
    auto runtime = PreparedRuntime();
    pal4::LoggerObject::SetFilePathAndWriteHeader(0x4000, 0x3000, runtime);

    const std::string separator = std::string(100, ':') + "\n";
    const std::string module_label = "PAL4.exe";
    Check(runtime.prints == std::vector<PrintCall>{
              {0x9000, separator, ""}, {0x9000, "$>\t", ""},
              {0x9000, "[%s Log at ", module_label},
              {0x9000, "09/25/26", ""},
              {0x9000, " %s]\n", "12:34:56"},
              {0x9000, separator, ""}, {0x9000, "\n\n", ""}},
          "header preserves all seven fprintf formats and their arguments");
    Check(runtime.events[3] == "module:512" &&
              runtime.events[4] == "date" && runtime.events[5] == "time",
          "module name and date/time are requested after a successful open");
    Check(runtime.events.back() == "close:36864",
          "successful header emission closes the stream");
}

void StringBufferGrowthOccursBeforeTheOpenCall() {
    auto runtime = PreparedRuntime();
    runtime.PutDword(0x4008, 4);
    runtime.stream = 0;
    pal4::LoggerObject::SetFilePathAndWriteHeader(0x4000, 0x3000, runtime);
    Check(runtime.ReadDword(0x4000) == 0x42424242 &&
              runtime.ReadDword(0x4008) == 17,
          "short existing capacity grows to source length plus eight");
    Check(runtime.events == std::vector<std::string>{
              "strlen:12288", "allocate:17", "free:1094795585",
              "copy:1111638594:12288:9", "open:1111638594:wt"},
          "old buffer is freed and replacement is filled before fopen");
}

void NoBackslashYieldsEmptyModuleLabel() {
    auto runtime = PreparedRuntime();
    runtime.module_path = "PAL4.exe";
    pal4::LoggerObject::SetFilePathAndWriteHeader(0x4000, 0x3000, runtime);

    Check(runtime.prints[2].argument.empty(),
          "no backslash copies a leading zero byte into the module label");
}

void TrailingBackslashYieldsEmptyModuleLabel() {
    auto runtime = PreparedRuntime();
    runtime.module_path = "C:\\Game\\";
    pal4::LoggerObject::SetFilePathAndWriteHeader(0x4000, 0x3000, runtime);

    Check(runtime.prints[2].argument.empty(),
          "trailing backslash leaves the module label empty");
}

void MissingModulePathYieldsEmptyModuleLabel() {
    auto runtime = PreparedRuntime();
    runtime.module_path.clear();
    pal4::LoggerObject::SetFilePathAndWriteHeader(0x4000, 0x3000, runtime);

    Check(runtime.prints[2].argument.empty(),
          "zero module-path length yields an empty label");
}

}  // namespace

int main() {
    OpenFailureStillUpdatesBufferButSkipsHeader();
    SuccessfulOpenWritesObservedHeaderSequence();
    StringBufferGrowthOccursBeforeTheOpenCall();
    NoBackslashYieldsEmptyModuleLabel();
    TrailingBackslashYieldsEmptyModuleLabel();
    MissingModulePathYieldsEmptyModuleLabel();
    std::cout << "6 logger-file-setup test cases passed\n";
}
