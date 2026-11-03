#pragma once

#include "pal4/package_resource_manager.h"
#include "pal4/script_manager_core.h"

#include <cstdint>

namespace pal4 {

class IPkScriptFacadeRuntime {
public:
    virtual ~IPkScriptFacadeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t InitializeManager() = 0;
    [[nodiscard]] virtual std::uint32_t GetScriptManager(
        std::uint32_t manager) = 0;
    [[nodiscard]] virtual IScriptManagerCoreRuntime& Scripts() = 0;
    [[nodiscard]] virtual IPackageResourceManagerRuntime& Resources() = 0;
};

class PkScriptFacadeRuntimeStub final : public IPkScriptFacadeRuntime {
public:
    [[nodiscard]] std::uint32_t InitializeManager() override { return 0; }
    [[nodiscard]] std::uint32_t GetScriptManager(
        std::uint32_t) override { return 0; }
    [[nodiscard]] IScriptManagerCoreRuntime& Scripts() override {
        return scripts_;
    }
    [[nodiscard]] IPackageResourceManagerRuntime& Resources() override {
        return resources_;
    }

private:
    ScriptManagerCoreRuntimeStub scripts_{};
    PackageResourceManagerRuntimeStub resources_{};
};

class PkScriptFacade final {
public:
    [[nodiscard]] static std::uint32_t LoadByName(
        std::uint32_t unused, std::uint32_t key,
        const char* path, IPkScriptFacadeRuntime& runtime);
    [[nodiscard]] static std::uint32_t Unload(
        std::uint32_t key, IPkScriptFacadeRuntime& runtime);
    [[nodiscard]] static std::uint32_t Read(
        std::uint32_t key, void* buffer,
        std::uint32_t bytes, IPkScriptFacadeRuntime& runtime);
    [[nodiscard]] static bool IsLoaded(
        std::uint32_t key, IPkScriptFacadeRuntime& runtime);
    [[nodiscard]] static std::uint64_t Seek(
        std::uint32_t key, std::int32_t offset,
        std::uint32_t mode, IPkScriptFacadeRuntime& runtime);
    [[nodiscard]] static std::uint8_t FileExists(
        std::uint32_t unused, const char* path,
        IPkScriptFacadeRuntime& runtime);
};

}  // namespace pal4
