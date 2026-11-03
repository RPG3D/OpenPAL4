#pragma once

#include "pal4/npc_info_manager.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IScriptCloudRuntime : public NpcInfoRuntimeStub {
public:
    [[nodiscard]] virtual std::uint32_t RegisterSingleton(
        std::string_view name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t deleter) = 0;
    virtual void CreateXmlNode(std::uint32_t document) = 0;
    virtual void ProcessCameraScripts() = 0;
    virtual void CleanupCameraScripts() = 0;
    [[nodiscard]] virtual std::uint32_t OpenCameraFile(
        std::string_view world, std::string_view scene,
        std::string_view camera) = 0;
};

class ScriptCloudRuntimeStub : public IScriptCloudRuntime {
public:
    std::uint32_t RegisterSingleton(std::string_view, std::uint32_t,
                                    std::uint32_t, std::uint32_t) override {
        return 0;
    }
    void CreateXmlNode(std::uint32_t) override {}
    void ProcessCameraScripts() override {}
    void CleanupCameraScripts() override {}
    std::uint32_t OpenCameraFile(std::string_view, std::string_view,
                                 std::string_view) override { return 0; }
};

class CameraRecord final {
public:
    static constexpr std::uint32_t kBytes = 0x44u;
    static void Initialize(std::uint32_t record, IScriptCloudRuntime& runtime);
    static void Cleanup(std::uint32_t record, IScriptCloudRuntime& runtime);
    static void Read(std::uint32_t record, std::uint32_t stream,
                     IScriptCloudRuntime& runtime);
};

struct ScriptCloudSharedTree {
    std::uint32_t sentinel = 0;
    std::uint32_t instances = 0;
};

class ScriptCloudManager final {
public:
    static constexpr std::string_view kSingletonName = "scriptCLDMgr";
    static constexpr std::uint32_t kSingletonId = 1038u;

    ScriptCloudManager(std::uint32_t address, ScriptCloudSharedTree& shared,
                       IScriptCloudRuntime& runtime);
    [[nodiscard]] static std::uint32_t Register(
        std::uint8_t& guard, std::uint32_t& registered,
        IScriptCloudRuntime& runtime);
    [[nodiscard]] bool Create();
    [[nodiscard]] bool Initialize();
    void Cleanup();
    void Destroy();
    [[nodiscard]] std::uint32_t Find(std::string_view name) const;
    void Clear();
    [[nodiscard]] std::uint32_t First();
    [[nodiscard]] std::uint32_t Next();
    [[nodiscard]] bool Load(std::string_view world, std::string_view scene,
                            std::string_view camera);

private:
    [[nodiscard]] bool Insert(std::string name, std::uint32_t record);
    std::uint32_t address_;
    ScriptCloudSharedTree& shared_;
    IScriptCloudRuntime& runtime_;
    std::uint32_t cursor_ = 0;
    bool initialized_ = false;
    bool owned_ = false;
};

}  // namespace pal4
