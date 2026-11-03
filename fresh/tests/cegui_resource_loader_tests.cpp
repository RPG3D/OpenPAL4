#include "pal4/cegui_resource_loader.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::CeguiResourceLoaderRuntimeStub {
public:
    pal4::CeguiRawDataContainer* output = nullptr;
    std::vector<std::string> calls;
    std::uint32_t string_storage = 0x1100u;
    bool equals_empty = false;
    bool use_package = true;
    std::uint32_t package_handle = 0x3000u;
    bool package_read = true;
    bool throw_on_read = false;
    bool local_open = true;
    bool local_close = true;
    bool throw_on_local_read = false;
    std::uint32_t buffer_size = 0u;

    std::uint32_t StringStorage(std::uint32_t address) override {
        Check(address == 0x1000u, "storage reads supplied CEGUI String");
        calls.emplace_back("storage");
        return string_storage;
    }
    bool EqualsEmptyString(std::uint32_t address) override {
        Check(address == 0x1000u, "equality checks supplied CEGUI String");
        calls.emplace_back("equals_empty");
        return equals_empty;
    }
    bool UsePackageFiles() override {
        calls.emplace_back("package_mode");
        return use_package;
    }
    std::uint32_t BuildUtf8(std::uint32_t address) override {
        Check(address == 0x1000u, "UTF-8 conversion uses resource name");
        calls.emplace_back("utf8");
        return 0x2000u;
    }
    void LoadUiScriptPath(std::uint32_t utf8, char* output_path,
                          std::uint32_t capacity) override {
        Check(utf8 == 0x2000u && capacity == 256u,
              "UI path resolution receives UTF-8 name and 256-byte output");
        calls.emplace_back("resolve_path");
        std::memcpy(output_path, "ui/menu.xml", sizeof("ui/menu.xml"));
    }
    std::uint32_t InitializeManager() override {
        calls.emplace_back("manager");
        return 0x10u;
    }
    std::uint32_t PackageOpen(std::uint32_t manager,
                              const char* path) override {
        Check(manager == 0x5009a0u &&
                  std::string(path) == "ui/menu.xml",
              "package open uses adjusted manager and resolved path");
        calls.emplace_back("package_open");
        return package_handle;
    }
    std::uint32_t PackageSize(std::uint32_t handle) override {
        Check(handle == 0x3000u,
              "package size reads opened handle +36");
        calls.emplace_back("package_size");
        return 41u;
    }
    std::uint32_t CreateBuffer(std::uint32_t provider,
                               std::uint32_t size) override {
        Check(provider == 0x5000u &&
                  size == (use_package ? 41u : 77u),
              "provider buffer receives selected stream length");
        calls.emplace_back("create_buffer");
        buffer_size = size;
        return 0x4000u;
    }
    bool PackageRead(std::uint32_t manager,
                      std::uint32_t buffer, std::uint32_t bytes,
                      std::uint32_t handle) override {
        Check(manager == 0x5009a0u && buffer == 0x4000u &&
                  bytes == 41u && handle == 0x3000u,
              "package read receives opened handle and exact size");
        Check(output->ReadDword(4u) == 0xa1a1u &&
                  output->ReadDword(8u) == 0xb2b2u,
              "output remains untouched until package read succeeds");
        calls.emplace_back("package_read");
        if (throw_on_read) {
            throw std::runtime_error("read failed");
        }
        return package_read;
    }
    void Free(std::uint32_t buffer) override {
        Check(buffer == 0x4000u, "failed read frees provider buffer");
        calls.emplace_back("free");
    }
    void PackageClose(std::uint32_t manager,
                      std::uint32_t handle) override {
        Check(manager == 0x5009a0u && handle == 0x3000u,
              "package close uses adjusted manager and opened handle");
        calls.emplace_back("package_close");
    }
    bool LocalOpen(std::uint32_t utf8) override {
        Check(utf8 == 0x2000u,
              "local file opening receives UTF-8 resource name");
        calls.emplace_back("local_open");
        return local_open;
    }
    std::uint32_t LocalSize() override {
        calls.emplace_back("local_size");
        return 77u;
    }
    void LocalRead(std::uint32_t buffer,
                   std::uint32_t size) override {
        Check(buffer == 0x4000u && size == 77u,
              "local read uses allocated provider buffer and file length");
        calls.emplace_back("local_read");
        if (throw_on_local_read) {
            throw std::runtime_error("local read failed");
        }
    }
    bool LocalClose() override {
        calls.emplace_back("local_close");
        return local_close;
    }
};

pal4::CeguiRawDataContainer InitialOutput() {
    pal4::CeguiRawDataContainer output;
    output.WriteDword(0u, 0xc0c0u);
    output.WriteDword(4u, 0xa1a1u);
    output.WriteDword(8u, 0xb2b2u);
    return output;
}

void EmptyStorageThrowsBeforeEqualityCheck() {
    auto output = InitialOutput();
    Runtime runtime;
    runtime.output = &output;
    runtime.string_storage = 0u;
    bool caught = false;
    try {
        pal4::CeguiResourceLoader::Load(
            0x5000u, 0x1000u, output, 0u, runtime);
    } catch (const pal4::CeguiInvalidRequest& error) {
        caught = error.GetReason() ==
                 pal4::CeguiInvalidRequest::Reason::EmptyName;
    }
    Check(caught && runtime.calls == std::vector<std::string>{"storage"} &&
              output.ReadDword(4u) == 0xa1a1u,
          "null string storage throws before CEGUI equality");
}

void EmptyNameEqualityThrowsBeforeModeCheck() {
    auto output = InitialOutput();
    Runtime runtime;
    runtime.output = &output;
    runtime.equals_empty = true;
    bool caught = false;
    try {
        pal4::CeguiResourceLoader::Load(
            0x5000u, 0x1000u, output, 0u, runtime);
    } catch (const pal4::CeguiInvalidRequest& error) {
        caught = error.GetReason() ==
                 pal4::CeguiInvalidRequest::Reason::EmptyName;
    }
    Check(caught && runtime.calls == std::vector<std::string>{
              "storage", "equals_empty"},
          "empty CEGUI String throws before package flag read");
}

void PackageSuccessPublishesBufferAndClosesHandle() {
    auto output = InitialOutput();
    Runtime runtime;
    runtime.output = &output;
    pal4::CeguiResourceLoader::Load(
        0x5000u, 0x1000u, output, 0xdeadbeefu, runtime);
    Check(output.ReadDword(0u) == 0xc0c0u &&
              output.ReadDword(4u) == 0x4000u &&
              output.ReadDword(8u) == 41u &&
              runtime.calls == std::vector<std::string>{
                  "storage", "equals_empty", "package_mode", "utf8",
                  "resolve_path", "manager", "package_open", "manager",
                  "package_size", "create_buffer", "manager",
                  "package_read", "manager", "package_close"},
          "package route reads, publishes and closes in observed order");
}

void PackageReadFailureFreesBufferAndPreservesOutput() {
    auto output = InitialOutput();
    Runtime runtime;
    runtime.output = &output;
    runtime.package_read = false;
    pal4::CeguiResourceLoader::Load(
        0x5000u, 0x1000u, output, 0u, runtime);
    Check(output.ReadDword(4u) == 0xa1a1u &&
              output.ReadDword(8u) == 0xb2b2u &&
              runtime.calls[runtime.calls.size() - 3u] == "free" &&
              runtime.calls.back() == "package_close",
          "failed package read frees allocation without publishing it");
}

void PackageOpenFailureIsReported() {
    auto output = InitialOutput();
    Runtime runtime;
    runtime.output = &output;
    runtime.package_handle = 0u;
    bool caught = false;
    try {
        pal4::CeguiResourceLoader::Load(
            0x5000u, 0x1000u, output, 0u, runtime);
    } catch (const pal4::CeguiInvalidRequest& error) {
        caught = error.GetReason() ==
                 pal4::CeguiInvalidRequest::Reason::MissingFile;
    }
    Check(caught && runtime.calls.back() == "package_open" &&
              output.ReadDword(4u) == 0xa1a1u,
          "C++ boundary reports missing package file before null dereference");
}

void LocalSuccessPublishesSizeAndBuffer() {
    auto output = InitialOutput();
    Runtime runtime;
    runtime.output = &output;
    runtime.use_package = false;
    runtime.local_close = false;
    pal4::CeguiResourceLoader::Load(
        0x5000u, 0x1000u, output, 0u, runtime);
    Check(output.ReadDword(0u) == 0xc0c0u &&
              output.ReadDword(4u) == 0x4000u &&
              output.ReadDword(8u) == 77u &&
              runtime.calls == std::vector<std::string>{
                  "storage", "equals_empty", "package_mode", "utf8",
                  "local_open", "local_size", "create_buffer",
                  "local_read", "local_close"},
          "local route publishes data even when file close reports failure");
}

void LocalMissingFileThrowsAndPreservesOutput() {
    auto output = InitialOutput();
    Runtime runtime;
    runtime.output = &output;
    runtime.use_package = false;
    runtime.local_open = false;
    bool caught = false;
    try {
        pal4::CeguiResourceLoader::Load(
            0x5000u, 0x1000u, output, 0u, runtime);
    } catch (const pal4::CeguiInvalidRequest& error) {
        caught = error.GetReason() ==
                 pal4::CeguiInvalidRequest::Reason::MissingFile;
    }
    Check(caught && runtime.calls.back() == "local_open" &&
              output.ReadDword(4u) == 0xa1a1u,
          "missing local file throws before allocation");
}

void PackageReadExceptionClosesHandle() {
    auto output = InitialOutput();
    Runtime runtime;
    runtime.output = &output;
    runtime.throw_on_read = true;
    bool caught = false;
    try {
        pal4::CeguiResourceLoader::Load(
            0x5000u, 0x1000u, output, 0u, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls[runtime.calls.size() - 3u] == "free" &&
              runtime.calls.back() == "package_close" &&
              output.ReadDword(4u) == 0xa1a1u,
          "exception path frees buffer and closes package handle");
}

void LocalReadExceptionClosesAndFrees() {
    auto output = InitialOutput();
    Runtime runtime;
    runtime.output = &output;
    runtime.use_package = false;
    runtime.throw_on_local_read = true;
    bool caught = false;
    try {
        pal4::CeguiResourceLoader::Load(
            0x5000u, 0x1000u, output, 0u, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls[runtime.calls.size() - 2u] == "free" &&
              runtime.calls.back() == "local_close" &&
              output.ReadDword(4u) == 0xa1a1u &&
              output.ReadDword(8u) == 0xb2b2u,
          "local read exception frees buffer, closes stream and preserves output");
}

}  // namespace

int main() {
    EmptyStorageThrowsBeforeEqualityCheck();
    EmptyNameEqualityThrowsBeforeModeCheck();
    PackageSuccessPublishesBufferAndClosesHandle();
    PackageReadFailureFreesBufferAndPreservesOutput();
    PackageOpenFailureIsReported();
    LocalSuccessPublishesSizeAndBuffer();
    LocalMissingFileThrowsAndPreservesOutput();
    PackageReadExceptionClosesHandle();
    LocalReadExceptionClosesAndFrees();
    std::cout << "9 CEGUI resource loader test cases passed\n";
}
