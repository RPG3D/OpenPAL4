#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace pal4 {

enum class RwPlugin8451Callback {
    Identity,
    GeometryConstruct,
    GeometryDestruct,
    GeometryCopy,
    StreamSize,
    HideHookAtomic,
    AttachFrameToAtomic,
    QueryUserDataAtomic,
};

struct RwPlugin8451State final {
    std::int32_t engine_offset = -1;
    std::int32_t geometry_offset = -1;
};

class IRwPlugin8451Lifecycle {
public:
    virtual ~IRwPlugin8451Lifecycle() = default;
    [[nodiscard]] virtual std::int32_t RegisterEngine(
        std::uint32_t size, std::uint32_t id,
        RwPlugin8451Callback construct,
        RwPlugin8451Callback destruct) = 0;
    [[nodiscard]] virtual std::int32_t RegisterGeometry(
        std::uint32_t size, std::uint32_t id,
        RwPlugin8451Callback construct,
        RwPlugin8451Callback destruct,
        RwPlugin8451Callback copy) = 0;
    [[nodiscard]] virtual std::int32_t RegisterGeometryStream(
        std::uint32_t id, RwPlugin8451Callback read,
        RwPlugin8451Callback write,
        RwPlugin8451Callback size) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
};

class IRwPlugin8451Runtime : public IRwPlugin8451Lifecycle {
public:
    ~IRwPlugin8451Runtime() override = default;
    [[nodiscard]] virtual std::uint32_t ForAllAtomics(
        std::uint32_t clump, RwPlugin8451Callback callback,
        std::uint32_t user_data) = 0;
    [[nodiscard]] virtual std::int32_t UserDataCount(
        std::uint32_t geometry) = 0;
    [[nodiscard]] virtual std::uint32_t UserDataArray(
        std::uint32_t geometry, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::string_view UserDataText(
        std::uint32_t array, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t HookRenderCallbackAddress() = 0;
    [[nodiscard]] virtual std::uint32_t DefaultRenderCallbackAddress() = 0;
    [[nodiscard]] virtual std::uint32_t FrameCount(
        std::uint32_t frame) = 0;
    virtual void UpdateFrameObjects(std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t ForAllAtomicsWithAttachment(
        std::uint32_t clump, RwPlugin8451Callback callback,
        std::string_view name, std::uint32_t frame) = 0;
    virtual void RemoveFrameChild(std::uint32_t frame) = 0;
    virtual void AddFrameChild(std::uint32_t parent,
                               std::uint32_t child) = 0;
    virtual void ForAllAtomicsWithQuery(
        std::uint32_t clump, RwPlugin8451Callback callback,
        std::string_view name, std::uint32_t& output) = 0;
};

class RwPlugin8451RuntimeStub final : public IRwPlugin8451Runtime {
public:
    std::int32_t RegisterEngine(
        std::uint32_t, std::uint32_t,
        RwPlugin8451Callback, RwPlugin8451Callback) override { return -1; }
    std::int32_t RegisterGeometry(
        std::uint32_t, std::uint32_t, RwPlugin8451Callback,
        RwPlugin8451Callback, RwPlugin8451Callback) override { return -1; }
    std::int32_t RegisterGeometryStream(
        std::uint32_t, RwPlugin8451Callback,
        RwPlugin8451Callback, RwPlugin8451Callback) override { return -1; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    std::uint32_t ReadWord(std::uint32_t) override { return 0; }
    std::uint32_t ForAllAtomics(
        std::uint32_t, RwPlugin8451Callback,
        std::uint32_t) override { return 0; }
    std::int32_t UserDataCount(std::uint32_t) override { return 0; }
    std::uint32_t UserDataArray(
        std::uint32_t, std::uint32_t) override { return 0; }
    std::string_view UserDataText(
        std::uint32_t, std::uint32_t) override { return {}; }
    std::uint32_t HookRenderCallbackAddress() override { return 0; }
    std::uint32_t DefaultRenderCallbackAddress() override { return 0; }
    std::uint32_t FrameCount(std::uint32_t) override { return 0; }
    void UpdateFrameObjects(std::uint32_t) override {}
    std::uint32_t ForAllAtomicsWithAttachment(
        std::uint32_t, RwPlugin8451Callback,
        std::string_view, std::uint32_t) override { return 0; }
    void RemoveFrameChild(std::uint32_t) override {}
    void AddFrameChild(std::uint32_t, std::uint32_t) override {}
    void ForAllAtomicsWithQuery(
        std::uint32_t, RwPlugin8451Callback,
        std::string_view, std::uint32_t&) override {}
};

class RwPlugin8451 final {
public:
    [[nodiscard]] static bool Register(RwPlugin8451State& state,
                                       IRwPlugin8451Lifecycle& runtime);
    [[nodiscard]] static std::uint32_t GeometryConstruct(
        std::uint32_t object, const RwPlugin8451State& state,
        IRwPlugin8451Lifecycle& runtime);
    [[nodiscard]] static std::uint32_t GeometryDestruct(
        std::uint32_t object, const RwPlugin8451State& state,
        IRwPlugin8451Lifecycle& runtime);
    [[nodiscard]] static std::uint32_t GeometryCopy(
        std::uint32_t target, std::uint32_t source,
        const RwPlugin8451State& state, IRwPlugin8451Lifecycle& runtime);
    [[nodiscard]] static std::uint32_t PluginSize();
    [[nodiscard]] static std::uint32_t HideClumpHookAtomics(
        std::uint32_t clump, IRwPlugin8451Runtime& runtime);
    [[nodiscard]] static std::uint32_t HideHookAtomic(
        std::uint32_t atomic, const RwPlugin8451State& state,
        IRwPlugin8451Runtime& runtime);
    [[nodiscard]] static std::uint32_t RenderHiddenAtomic(
        std::uint32_t atomic, IRwPlugin8451Runtime& runtime);
    [[nodiscard]] static std::uint32_t ProcessClumpUserData(
        std::uint32_t clump, std::string_view name,
        std::uint32_t frame, IRwPlugin8451Runtime& runtime);
    [[nodiscard]] static std::uint32_t AttachFrameToMatchingAtomic(
        std::uint32_t atomic, std::string_view name,
        std::uint32_t frame, const RwPlugin8451State& state,
        IRwPlugin8451Runtime& runtime);
    [[nodiscard]] static std::uint32_t RemoveFrameChildAndClear(
        std::uint32_t clump, std::string_view name,
        std::uint32_t frame, IRwPlugin8451Runtime& runtime);
    [[nodiscard]] static std::uint32_t QueryClumpUserData(
        std::uint32_t clump, std::optional<std::string_view> name,
        IRwPlugin8451Runtime& runtime);
    [[nodiscard]] static std::uint32_t QueryAtomicUserData(
        std::uint32_t atomic, std::string_view name,
        std::uint32_t& output, IRwPlugin8451Runtime& runtime);
};

}  // namespace pal4
