#include "pal4/pal_game_object_manager_load.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IPalGameObjectManagerLoadRuntime {
    static constexpr std::uint32_t manager = 0x1000u;
    static constexpr std::uint32_t stream = 0x2000u;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> stream_words{2u, 3u, 4u};
    std::map<std::uint32_t, std::uint32_t> factories{
        {3u, 0x3000u}, {4u, 0x4000u}};
    std::map<std::uint32_t, std::uint32_t> created{
        {0x3000u, 0x5000u}, {0x4000u, 0x6000u}};
    std::map<std::uint32_t, std::string> names{
        {0x5000u, "Bell"}, {0x6000u, "Gate"}};
    std::size_t read_index = 0u;
    std::uint32_t open_result = stream;
    std::uint32_t failed_factory = 0u;
    std::uint32_t failed_record = 0u;
    bool fail_insert = false;

    std::string_view BasePath() override {
        calls.emplace_back("base path");
        return "Root\\";
    }
    std::uint32_t OpenStream(std::string_view path,
                             std::uint32_t context_type,
                             std::uint32_t access_mode) override {
        Check(path == "Root\\N01\\S01\\GameObjs.gob" &&
              context_type == 2u && access_mode == 1u,
              "scene object path and stream mode");
        calls.emplace_back("open");
        return open_result;
    }
    bool ReadU32(std::uint32_t handle,
                 std::uint32_t& value) override {
        Check(handle == stream, "read stream handle");
        calls.emplace_back("read");
        if (read_index == stream_words.size()) return false;
        value = stream_words[read_index++];
        return true;
    }
    void CloseStream(std::uint32_t handle) override {
        Check(handle == stream, "close stream handle");
        calls.emplace_back("close");
    }
    void DeactivateAndClear(std::uint32_t address) override {
        Check(address == manager, "clear manager");
        calls.emplace_back("clear");
    }
    std::uint32_t FactoryForType(std::uint32_t address,
                                 std::uint32_t type) override {
        Check(address == manager, "factory manager");
        calls.emplace_back("factory " + std::to_string(type));
        if (type == failed_factory) return 0u;
        const auto it = factories.find(type);
        return it == factories.end() ? 0u : it->second;
    }
    std::uint32_t CreateObject(std::uint32_t factory) override {
        calls.emplace_back("create " + std::to_string(factory));
        return created.at(factory);
    }
    void DestroyObject(std::uint32_t object) override {
        calls.emplace_back("destroy " + std::to_string(object));
    }
    bool LoadRecord(std::uint32_t object,
                    std::uint32_t handle) override {
        Check(handle == stream, "record stream handle");
        calls.emplace_back("load " + std::to_string(object));
        return object != failed_record;
    }
    std::string_view ObjectName(std::uint32_t object) override {
        calls.emplace_back("name " + std::to_string(object));
        return names.at(object);
    }
    void InsertObject(std::uint32_t address,
                      std::string_view name,
                      std::uint32_t object) override {
        Check(address == manager && names.at(object) == name,
              "insert object arguments");
        calls.emplace_back("insert " + std::string(name));
        if (fail_insert) throw std::runtime_error("insert failed");
    }
};

bool Load(Runtime& runtime) {
    return pal4::PalGameObjectManagerLoad::LoadSceneObjects(
        Runtime::manager, "N01", "S01", runtime);
}

void CheckSuccess() {
    Runtime runtime;
    Check(Load(runtime) &&
          runtime.calls == (std::vector<std::string>{
              "base path", "open", "clear", "read", "read", "read",
              "factory 3", "create 12288", "load 20480", "name 20480",
              "insert Bell", "factory 4", "create 16384", "load 24576",
              "name 24576", "insert Gate", "close"}),
          "scene loading path, type table and object order");
    Runtime empty;
    empty.stream_words = {0u};
    Check(Load(empty) &&
          empty.calls == (std::vector<std::string>{
              "base path", "open", "clear", "read", "close"}),
          "empty scene object list");
}

void CheckFailures() {
    Runtime no_stream;
    no_stream.open_result = 0u;
    Check(!Load(no_stream) &&
          no_stream.calls == (std::vector<std::string>{
              "base path", "open"}), "open failure preserves old objects");
    Runtime truncated;
    truncated.stream_words = {2u, 3u};
    Check(!Load(truncated) && truncated.calls.back() == "close" &&
          truncated.calls.size() == 7u,
          "truncated type table closes stream before factories");
    Runtime no_count;
    no_count.stream_words.clear();
    Check(!Load(no_count) && no_count.calls.back() == "close",
          "missing count closes stream");
    Runtime no_factory;
    no_factory.failed_factory = 4u;
    Check(!Load(no_factory) && no_factory.calls.back() == "close" &&
          no_factory.calls[no_factory.calls.size() - 2u] == "factory 4",
          "missing factory stops after prior object");
    Runtime failed_record;
    failed_record.failed_record = 0x5000u;
    Check(!Load(failed_record) &&
          failed_record.calls == (std::vector<std::string>{
              "base path", "open", "clear", "read", "read", "read",
              "factory 3", "create 12288", "load 20480",
              "destroy 20480", "close"}),
          "failed record destroys uninserted object");
    Runtime failed_insert;
    failed_insert.fail_insert = true;
    try {
        static_cast<void>(Load(failed_insert));
        throw std::runtime_error("missing insert failure");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "insert failed" &&
              failed_insert.calls[failed_insert.calls.size() - 2u] ==
                  "destroy 20480" &&
              failed_insert.calls.back() == "close",
              "insert exception releases unowned object and stream");
    }
}

}  // namespace

int main() {
    try {
        CheckSuccess();
        CheckFailures();
        std::cout << "pal_game_object_manager_load_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
