#include "pal4/virtual_file_system_initializer.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
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

class Runtime final : public pal4::IVirtualFileSystemRuntime {
public:
    std::uint32_t GetCurrentDirectory(const std::uint32_t size,
                                       std::string& output) override {
        events.emplace_back("directory");
        Check(size == 0x104u, "Win32 directory buffer is 260 bytes");
        output = directory;
        return directory_length;
    }
    void AppendPathSeparator(std::string& path) override {
        events.emplace_back("separator");
        path += '\\';
    }
    std::uint32_t CreateCallbacks(const std::uint32_t size,
                                   const std::string_view path,
                                   const std::string_view alias) override {
        events.emplace_back("create");
        Check(size == 0x14u, "filesystem callback record size");
        created.emplace_back(std::string(path), std::string(alias));
        const std::uint32_t call = create_calls++;
        return call == fail_create_at ? 0u : 0x100u + call;
    }
    std::uint32_t RegisterCallbacks(
        const std::uint32_t callbacks) override {
        events.emplace_back("register");
        registered.push_back(callbacks);
        return register_calls++ == fail_register_at ? 0u : 1u;
    }
    void UppercasePath(std::string& path) override {
        events.emplace_back("uppercase");
        std::transform(path.begin(), path.end(), path.begin(),
                       [](const unsigned char value) {
                           return static_cast<char>(std::toupper(value));
                       });
    }
    std::uint32_t GetLogicalDrives() override {
        events.emplace_back("drives");
        return drive_mask;
    }
    std::uint32_t GetDriveType(const std::string_view root) override {
        events.emplace_back("drive-type");
        drive_roots.emplace_back(root);
        Check(root.size() == 3u && root[1] == ':' && root[2] == '\\',
              "GetDriveType receives rooted drive path");
        return root[0] == 'B' ? 0u
             : root[0] == 'D' ? 1u
             : 3u;
    }
    void SetGlobalCallback(const std::uint32_t callbacks) override {
        events.emplace_back("set-global");
        selected.push_back(callbacks);
        global_callback = callbacks;
    }
    std::uint32_t GetGlobalCallback() override {
        events.emplace_back("get-global");
        return global_callback;
    }
    std::uint32_t InitializePackFileManager() override {
        events.emplace_back("pack-init");
        return 0x900u;
    }
    void ProcessPackFiles(const std::uint32_t manager,
                           const std::uint32_t callbacks) override {
        events.emplace_back("pack-process");
        Check(manager == 0x900u, "pack manager receives created instance");
        processed_callback = callbacks;
    }
    std::uint32_t InitializeLogger() override {
        events.emplace_back("logger-init");
        return 0x1234u;
    }
    void AppendLog(const std::uint32_t logger, const std::string_view format,
                   const std::string_view source_file,
                   const std::uint32_t line) override {
        events.emplace_back("logger-append");
        Check(logger == 0x1234u &&
                  source_file ==
                      R"(D:\\VcProject\\Pal4\\project\\Sourcecode\\RwCasing\\rwc_Initialize.cpp)",
              "logger and original source path");
        logs.emplace_back(std::string(format), line);
    }

    std::string directory = "c:\\Game";
    std::uint32_t directory_length = 7u;
    std::uint32_t drive_mask =
        (1u << 1u) | (1u << 2u) | (1u << 3u) | (1u << 4u);
    std::uint32_t fail_create_at = 0xffffffffu;
    std::uint32_t fail_register_at = 0xffffffffu;
    std::uint32_t create_calls = 0u;
    std::uint32_t register_calls = 0u;
    std::uint32_t global_callback = 0u;
    std::uint32_t processed_callback = 0xffffffffu;
    std::vector<std::pair<std::string, std::string>> created;
    std::vector<std::uint32_t> registered;
    std::vector<std::string> drive_roots;
    std::vector<std::uint32_t> selected;
    std::vector<std::pair<std::string, std::uint32_t>> logs;
    std::vector<std::string> events;
};

void CheckLog(const Runtime& runtime, const std::string_view message,
              const std::uint32_t line) {
    Check(runtime.logs.size() == 1u &&
              runtime.logs[0] ==
                  std::pair<std::string, std::uint32_t>{
                      "FILE:  [%s]\nLINE:  [%d]\nINFO:  " +
                          std::string(message) + "\n\n\n",
                      line} &&
              runtime.events[runtime.events.size() - 2u] == "logger-init" &&
              runtime.events.back() == "logger-append",
          "exact message, original source line and logger order");
}

void CurrentDirectoryFailures() {
    for (const std::uint32_t length : {0u, 0x105u}) {
        pal4::GameRenderState state(0x9000u);
        Runtime runtime;
        runtime.directory_length = length;
        Check(pal4::VirtualFileSystemInitializer(state).Initialize(runtime) ==
                  0u &&
                  runtime.created.empty(),
              "invalid directory length skips filesystem setup");
        CheckLog(runtime,
                 "Error[RWCInstallFileSystem failed <CurrentDirectory error>]",
                 0x2f0u);
    }
    pal4::GameRenderState state(0x9000u);
    Runtime runtime;
    runtime.directory_length = 0x104u;
    runtime.drive_mask = 0u;
    Check(pal4::VirtualFileSystemInitializer(state).Initialize(runtime) ==
              1u &&
              runtime.created.size() == 1u,
          "exactly 260 return bytes pass original greater-than check");
}

void BaseCallbackFailures() {
    pal4::GameRenderState state(0x9000u);
    {
        Runtime runtime;
        runtime.fail_create_at = 0u;
        Check(pal4::VirtualFileSystemInitializer(state).Initialize(runtime) ==
                  0u &&
                  runtime.logs.empty() && runtime.registered.empty(),
              "base callback creation failure returns silently");
    }
    {
        Runtime runtime;
        runtime.fail_register_at = 0u;
        Check(pal4::VirtualFileSystemInitializer(state).Initialize(runtime) ==
                  0u &&
                  runtime.registered == std::vector<std::uint32_t>{0x100u},
              "base callback registration failure stops initialization");
        CheckLog(runtime,
                 "Error[RWCInstallFileSystem failed <RtFSManagerRegister>]",
                 0x301u);
    }
}

void DriveCallbackFailures() {
    pal4::GameRenderState state(0x9000u);
    for (const bool create_failure : {true, false}) {
        Runtime runtime;
        runtime.fail_create_at = create_failure ? 1u : 0xffffffffu;
        runtime.fail_register_at = create_failure ? 0xffffffffu : 1u;
        Check(pal4::VirtualFileSystemInitializer(state).Initialize(runtime) ==
                  0u &&
                  runtime.logs.empty() &&
                  runtime.created.size() == 2u &&
                  runtime.drive_roots ==
                      std::vector<std::string>{"B:\\", "C:\\"} &&
                  runtime.selected.empty(),
              "drive callback creation/registration failure is silent");
    }
}

void DriveSelectionAndPackFiles() {
    pal4::GameRenderState state(0x9000u);
    state.Write(0x58cu, 1u);
    Runtime runtime;
    Check(pal4::VirtualFileSystemInitializer(state).Initialize(runtime) ==
              1u &&
              runtime.created ==
                  std::vector<std::pair<std::string, std::string>>{
                      {"c:\\Game\\", "rwc"}, {"C:", "C"}, {"E:", "E"}} &&
              runtime.drive_roots ==
                  std::vector<std::string>{
                      "B:\\", "C:\\", "D:\\", "E:\\"} &&
              runtime.registered ==
                  std::vector<std::uint32_t>{0x100u, 0x101u, 0x102u} &&
              runtime.selected == std::vector<std::uint32_t>{0x101u} &&
              runtime.processed_callback == 0x101u,
          "drive bits, type skip, uppercase matching and pack callback");
    CheckLog(runtime, "RWCInstallFileSystem success", 0x34du);
    Check(runtime.events[runtime.events.size() - 5u] == "pack-init" &&
              runtime.events[runtime.events.size() - 4u] == "get-global" &&
              runtime.events[runtime.events.size() - 3u] == "pack-process",
          "pack manager runs before success log");
}

void NonDriveDirectoryAndNoPackFiles() {
    pal4::GameRenderState state(0x9000u);
    Runtime runtime;
    runtime.directory = "\\\\server\\share";
    runtime.directory_length =
        static_cast<std::uint32_t>(runtime.directory.size());
    Check(pal4::VirtualFileSystemInitializer(state).Initialize(runtime) ==
              1u &&
              runtime.selected ==
                  std::vector<std::uint32_t>{0x100u, 0x100u} &&
              runtime.processed_callback == 0xffffffffu &&
              std::find(runtime.events.begin(), runtime.events.end(),
                        "pack-init") == runtime.events.end(),
          "non-drive directory selects base callbacks for each valid drive");
    CheckLog(runtime, "RWCInstallFileSystem success", 0x34du);
}

}  // namespace

int main() {
    CurrentDirectoryFailures();
    BaseCallbackFailures();
    DriveCallbackFailures();
    DriveSelectionAndPackFiles();
    NonDriveDirectoryAndNoPackFiles();
    std::cout << "8 virtual filesystem initialization paths passed\n";
}
