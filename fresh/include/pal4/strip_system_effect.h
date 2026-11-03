#pragma once

#include "pal4/strip_resource_block.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IStripSystemFactoryRuntime : public IStripResourceBlockRuntime {
public:
    virtual ~IStripSystemFactoryRuntime() = default;
    [[nodiscard]] virtual bool Initialize(
        std::uint32_t effect, std::uint32_t config,
        std::uint32_t clump, std::uint32_t context) = 0;
    virtual void Destroy(std::uint32_t effect, bool free_memory) = 0;
};

class IStripSystemTimeRuntime {
public:
    virtual ~IStripSystemTimeRuntime() = default;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
};

class IStripSystemConfigRuntime {
public:
    virtual ~IStripSystemConfigRuntime() = default;
    virtual void Reset(std::uint32_t effect) = 0;
    [[nodiscard]] virtual std::int32_t ReadInt(
        std::uint32_t config, std::string_view key,
        std::int32_t fallback) = 0;
    [[nodiscard]] virtual float ReadFloat(
        std::uint32_t config, std::string_view key,
        float fallback) = 0;
    [[nodiscard]] virtual std::string ReadText(
        std::uint32_t config, std::string_view key) = 0;
    [[nodiscard]] virtual std::uint32_t FindTexture(
        std::string_view path) = 0;
    [[nodiscard]] virtual std::uint32_t LoadTexture(
        std::string_view path) = 0;
    virtual void DestroyTexture(std::uint32_t texture) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void SetDivision(std::uint32_t block,
                             std::uint32_t value) = 0;
    [[nodiscard]] virtual bool SetupBlock(
        std::uint32_t block, std::uint32_t texture,
        std::uint32_t clump, float lifetime,
        std::uint8_t mode) = 0;
};

class IStripSystemLifecycleRuntime {
public:
    virtual ~IStripSystemLifecycleRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t CleanupResources(
        std::uint32_t block) = 0;
    [[nodiscard]] virtual std::uint8_t UpdateVisual(
        std::uint32_t block, double seconds,
        std::uint8_t time_flag) = 0;
    virtual void ManageVisual(std::uint32_t block) = 0;
    virtual void DestructResourceBlocks(
        std::uint32_t first, std::uint32_t size,
        std::uint32_t count) = 0;
};

class StripSystemEffect final {
public:
    static constexpr std::uint32_t kObjectSize = 0x118u;
    static constexpr std::uint32_t kBaseVtable = 0x84b208u;
    static constexpr std::uint32_t kVtable = 0x84b21cu;

    // IDA 0x63c700: construct two strip resource blocks and initialize.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t config, std::uint32_t clump,
        std::uint32_t context, IStripSystemFactoryRuntime& runtime);
    // IDA 0x63c7d0: read lifetime and mark the time flag.
    [[nodiscard]] static double GetTime(
        std::uint32_t effect, IStripSystemTimeRuntime& runtime);
    // IDA 0x63c7e0: initialize two strip blocks from XML and a texture.
    [[nodiscard]] static bool InitializeFromConfig(
        std::uint32_t effect, std::uint32_t config,
        std::uint32_t clump, std::uint32_t context,
        IStripSystemConfigRuntime& runtime);
    // IDA 0x63cb10: release resources in both strips when active.
    [[nodiscard]] static std::uint8_t CleanupObject(
        std::uint32_t effect, IStripSystemLifecycleRuntime& runtime);
    // IDA 0x63cb40/0x63cb80: update both strips' visual state.
    [[nodiscard]] static std::uint8_t UpdateVisuals(
        std::uint32_t effect, double seconds,
        IStripSystemLifecycleRuntime& runtime);
    static void ManageVisuals(
        std::uint32_t effect, IStripSystemLifecycleRuntime& runtime);
    // IDA 0x63cbd0: clean resources and destroy both blocks.
    static void Destruct(
        std::uint32_t effect, IStripSystemLifecycleRuntime& runtime);
};

}  // namespace pal4
