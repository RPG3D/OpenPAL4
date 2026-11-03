#include "pal4/script_cloud_manager.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(const bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::ScriptCloudRuntimeStub {
    std::uint32_t next = 0x1000;
    std::uint32_t fail_size = 0;
    std::uint32_t stream = 0;
    std::uint32_t stream_count = 0;
    std::uint32_t stream_kind = 0;
    std::uint32_t registered = 0xeeee;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::string> strings;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> frees;
    std::vector<std::uint32_t> xml_nodes;
    std::vector<std::uint32_t> destroyed_xml;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> reads;
    std::vector<std::string> input_names;
    std::vector<std::string> events;
    std::string registration_name;
    std::vector<std::uint32_t> registration_arguments;
    std::string opened_world;
    std::string opened_scene;
    std::string opened_camera;
    std::uint32_t closed_stream = 0;
    std::uint32_t xml_checks = 0;
    std::uint32_t facet_cleanup = 0;

    std::uint32_t Allocate(const std::uint32_t size) override {
        allocations.push_back(size);
        if (size == fail_size) return 0;
        const auto value = next;
        next += size + 0x100;
        return value;
    }
    void Free(const std::uint32_t address) override { frees.push_back(address); }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override { words[address] = value; }
    void InitializeString(const std::uint32_t address) override {
        strings[address].clear();
    }
    void AssignString(const std::uint32_t address,
                      const std::string_view value) override {
        strings[address] = value;
    }
    std::string ReadStringValue(const std::uint32_t address) override {
        return strings[address];
    }
    void CleanupString(const std::uint32_t address) override {
        strings.erase(address);
        events.push_back("cleanup-text");
    }
    void ReadStreamString(std::uint32_t,
                          const std::uint32_t address) override {
        if (input_names.empty()) return;
        strings[address] = input_names.front();
        input_names.erase(input_names.begin());
    }
    void ReadStreamData(std::uint32_t, const std::uint32_t address,
                        const std::uint32_t size) override {
        reads.emplace_back(address, size);
    }
    std::uint32_t ReadStreamWord(std::uint32_t) override { return stream_count; }
    std::uint32_t StreamKind(std::uint32_t) override { return stream_kind; }
    std::uint32_t StreamLocale(std::uint32_t) override { return 10; }
    std::uint32_t CreateLocaleFacet(const std::uint32_t locale) override {
        return locale + 1;
    }
    void DestroyLocaleFacet(std::uint32_t) override { ++facet_cleanup; }
    void CheckXml(std::uint32_t, std::uint32_t) override { ++xml_checks; }
    std::uint32_t InitializeXml(const std::uint32_t storage,
                                std::uint32_t) override { return storage; }
    void DestroyXml(const std::uint32_t document) override {
        destroyed_xml.push_back(document);
    }
    void CreateXmlNode(const std::uint32_t document) override {
        xml_nodes.push_back(document);
    }
    std::uint32_t RegisterSingleton(
        const std::string_view name, const std::uint32_t id,
        const std::uint32_t factory, const std::uint32_t deleter) override {
        registration_name = name;
        registration_arguments = {id, factory, deleter};
        return registered;
    }
    void ProcessCameraScripts() override { events.push_back("process"); }
    void CleanupCameraScripts() override { events.push_back("cleanup"); }
    std::uint32_t OpenCameraFile(
        const std::string_view world, const std::string_view scene,
        const std::string_view camera) override {
        events.push_back("open");
        opened_world = world;
        opened_scene = scene;
        opened_camera = camera;
        return stream;
    }
    void CloseStream(const std::uint32_t value) override {
        events.push_back("close");
        closed_stream = value;
    }
};

void RecordLifecycle() {
    Runtime runtime;
    constexpr auto record = 0x8000u;
    pal4::CameraRecord::Initialize(record, runtime);
    const auto document = runtime.words[record + 64u];
    Check(runtime.strings.contains(record) &&
          runtime.words[record + 36u] == 0 &&
          runtime.words[record + 48u] == 1 &&
          runtime.words[record + 52u] == 0x3f800000u &&
          document != 0 && runtime.xml_nodes ==
              (std::vector<std::uint32_t>{document}),
          "68-byte camera defaults and XML root");
    runtime.stream_kind = 1;
    pal4::CameraRecord::Read(record, 0x9000, runtime);
    Check(runtime.reads.size() == 9 &&
          runtime.reads[0] == std::pair{record + 12u, 12u} &&
          runtime.reads[1] == std::pair{record + 24u, 12u} &&
          runtime.reads[8] == std::pair{record + 60u, 4u} &&
          runtime.xml_checks == 1 && runtime.facet_cleanup == 1,
          "camera stream layout and facet branch");
    runtime.stream_kind = 0;
    pal4::CameraRecord::Read(record, 0x9000, runtime);
    Check(runtime.xml_checks == 1 && runtime.facet_cleanup == 1,
          "plain stream skips facet");
    pal4::CameraRecord::Cleanup(record, runtime);
    Check(runtime.destroyed_xml ==
              (std::vector<std::uint32_t>{document}) &&
          runtime.frees == (std::vector<std::uint32_t>{document}) &&
          runtime.words[record + 64u] == 0,
          "document and text cleanup");
}

void RegistrationAndTree() {
    Runtime runtime;
    std::uint8_t guard = 0;
    std::uint32_t registered = 0;
    Check(pal4::ScriptCloudManager::Register(guard, registered, runtime) ==
              runtime.registered &&
          pal4::ScriptCloudManager::Register(guard, registered, runtime) ==
              runtime.registered &&
          guard == 1 && runtime.registration_name == "scriptCLDMgr" &&
          runtime.registration_arguments ==
              (std::vector<std::uint32_t>{1038, 0x524c90, 0x524cf0}),
          "guarded singleton registration");
    pal4::ScriptCloudSharedTree shared;
    pal4::ScriptCloudManager manager(0, shared, runtime);
    Check(manager.Create() && shared.instances == 1 &&
          runtime.allocations ==
              (std::vector<std::uint32_t>{20, 36, 36}),
          "20-byte manager and shared nil");
    Check(manager.First() == 0 && manager.Next() == 0 &&
          manager.Find("absent") == 0,
          "empty tree traversal and lookup");
    runtime.events.clear();
    Check(!manager.Load("w", "s", "c") &&
          runtime.events ==
              (std::vector<std::string>{"process", "open", "cleanup"}) &&
          runtime.opened_world == "w" && runtime.opened_scene == "s" &&
          runtime.opened_camera == "c", "missing camera file cleanup");
    runtime.stream = 0x9000;
    runtime.stream_count = 2;
    runtime.input_names = {"beta", "alpha"};
    runtime.events.clear();
    Check(manager.Load("w", "s", "c") &&
          runtime.events.front() == "process" &&
          runtime.events[1] == "open" &&
          runtime.events[runtime.events.size() - 2] == "close" &&
          runtime.events.back() == "cleanup" &&
          runtime.closed_stream == runtime.stream &&
          manager.Find("alpha") && manager.Find("beta"),
          "two camera records and script processing order");
    const auto alpha = manager.Find("alpha");
    const auto beta = manager.Find("beta");
    Check(manager.First() == alpha && manager.Next() == beta &&
          manager.Next() == 0 && manager.Next() == 0,
          "camera first/next ordered iteration");
    runtime.stream_count = 0;
    Check(manager.Load("w", "s", "empty") &&
          manager.First() == 0 && manager.Find("alpha") == 0,
          "successful empty reload clears camera records");
    manager.Destroy();
    Check(shared.instances == 0 && shared.sentinel == 0 &&
          runtime.frees.back() != 0,
          "manager destruction releases tree and object");
}

void AllocationFailure() {
    Runtime runtime;
    pal4::ScriptCloudSharedTree shared;
    pal4::ScriptCloudManager manager(0, shared, runtime);
    runtime.fail_size = 20;
    Check(!manager.Create() && shared.instances == 0,
          "manager allocation failure");
    runtime.fail_size = 0;
    Check(manager.Create(), "manager creation retry");
    runtime.fail_size = 68;
    runtime.stream = 0x9000;
    runtime.stream_count = 1;
    Check(!manager.Load("w", "s", "c") &&
          runtime.closed_stream == runtime.stream &&
          runtime.events.back() == "cleanup",
          "record allocation failure closes stream and scripts");
    manager.Destroy();
}

}  // namespace

int main() {
    try {
        RecordLifecycle();
        RegistrationAndTree();
        AllocationFailure();
        std::cout << "script_cloud_manager_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
