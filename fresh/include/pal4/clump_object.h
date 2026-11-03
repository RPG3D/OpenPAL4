#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

class IClumpObjectConfigRuntime;
class IClumpObjectControlRuntime;
class IClumpObjectUpdateRuntime;
class IClumpObjectRenderRuntime;
class IClumpObjectTransformRuntime;
class IClumpObjectFrameRuntime;
class IClumpObjectColorRuntime;

struct ClumpObjectState final {
    std::array<std::uint32_t, 81u> words{};
};

static_assert(sizeof(ClumpObjectState) == 0x144);

class IClumpObjectAccessRuntime {
public:
    virtual ~IClumpObjectAccessRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
};

class IClumpObjectRuntime : public IClumpObjectAccessRuntime {
public:
    virtual ~IClumpObjectRuntime() = default;
    virtual void InitializeXml(ClumpObjectState& state,
                               std::uint32_t offset) = 0;
    virtual void DestroyXml(ClumpObjectState& state,
                            std::uint32_t offset) = 0;
    virtual void ReleaseText(ClumpObjectState& state,
                             std::uint32_t offset) = 0;
    virtual void ReleaseBuffer(std::uint32_t address) = 0;
    virtual void SetErrorFlag(ClumpObjectState& state,
                              std::uint32_t offset,
                              std::uint8_t flag) = 0;
    [[nodiscard]] virtual std::uint32_t SystemInstance() = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class IClumpObjectPlaybackRuntime {
public:
    virtual ~IClumpObjectPlaybackRuntime() = default;
    virtual void QueryTaskValues(ClumpObjectState& state,
                                 std::uint32_t facet_offset,
                                 std::array<std::int32_t, 4>& values) = 0;
    [[nodiscard]] virtual double Duration(ClumpObjectState& state) = 0;
};

class ClumpObjectRuntimeStub final : public IClumpObjectRuntime {
public:
    void InitializeXml(ClumpObjectState&, std::uint32_t) override {}
    void DestroyXml(ClumpObjectState&, std::uint32_t) override {}
    void ReleaseText(ClumpObjectState&, std::uint32_t) override {}
    void ReleaseBuffer(std::uint32_t) override {}
    void SetErrorFlag(ClumpObjectState&, std::uint32_t,
                      std::uint8_t) override {}
    [[nodiscard]] std::uint32_t SystemInstance() override { return 0u; }
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
};

class ClumpObject final {
public:
    // IDA 0x608ef0. The stack byte copied into four fields is explicit.
    static ClumpObjectState& Construct(
        ClumpObjectState& state, std::uint8_t initial_byte,
        IClumpObjectRuntime& runtime);
    // IDA 0x6090c0.
    static void Destroy(ClumpObjectState& state,
                        IClumpObjectRuntime& runtime);
    // IDA 0x609b30/0x609b50/0x609b70.
    [[nodiscard]] static bool IsActive(
        const ClumpObjectState& state,
        IClumpObjectAccessRuntime& runtime);
    [[nodiscard]] static std::uint32_t ObjectHandle(
        const ClumpObjectState& state,
        IClumpObjectAccessRuntime& runtime);
    [[nodiscard]] static std::uint32_t Object(
        const ClumpObjectState& state,
        IClumpObjectAccessRuntime& runtime);
    // IDA 0x60ae00/0x60ae30/0x60ae60. Return the low byte of the
    // final copied word, as the native char return does.
    [[nodiscard]] static std::uint8_t GetPosition(
        const ClumpObjectState& state,
        std::array<std::uint32_t, 3>& output,
        IClumpObjectAccessRuntime& runtime);
    [[nodiscard]] static std::uint8_t GetRotation(
        const ClumpObjectState& state,
        std::array<std::uint32_t, 3>& output,
        IClumpObjectAccessRuntime& runtime);
    [[nodiscard]] static std::uint8_t GetScale(
        const ClumpObjectState& state,
        std::array<std::uint32_t, 3>& output,
        IClumpObjectAccessRuntime& runtime);
    // IDA 0x60ae90/0x60af30/0x60af80.
    [[nodiscard]] static std::uint32_t SetPosition(
        const ClumpObjectState& state, const std::array<float, 3>& value,
        bool relative, IClumpObjectTransformRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetRotation(
        const ClumpObjectState& state, const std::array<float, 3>& value,
        bool relative, IClumpObjectTransformRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetScale(
        const ClumpObjectState& state, const std::array<float, 3>& value,
        bool relative, IClumpObjectTransformRuntime& runtime);
    // IDA 0x60b050.
    [[nodiscard]] static std::uint32_t ApplyFrameTransform(
        const ClumpObjectState& state,
        const std::array<float, 3>& target,
        const std::array<float, 3>& rotation,
        const std::array<float, 3>& step,
        IClumpObjectFrameRuntime& runtime);
    // IDA 0x60b250/0x60b280, native color facet at object +36.
    [[nodiscard]] static std::uint8_t GetColor(
        const ClumpObjectState& state,
        std::array<std::uint8_t, 4>& output);
    [[nodiscard]] static std::uintptr_t SetColor(
        ClumpObjectState& state,
        const std::array<std::uint8_t, 4>& color,
        IClumpObjectColorRuntime& runtime);
    // IDA 0x609b90.
    [[nodiscard]] static std::string_view ClassName();
    // IDA 0x609ba0.
    static void SetTimedMode(ClumpObjectState& state, std::uint32_t enabled,
                             IClumpObjectPlaybackRuntime& runtime);
    // IDA 0x609c10.
    [[nodiscard]] static std::uint8_t LoadConfiguration(
        ClumpObjectState& state, std::uint32_t document,
        IClumpObjectConfigRuntime& runtime);
    // IDA 0x60a7c0/0x60a860.
    [[nodiscard]] static std::uint8_t Reset(
        ClumpObjectState& state, IClumpObjectConfigRuntime& runtime);
    [[nodiscard]] static std::uint32_t StopAndDestroy(
        ClumpObjectState& state, IClumpObjectControlRuntime& runtime);
    // IDA 0x60aa60: EDI is saved locally, not an extra argument.
    static void Update(ClumpObjectState& state, double delta_time,
                       IClumpObjectUpdateRuntime& runtime);
    // IDA 0x60abd0. The native stack argument is unused.
    [[nodiscard]] static std::uint8_t RenderClump(
        const ClumpObjectState& state, std::uint32_t unused_argument,
        IClumpObjectRenderRuntime& runtime);
};

}  // namespace pal4
