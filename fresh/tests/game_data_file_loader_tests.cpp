#include "pal4/game_data_file_loader.h"

#include <algorithm>
#include <cstdint>
#include <deque>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataFileLoaderRuntime {
    std::deque<std::string> texts{
        "HEADER", "SCP_A", "SCP_B", "Equip", "EquipRelation"};
    std::deque<std::uint32_t> words{20060228, 2, 2};
    std::vector<std::string> calls;
    bool bootstrap_enabled = true;
    std::uint32_t bootstrap_result = 0x2000;
    std::uint32_t open_result = 0x3000;
    std::string failed_script;
    std::string failed_table;
    std::uint32_t throw_on_text = 0;
    std::uint32_t text_reads = 0;

    bool LoadBootstrapEnabled() override {
        calls.emplace_back("bootstrap enabled");
        return bootstrap_enabled;
    }
    std::uint32_t LoadBootstrapScript(std::string_view path) override {
        Check(path == "gamedata\\database.cpk", "bootstrap archive path");
        calls.emplace_back("load bootstrap");
        return bootstrap_result;
    }
    void ProcessBootstrapScript(std::uint32_t script) override {
        Check(script == 0x2000, "bootstrap script handle");
        calls.emplace_back("process bootstrap");
    }
    void CleanupAllScripts() override {
        calls.emplace_back("cleanup scripts");
    }
    void DestroyBootstrapScript(std::uint32_t script) override {
        Check(script == 0x2000, "destroy script handle");
        calls.emplace_back("destroy bootstrap");
    }
    std::uint32_t OpenFile(const char* path,
                           std::string_view mode) override {
        Check(std::string_view(path) == "data.bin" && mode == "rb",
              "binary file path and read mode");
        calls.emplace_back("open");
        return open_result;
    }
    void CloseFile(std::uint32_t file) override {
        Check(file == 0x3000, "close file handle");
        calls.emplace_back("close");
    }
    std::uint32_t CreateStream(std::uint32_t file) override {
        Check(file == 0x3000, "stream source file");
        calls.emplace_back("create stream");
        return 0x4000;
    }
    void DestroyStream(std::uint32_t stream) override {
        Check(stream == 0x4000, "destroy stream handle");
        calls.emplace_back("destroy stream");
    }
    std::string ReadText(std::uint32_t stream) override {
        Check(stream == 0x4000 && !texts.empty(), "text stream state");
        ++text_reads;
        if (text_reads == throw_on_text)
            throw std::runtime_error("stream read failed");
        auto value = texts.front();
        texts.pop_front();
        calls.push_back("text " + value);
        return value;
    }
    std::uint32_t ReadWord(std::uint32_t stream) override {
        Check(stream == 0x4000 && !words.empty(), "word stream state");
        const auto value = words.front();
        words.pop_front();
        calls.push_back("word " + std::to_string(value));
        return value;
    }
    std::string_view ExpectedSignature() override {
        calls.emplace_back("expected signature");
        return "HEADER";
    }
    bool LoadScriptEntry(std::string_view name, std::uint32_t database,
                         std::uint32_t stream,
                         std::uint32_t context) override {
        Check(database == 0x5000 && stream == 0x4000 &&
                  context == 0x6000,
              "script load context");
        calls.push_back("script " + std::string(name));
        return name != failed_script;
    }
    bool LoadTableEntry(std::string_view name, std::uint32_t database,
                        std::uint32_t stream,
                        std::uint32_t context) override {
        Check(database == 0x5000 && stream == 0x4000 &&
                  context == 0x6000,
              "table load context");
        calls.push_back("table " + std::string(name));
        return name != failed_table;
    }
    void RemoveTableEntry(std::uint32_t database,
                          std::string_view name) override {
        Check(database == 0x5000, "remove database pointer");
        calls.push_back("remove " + std::string(name));
    }
};

std::size_t At(const Runtime& runtime, const std::string& call) {
    const auto found = std::find(runtime.calls.begin(), runtime.calls.end(),
                                 call);
    Check(found != runtime.calls.end(), "expected call missing");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void CheckSuccess() {
    Runtime runtime;
    Check(pal4::GameDataFileLoader::Load(
              "data.bin", 0x5000, 0x6000, runtime) &&
              runtime.calls == std::vector<std::string>{
                                   "bootstrap enabled", "load bootstrap",
                                   "process bootstrap", "open",
                                   "create stream", "text HEADER",
                                   "expected signature", "word 20060228",
                                   "word 2", "text SCP_A", "script SCP_A",
                                   "text SCP_B", "script SCP_B", "word 2",
                                   "text Equip", "table Equip",
                                   "text EquipRelation", "table EquipRelation",
                                   "destroy stream", "close",
                                   "cleanup scripts", "destroy bootstrap"},
          "signature, version, scripts and tables load in file order");

    Runtime no_bootstrap;
    no_bootstrap.bootstrap_enabled = false;
    no_bootstrap.words = {20060228, 0, 0};
    no_bootstrap.texts = {"HEADER"};
    Check(pal4::GameDataFileLoader::Load(
              "data.bin", 0x5000, 0x6000, no_bootstrap) &&
              no_bootstrap.calls.back() == "close" &&
              std::find(no_bootstrap.calls.begin(),
                        no_bootstrap.calls.end(),
                        "load bootstrap") == no_bootstrap.calls.end(),
          "disabled bootstrap still loads empty database");
}

void CheckEarlyFailures() {
    Runtime no_path;
    Check(!pal4::GameDataFileLoader::Load(
              nullptr, 0x5000, 0x6000, no_path) &&
              no_path.calls == std::vector<std::string>{
                                   "bootstrap enabled", "load bootstrap",
                                   "process bootstrap", "cleanup scripts",
                                   "destroy bootstrap"},
          "null path cleans bootstrap without opening a file");

    Runtime no_open;
    no_open.open_result = 0;
    Check(!pal4::GameDataFileLoader::Load(
              "data.bin", 0x5000, 0x6000, no_open) &&
              no_open.calls.back() == "destroy bootstrap" &&
              std::find(no_open.calls.begin(), no_open.calls.end(),
                        "create stream") == no_open.calls.end(),
          "open failure cleans bootstrap before stream setup");

    Runtime no_archive;
    no_archive.bootstrap_result = 0;
    no_archive.words = {20060228, 0, 0};
    no_archive.texts = {"HEADER"};
    Check(pal4::GameDataFileLoader::Load(
              "data.bin", 0x5000, 0x6000, no_archive) &&
              std::find(no_archive.calls.begin(), no_archive.calls.end(),
                        "process bootstrap") == no_archive.calls.end() &&
              no_archive.calls.back() == "close",
          "missing optional archive does not prevent database load");
}

void CheckHeaderFailures() {
    Runtime signature;
    signature.texts.front() = "WRONG";
    Check(!pal4::GameDataFileLoader::Load(
              "data.bin", 0x5000, 0x6000, signature) &&
              At(signature, "text WRONG") <
                  At(signature, "destroy stream") &&
              signature.calls.back() == "destroy bootstrap" &&
              signature.words.size() == 3,
          "signature failure reads no version and closes resources");

    Runtime version;
    version.words.front() = 20060227;
    Check(!pal4::GameDataFileLoader::Load(
              "data.bin", 0x5000, 0x6000, version) &&
              At(version, "word 20060227") <
                  At(version, "destroy stream") &&
              version.words.size() == 2 &&
              version.calls.back() == "destroy bootstrap",
          "version mismatch stops before item counts");
}

void CheckItemFailures() {
    Runtime script;
    script.failed_script = "SCP_B";
    Check(!pal4::GameDataFileLoader::Load(
              "data.bin", 0x5000, 0x6000, script) &&
              At(script, "script SCP_A") < At(script, "script SCP_B") &&
              At(script, "script SCP_B") <
                  At(script, "destroy stream") &&
              std::find(script.calls.begin(), script.calls.end(),
                        "table Equip") == script.calls.end(),
          "failed script stops before table count and closes resources");

    Runtime table;
    table.failed_table = "EquipRelation";
    Check(!pal4::GameDataFileLoader::Load(
              "data.bin", 0x5000, 0x6000, table) &&
              At(table, "table Equip") <
                  At(table, "table EquipRelation") &&
              At(table, "table EquipRelation") <
                  At(table, "remove EquipRelation") &&
              At(table, "remove EquipRelation") <
                  At(table, "destroy stream"),
          "failed table is removed before file and bootstrap cleanup");

    Runtime exception;
    exception.throw_on_text = 2;
    bool thrown = false;
    try {
        (void)pal4::GameDataFileLoader::Load(
            "data.bin", 0x5000, 0x6000, exception);
    } catch (const std::runtime_error& error) {
        thrown = std::string_view(error.what()) == "stream read failed";
    }
    Check(thrown && exception.calls.back() == "destroy bootstrap" &&
              At(exception, "destroy stream") < At(exception, "close"),
          "stream exception still releases all acquired resources");
}

}  // namespace

int main() {
    try {
        CheckSuccess();
        CheckEarlyFailures();
        CheckHeaderFailures();
        CheckItemFailures();
        std::cout << "game_data_file_loader_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
