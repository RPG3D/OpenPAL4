#pragma once

#include <cstdint>
#include <initializer_list>
#include <string_view>

namespace pal4 {

enum class WorldPathCall : std::uint8_t {
    ReadString,
    ConstructGrid,
    InitializeGrid,
    DestroyGrid,
    CleanupText,
    InitializeRecordText,
};

class IWorldPathRuntime {
public:
    virtual ~IWorldPathRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t pointer) = 0;
    [[nodiscard]] virtual std::uint32_t ReadStreamWord(std::uint32_t stream) = 0;
    [[nodiscard]] virtual std::uint8_t ReadStreamByte(std::uint32_t stream) = 0;
    virtual void ReadStreamRecord(std::uint32_t stream,
                                  std::uint32_t destination) = 0;
    [[nodiscard]] virtual std::uint32_t OpenPathwayStream(
        std::string_view directory, std::string_view name) = 0;
    virtual void ClosePathwayStream(std::uint32_t stream) = 0;
    [[nodiscard]] virtual std::uint32_t RegisterSingleton(
        std::string_view name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t deleter) = 0;
    [[nodiscard]] virtual std::uint32_t Call(
        WorldPathCall operation,
        std::initializer_list<std::uint32_t> arguments) = 0;
};

class WorldPathRuntimeStub final : public IWorldPathRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t Free(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ReadStreamWord(std::uint32_t) override {
        return 0u;
    }
    [[nodiscard]] std::uint8_t ReadStreamByte(std::uint32_t) override {
        return 0u;
    }
    void ReadStreamRecord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t OpenPathwayStream(
        std::string_view, std::string_view) override { return 0u; }
    void ClosePathwayStream(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t RegisterSingleton(
        std::string_view, std::uint32_t, std::uint32_t,
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t Call(
        WorldPathCall,
        std::initializer_list<std::uint32_t>) override { return 0u; }
};

class PathwayRecord final {
public:
    // IDA 0x522c80/0x522dd0: stream-backed grid and its release.
    [[nodiscard]] static std::uint32_t Load(
        std::uint32_t record, std::uint32_t stream,
        IWorldPathRuntime& runtime);
    [[nodiscard]] static std::uint32_t Release(
        std::uint32_t record, IWorldPathRuntime& runtime);
    // IDA 0x522f30: forward the record's embedded string cleanup.
    [[nodiscard]] static std::uint32_t CleanupText(
        std::uint32_t record, IWorldPathRuntime& runtime);
};

class WorldPathPlanner final {
public:
    static constexpr std::string_view kSingletonName = "worldPathPlanner";
    static constexpr std::uint32_t kSingletonId = 1026u;

    // IDA 0x522df0-0x522ed0: singleton and path array lifecycle.
    [[nodiscard]] static std::uint32_t Register(
        std::uint8_t& guard, std::uint32_t& registered,
        IWorldPathRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(IWorldPathRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t planner, IWorldPathRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupData(
        std::uint32_t planner, IWorldPathRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupPathList(
        std::uint32_t planner, IWorldPathRuntime& runtime);
    // IDA 0x522f40/0x522f70: name lookup and Pathway.pwy loading.
    [[nodiscard]] static std::uint32_t Find(
        std::uint32_t planner, std::uint32_t name,
        IWorldPathRuntime& runtime);
    [[nodiscard]] static bool LoadPathwayFile(
        std::uint32_t planner, std::string_view directory,
        std::string_view name, IWorldPathRuntime& runtime);
};

}  // namespace pal4
