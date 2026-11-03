#pragma once

#include <array>
#include <cstdint>
#include <cstring>

namespace pal4 {

struct PictureViewWindowState {
    std::array<std::uint8_t, 0x64> bytes{};

    [[nodiscard]] std::uint32_t Read(std::uint32_t offset) const noexcept {
        std::uint32_t value{};
        std::memcpy(&value, bytes.data() + offset, sizeof(value));
        return value;
    }
    void Write(std::uint32_t offset, std::uint32_t value) noexcept {
        std::memcpy(bytes.data() + offset, &value, sizeof(value));
    }
};

struct PictureRecordRange {
    std::uint32_t begin{};
    std::uint32_t end{};
};

class IPictureViewWindowLifecycleRuntime {
public:
    virtual ~IPictureViewWindowLifecycleRuntime() = default;
    virtual void LoadLayout(PictureViewWindowState& state,
                            std::uint32_t name, std::uint32_t first_flag,
                            std::uint32_t second_flag) = 0;
    virtual void InitializeElements(PictureViewWindowState& state) = 0;
    [[nodiscard]] virtual PictureRecordRange PictureRecords() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateMemory(
        std::uint32_t bytes) = 0;
    virtual void FreeMemory(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyDialog(
        PictureViewWindowState& state) = 0;
};

class PictureViewWindowLifecycleRuntimeStub :
    public IPictureViewWindowLifecycleRuntime {
public:
    void LoadLayout(PictureViewWindowState&, std::uint32_t,
                    std::uint32_t, std::uint32_t) override {}
    void InitializeElements(PictureViewWindowState&) override {}
    [[nodiscard]] PictureRecordRange PictureRecords() override { return {}; }
    [[nodiscard]] std::uint32_t ReadWord(
        std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t AllocateMemory(
        std::uint32_t) override { return 0u; }
    void FreeMemory(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t DestroyDialog(
        PictureViewWindowState&) override { return 0u; }
};

class PictureViewWindowRecords final {
public:
    [[nodiscard]] static bool Eligible(std::uint32_t record_id) noexcept;
    static void Append(PictureViewWindowState& state,
                       std::uint32_t record_address,
                       IPictureViewWindowLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t CopyWords(
        std::uint32_t begin, std::uint32_t end, std::uint32_t destination,
        IPictureViewWindowLifecycleRuntime& runtime);
};

class PictureViewWindowLifecycle final {
public:
    static constexpr std::uint32_t kObjectVtable = 0x8436f0u;
    static constexpr std::uint32_t kMemberVtable = 0x8436c8u;
    static constexpr std::uint32_t kInitialMemberVtable = 0x8440d8u;

    [[nodiscard]] static PictureViewWindowState& Construct(
        PictureViewWindowState& state, std::uint32_t object_address,
        std::uint32_t name,
        IPictureViewWindowLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t Cleanup(
        PictureViewWindowState& state,
        IPictureViewWindowLifecycleRuntime& runtime);
};

}  // namespace pal4
