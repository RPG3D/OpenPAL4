#include "pal4/xml_config_file_loader.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IXmlConfigFileLoadRuntime {
    std::vector<std::string> events;
    std::vector<std::int32_t> versions{1, 1};
    std::size_t version_index = 0u;
    bool open_result = true;
    bool failed = false;
    bool close_result = true;
    bool throw_during_read = false;
    std::uint8_t node_result = 7u;

    void ConstructStream() override { events.emplace_back("construct"); }
    bool OpenInput(const std::uint32_t filename,
                   const std::uint32_t mode) override {
        Check(filename == 0x4000u && mode == 33u,
              "input open uses original filename and mode 33");
        events.emplace_back("open");
        return open_result;
    }
    void SetStreamError(const std::uint32_t flags) override {
        Check(flags == 2u, "open and close errors set stream flag two");
        events.emplace_back("error");
        failed = true;
    }
    bool StreamFailed() override {
        events.emplace_back("failed");
        return failed;
    }
    std::int32_t ReadVersionWord() override {
        events.emplace_back("version");
        Check(version_index < versions.size(), "version header has two words");
        return versions[version_index++];
    }
    std::uint8_t ReadNodeData(const std::uint32_t parent,
                              const std::uint32_t config) override {
        Check(parent == 0u && config == 0x6000u,
              "XML root read uses null parent and supplied config");
        events.emplace_back("node");
        if (throw_during_read) throw std::runtime_error("read failed");
        return node_result;
    }
    bool CloseInput() override {
        events.emplace_back("close");
        return close_result;
    }
    void ResetFileBuffer() override { events.emplace_back("reset"); }
    void DestroyStream() override { events.emplace_back("destroy"); }
};

void TestNullDestinationAndOpenFailure() {
    Runtime null_config;
    Check(pal4::XmlConfigFileLoader::Load(
              0x4000u, 0u, null_config) == 0u &&
              null_config.events.empty(),
          "null config returns before stream construction");

    Runtime open_failure;
    open_failure.open_result = false;
    Check(pal4::XmlConfigFileLoader::Load(
              0x4000u, 0x6000u, open_failure) == 0u &&
              open_failure.events ==
                  (std::vector<std::string>{
                      "construct", "open", "error", "failed", "destroy"}),
          "failed open sets stream error and cleans up without reading");

    Runtime bad_state;
    bad_state.failed = true;
    Check(pal4::XmlConfigFileLoader::Load(
              0x4000u, 0x6000u, bad_state) == 0u &&
              bad_state.events ==
                  (std::vector<std::string>{
                      "construct", "open", "failed", "destroy"}),
          "stream state failure stops before version reads");
}

void TestVersionGate() {
    for (const std::vector<std::int32_t> versions : {
             std::vector<std::int32_t>{2, 0},
             std::vector<std::int32_t>{1, 2}}) {
        Runtime unsupported;
        unsupported.versions = versions;
        Check(pal4::XmlConfigFileLoader::Load(
                  0x4000u, 0x6000u, unsupported) == 0u &&
                  unsupported.events ==
                      (std::vector<std::string>{
                          "construct", "open", "failed", "version",
                          "version", "destroy"}),
              "unsupported major or minor version skips XML read and close");
    }
    Runtime older;
    older.versions = {0, 999};
    Check(pal4::XmlConfigFileLoader::Load(
              0x4000u, 0x6000u, older) == 7u &&
              older.version_index == 2u,
          "major zero accepts any minor version");
}

void TestReadCloseAndExceptionCleanup() {
    Runtime success;
    Check(pal4::XmlConfigFileLoader::Load(
              0x4000u, 0x6000u, success) == 7u &&
              success.events ==
                  (std::vector<std::string>{
                      "construct", "open", "failed", "version", "version",
                      "node", "close", "reset", "destroy"}),
          "successful load reads XML then closes and resets buffer");

    Runtime close_failure;
    close_failure.close_result = false;
    close_failure.node_result = 9u;
    Check(pal4::XmlConfigFileLoader::Load(
              0x4000u, 0x6000u, close_failure) == 9u &&
              close_failure.events ==
                  (std::vector<std::string>{
                      "construct", "open", "failed", "version", "version",
                      "node", "close", "error", "destroy"}),
          "close failure flags stream but retains XML read result");

    Runtime thrown;
    thrown.throw_during_read = true;
    bool caught = false;
    try {
        (void)pal4::XmlConfigFileLoader::Load(
            0x4000u, 0x6000u, thrown);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && thrown.events.back() == "destroy",
          "stream cleanup runs when XML parser throws");

    pal4::XmlConfigFileLoadRuntimeStub stub;
    Check(pal4::XmlConfigFileLoader::Load(0u, 0x6000u, stub) == 0u,
          "linkable file and XML runtime stub fails closed");
}

}  // namespace

int main() {
    TestNullDestinationAndOpenFailure();
    TestVersionGate();
    TestReadCloseAndExceptionCleanup();
}
