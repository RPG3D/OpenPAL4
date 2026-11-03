#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct CylinderObjectState final {
    std::array<std::uint32_t, 39u> words{};
};

static_assert(sizeof(CylinderObjectState) == 156u);

class ICylinderObjectRuntime {
public:
    virtual ~ICylinderObjectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SystemInstance() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void FreeBuffer(std::uint32_t address) = 0;
    virtual void FreeObject(CylinderObjectState& state) = 0;
    [[nodiscard]] virtual std::uint32_t ForwardAction(
        CylinderObjectState& state, const std::array<std::uint32_t, 3>& args,
        std::uint32_t mode) = 0;
};

class CylinderObjectRuntimeStub final : public ICylinderObjectRuntime {
public:
    [[nodiscard]] std::uint32_t SystemInstance() override { return 0u; }
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    void FreeBuffer(std::uint32_t) override {}
    void FreeObject(CylinderObjectState&) override {}
    [[nodiscard]] std::uint32_t ForwardAction(
        CylinderObjectState&, const std::array<std::uint32_t, 3>&,
        std::uint32_t) override { return 0u; }
};

class ICylinderObjectAccessRuntime {
public:
    virtual ~ICylinderObjectAccessRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
};

class ICylinderObjectPlaybackRuntime {
public:
    virtual ~ICylinderObjectPlaybackRuntime() = default;
    virtual void QueryTaskValues(CylinderObjectState& state,
                                 std::uint32_t facet_offset,
                                 std::array<std::int32_t, 4>& values) = 0;
    [[nodiscard]] virtual double Duration(CylinderObjectState& state) = 0;
};

class CylinderObject final {
public:
    // IDA 0x60bfb0. The stack-derived byte at +140 is explicit.
    static CylinderObjectState& Construct(
        CylinderObjectState& state, std::uint8_t initial_byte,
        ICylinderObjectRuntime& runtime);
    // IDA 0x60c0c0, facet at object +8.
    [[nodiscard]] static std::uint32_t ForwardAction(
        CylinderObjectState& state, const std::array<std::uint32_t, 3>& args,
        ICylinderObjectRuntime& runtime);
    // IDA 0x60c100.
    static void Destroy(CylinderObjectState& state,
                        ICylinderObjectRuntime& runtime);
    // IDA code island 0x60c0e0, deleting destructor thunk.
    [[nodiscard]] static std::uintptr_t DeletingDestroy(
        CylinderObjectState& state, std::uint32_t flags,
        ICylinderObjectRuntime& runtime);
    // IDA 0x60cb00/0x60cb20.
    [[nodiscard]] static std::uint32_t ResourceHandle(
        const CylinderObjectState& state,
        ICylinderObjectAccessRuntime& runtime);
    [[nodiscard]] static std::uint32_t Resource(
        const CylinderObjectState& state,
        ICylinderObjectAccessRuntime& runtime);
    // IDA 0x60cb40/0x60cb50.
    [[nodiscard]] static std::string_view ClassName();
    static void SetTimedMode(CylinderObjectState& state,
                             std::uint32_t enabled,
                             ICylinderObjectPlaybackRuntime& runtime);
};

}  // namespace pal4
