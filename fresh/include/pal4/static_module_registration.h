#pragma once

#include <cstdint>

namespace pal4 {

struct StaticModuleDescriptor {
    std::uint32_t registration_entry;
    std::uint32_t initializer_entry;
    std::uint32_t cleanup_entry;
    std::uint32_t object_address;
    std::uint32_t initializer_target;
    std::uint32_t cleanup_target;
};

enum class StaticModuleSeries : std::uint8_t {
    First,
    Second,
    Third,
    Fourth,
    Fifth,
    Sixth,
    Seventh,
    Eighth,
};

class IStaticModuleRuntime {
public:
    virtual ~IStaticModuleRuntime() = default;
    virtual void Initialize(std::uint32_t object,
                            std::uint32_t target) = 0;
    virtual void Cleanup(std::uint32_t object,
                         std::uint32_t target) = 0;
    [[nodiscard]] virtual int RegisterAtExit(std::uint32_t callback) = 0;
};

class StaticModuleRegistration final {
public:
    static constexpr std::uint32_t kCount = 232u;
    static constexpr std::uint32_t kFirstEntry = 0x510290u;
    static constexpr std::uint32_t kEntryStride = 0x40u;
    static constexpr std::uint32_t kFirstObject = 0x8f2d40u;
    static constexpr std::uint32_t kSecondFirstEntry = 0x514a60u;
    static constexpr std::uint32_t kSecondFirstObject = 0x8f6010u;
    static constexpr std::uint32_t kThirdCount = 232u;
    static constexpr std::uint32_t kThirdFirstEntry = 0x58b520u;
    static constexpr std::uint32_t kThirdFirstObject = 0x8fdd68u;
    static constexpr std::uint32_t kFourthFirstEntry = 0x5b7430u;
    static constexpr std::uint32_t kFourthFirstObject = 0x901370u;
    static constexpr std::uint32_t kFifthFirstEntry = 0x5bee10u;
    static constexpr std::uint32_t kFifthFirstObject = 0x904630u;
    static constexpr std::uint32_t kSixthCount = 231u;
    static constexpr std::uint32_t kSixthFirstEntry = 0x5c4470u;
    static constexpr std::uint32_t kSixthFirstObject = 0x9078b8u;
    static constexpr std::uint32_t kSeventhFirstEntry = 0x5c84a0u;
    static constexpr std::uint32_t kSeventhFirstObject = 0x90abb8u;
    static constexpr std::uint32_t kEighthFirstEntry = 0x5cf5b0u;
    static constexpr std::uint32_t kEighthFirstObject = 0x90de88u;
    static constexpr std::uint32_t kObjectStride = 0x38u;
    static constexpr std::uint32_t kCleanupTarget = 0x45c330u;

    // IDA 0x510290-0x513c80: 232 consecutive registration triples.
    [[nodiscard]] static StaticModuleDescriptor Describe(std::uint32_t index);
    // IDA 0x514a60-0x518450: a second 232-triple sequence.
    // IDA 0x58b520-0x58ef10: a third 232-triple sequence.
    // IDA 0x5b7430-0x5bae20: a fourth 232-triple sequence.
    // IDA 0x5bee10-0x5c2800: a fifth 232-triple sequence.
    // IDA 0x5c4470-0x5c7e20: a sixth 231-triple sequence.
    // IDA 0x5c84a0-0x5cbe90: a seventh 232-triple sequence.
    // IDA 0x5cf5b0-0x5d2fa0: an eighth 232-triple sequence.
    [[nodiscard]] static StaticModuleDescriptor Describe(
        StaticModuleSeries series, std::uint32_t index);
    [[nodiscard]] static int Register(std::uint32_t index,
                                      IStaticModuleRuntime& runtime);
    [[nodiscard]] static int Register(StaticModuleSeries series,
                                      std::uint32_t index,
                                      IStaticModuleRuntime& runtime);
    static void Initialize(std::uint32_t index,
                           IStaticModuleRuntime& runtime);
    static void Initialize(StaticModuleSeries series, std::uint32_t index,
                           IStaticModuleRuntime& runtime);
    static void Cleanup(std::uint32_t index,
                        IStaticModuleRuntime& runtime);
    static void Cleanup(StaticModuleSeries series, std::uint32_t index,
                        IStaticModuleRuntime& runtime);
};

}  // namespace pal4
