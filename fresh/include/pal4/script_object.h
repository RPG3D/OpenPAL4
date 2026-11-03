#pragma once

#include <cstdint>
#include <optional>

namespace pal4 {

class IScriptObjectRuntime {
public:
    virtual ~IScriptObjectRuntime() = default;
    virtual void InitializeTaskContainer(std::uint32_t address) = 0;
    virtual std::uint32_t DestroyTaskContainer(std::uint32_t address) = 0;
};

class ScriptObjectRuntimeStub final : public IScriptObjectRuntime {
public:
    void InitializeTaskContainer(std::uint32_t address) override;
    std::uint32_t DestroyTaskContainer(std::uint32_t address) override;
};

class IScriptEventNodeRuntime {
public:
    virtual ~IScriptEventNodeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t NilAddress() = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CallFactory(
        std::uint32_t function_address) = 0;
    virtual void AddTaskAndInitBase(
        std::uint32_t owner, std::uint32_t task,
        std::uint32_t source, std::uint32_t timing) = 0;
};

class ScriptEventNodeRuntimeStub final : public IScriptEventNodeRuntime {
public:
    std::uint32_t NilAddress() override;
    std::uint32_t ReadDword(std::uint32_t address) override;
    std::uint32_t CallFactory(std::uint32_t function_address) override;
    void AddTaskAndInitBase(std::uint32_t owner, std::uint32_t task,
                            std::uint32_t source,
                            std::uint32_t timing) override;
};

class ScriptObject final {
public:
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t address, IScriptObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t address, IScriptObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t SearchEventNode(
        std::uint32_t tree, std::uint32_t class_id_address,
        IScriptEventNodeRuntime& runtime);
    // 现代注册表解析后复用原工厂调用/提交顺序；空optional表示未找到，值0仍调用工厂。
    [[nodiscard]] static std::uint32_t ProcessResolvedFactory(
        std::uint32_t owner,std::optional<std::uint32_t> factory,
        std::uint32_t source,std::uint32_t timing,IScriptEventNodeRuntime&);
    [[nodiscard]] static std::uint32_t ProcessEventData(
        std::uint32_t owner, std::uint32_t class_id_address,
        std::uint32_t event_data, std::uint32_t source,
        std::uint32_t timing, IScriptEventNodeRuntime& runtime);
};

}  // namespace pal4
