#include "pal4/package_resource_manager.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {

struct Runtime final : pal4::IPackageResourceManagerRuntime {
    bool rewrite = false;
    std::vector<std::string> calls;

    bool HasPathRewrite() override {
        calls.emplace_back("has-rewrite");
        return rewrite;
    }
    void RewritePath(const char* source,
                     std::array<char, 256>& output) override {
        calls.emplace_back("rewrite:" + std::string(source));
        const std::string mapped = "package/" + std::string(source);
        std::copy(mapped.begin(), mapped.end(), output.begin());
    }
    std::uint32_t OpenNativeFile(const char* path) override {
        calls.emplace_back("open:" + std::string(path));
        return 0x1234u;
    }
    std::uint8_t NativeFileExists(const char* path) override {
        calls.emplace_back("exists:" + std::string(path));
        return 0x45u;
    }
};

int failures = 0;

void Check(bool condition, const char* message) {
    if (!condition) {
        ++failures;
        std::cerr << message << '\n';
    }
}

void TestResourcePaths() {
    Runtime runtime;
    Check(pal4::PackageResourceManager::OpenFile(
              "gamedata\\x.csb", runtime) == 0x1234u &&
              runtime.calls == std::vector<std::string>({
                  "has-rewrite", "open:gamedata\\x.csb"}),
          "open without callback passes original path");
    runtime.calls.clear();
    runtime.rewrite = true;
    Check(pal4::PackageResourceManager::OpenFile(
              "x.csb", runtime) == 0x1234u &&
              runtime.calls == std::vector<std::string>({
                  "has-rewrite", "rewrite:x.csb", "open:package/x.csb"}),
          "open rewrites path before native package call");
    runtime.calls.clear();
    Check(pal4::PackageResourceManager::FileExists(
              "x.csb", runtime) == 0x45u &&
              runtime.calls == std::vector<std::string>({
                  "has-rewrite", "rewrite:x.csb", "exists:package/x.csb"}),
          "existence query uses the same callback path");
    runtime.calls.clear();
    runtime.rewrite = false;
    Check(pal4::PackageResourceManager::FileExists(
              "x.csb", runtime) == 0x45u &&
              runtime.calls == std::vector<std::string>({
                  "has-rewrite", "exists:x.csb"}),
          "existence query without callback uses original path");
    runtime.calls.clear();
    Check(pal4::PackageResourceManager::OpenFile(nullptr, runtime) == 0u &&
              pal4::PackageResourceManager::FileExists(nullptr, runtime) ==
                  0u && runtime.calls.empty(),
          "null path is rejected safely");
}

}  // namespace

int main() {
    TestResourcePaths();
    return failures == 0 ? 0 : 1;
}
