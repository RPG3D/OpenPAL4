#pragma once

#include <cstddef>
#include <cstdint>

namespace pal4 {

struct UppercaseStringState final {
    std::uint32_t prefix = 0u;
    std::uint32_t data = 0u;
    std::uint32_t length = 0u;
    std::uint32_t capacity = 0u;
};

static_assert(sizeof(UppercaseStringState) == 16);
static_assert(offsetof(UppercaseStringState, data) == 4);
static_assert(offsetof(UppercaseStringState, length) == 8);
static_assert(offsetof(UppercaseStringState, capacity) == 12);

class IStringUppercaseRuntime {
public:
    virtual ~IStringUppercaseRuntime() = default;
    [[nodiscard]] virtual std::uint32_t EmptySource() = 0;
    [[nodiscard]] virtual std::uint32_t Duplicate(std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t ToUpper(std::uint32_t text) = 0;
    [[nodiscard]] virtual std::uint32_t Length(std::uint32_t text) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void SetErrorFlag(UppercaseStringState& state,
                              std::uint8_t value) = 0;
    virtual void Reallocate(UppercaseStringState& state,
                            std::uint32_t length) = 0;
    virtual void Copy(std::uint32_t destination, std::uint32_t source,
                      std::uint32_t length) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
};

class StringUppercaseRuntimeStub final : public IStringUppercaseRuntime {
public:
    [[nodiscard]] std::uint32_t EmptySource() override { return 0u; }
    [[nodiscard]] std::uint32_t Duplicate(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ToUpper(std::uint32_t text) override {
        return text;
    }
    [[nodiscard]] std::uint32_t Length(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t) override { return 0u; }
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    void SetErrorFlag(UppercaseStringState&, std::uint8_t) override {}
    void Reallocate(UppercaseStringState&, std::uint32_t) override {}
    void Copy(std::uint32_t, std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t Free(std::uint32_t) override { return 0u; }
};

class StringUppercase final {
public:
    // IDA 0x5fc330.
    [[nodiscard]] static std::uint32_t Convert(
        UppercaseStringState& state, IStringUppercaseRuntime& runtime);
};

}  // namespace pal4
