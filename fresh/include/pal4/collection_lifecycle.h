#pragma once

#include <cstddef>
#include <cstdint>

namespace pal4 {

struct CollectionBaseState {
    std::uint32_t vtable{};
    std::uint32_t references{};
};

struct CollectionWorldState {
    static constexpr std::uint32_t kVtable = 0x84348cu;

    std::uint32_t vtable{};
    std::uint8_t byte_c4{};
    std::uint32_t field_c8{};
    std::uint32_t field_cc{};
    std::uint32_t field_d0{};
    std::uint8_t byte_d4{};
    std::uint32_t field_d8{};
    std::uint32_t field_dc{};
    std::uint32_t field_e0{};
};

struct CollectionState {
    static constexpr std::uint32_t kVtable = 0x84346cu;
    static constexpr std::uint32_t kBaseVtable = 0x84347cu;

    std::uint8_t initial_byte{};
    std::uint32_t* first{};
    std::uint32_t* last{};
    std::uint32_t* limit{};
    CollectionBaseState base{};
    CollectionWorldState world{};
};

class ICollectionLifecycleRuntime {
public:
    virtual ~ICollectionLifecycleRuntime() = default;
    virtual void ConstructGameWorld(CollectionState&) = 0;
    [[nodiscard]] virtual std::uint32_t* AllocateWords(std::size_t count) = 0;
    virtual void FreeWords(std::uint32_t* storage) = 0;
    virtual void FreeBase(CollectionBaseState&) = 0;
    [[nodiscard]] virtual int Clear(std::uint32_t receiver,
                                    CollectionState& state,
                                    std::size_t header_offset,
                                    std::size_t element_size) = 0;
    [[nodiscard]] virtual int ClearAndFree(std::uint32_t receiver,
                                           CollectionState& state,
                                           std::size_t header_offset,
                                           std::size_t element_size) = 0;
    virtual void DeleteElement(std::uint32_t address,
                               std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t WorldClump(
        CollectionState&) = 0;
    virtual void RemoveWorldClump(std::uint32_t clump) = 0;
    virtual void CleanupAtomicGeometry(CollectionState&,
                                       std::size_t offset) = 0;
    [[nodiscard]] virtual std::uint32_t GameInstance() = 0;
    [[nodiscard]] virtual int CleanupWorld(CollectionState&,
                                            std::uint32_t owner) = 0;
    [[nodiscard]] virtual std::uint8_t ReadPrefixByte(
        std::uint32_t address) = 0;
    virtual void WritePrefixByte(std::uint32_t address,
                                 std::uint8_t value) = 0;
    virtual void FreeWorldBuffer(std::uint32_t address) = 0;
    virtual void DestroyGameWorld(CollectionState&) = 0;
};

class CollectionLifecycleRuntimeStub : public ICollectionLifecycleRuntime {
public:
    void ConstructGameWorld(CollectionState&) override {}
    [[nodiscard]] std::uint32_t* AllocateWords(std::size_t count) override {
        return new std::uint32_t[count];
    }
    void FreeWords(std::uint32_t* storage) override { delete[] storage; }
    void FreeBase(CollectionBaseState&) override {}
    [[nodiscard]] int Clear(std::uint32_t, CollectionState&,
                            std::size_t, std::size_t) override { return 0; }
    [[nodiscard]] int ClearAndFree(std::uint32_t, CollectionState&,
                                   std::size_t, std::size_t) override {
        return 0;
    }
    void DeleteElement(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t WorldClump(CollectionState&) override {
        return 0u;
    }
    void RemoveWorldClump(std::uint32_t) override {}
    void CleanupAtomicGeometry(CollectionState&, std::size_t) override {}
    [[nodiscard]] std::uint32_t GameInstance() override { return 0u; }
    [[nodiscard]] int CleanupWorld(CollectionState&,
                                   std::uint32_t) override { return 0; }
    [[nodiscard]] std::uint8_t ReadPrefixByte(
        std::uint32_t) override { return 0u; }
    void WritePrefixByte(std::uint32_t, std::uint8_t) override {}
    void FreeWorldBuffer(std::uint32_t) override {}
    void DestroyGameWorld(CollectionState&) override {}
};

class CollectionLifecycle final {
public:
    static constexpr std::size_t kInitialCapacity = 8u;

    // IDA 0x444ac0 and 0x445050 read separate uninitialized stack bytes.
    [[nodiscard]] static CollectionState& Construct(
        CollectionState& state, std::uint8_t initial_byte,
        std::uint8_t nested_byte,
        ICollectionLifecycleRuntime& runtime);
    static void InitializeNested(CollectionState& state,
                                 std::uint8_t nested_byte,
                                 ICollectionLifecycleRuntime& runtime);
    static void ConstructBase(CollectionBaseState& base);
    [[nodiscard]] static CollectionBaseState& DeleteBase(
        CollectionBaseState& base, std::uint32_t flags,
        ICollectionLifecycleRuntime& runtime);
    [[nodiscard]] static const char* IdGeneratorName() noexcept;
    [[nodiscard]] static int Clear(CollectionState& state,
                                   std::uint32_t receiver,
                                   ICollectionLifecycleRuntime& runtime);
    [[nodiscard]] static int ClearAndFree(CollectionState& state,
                                          std::uint32_t receiver,
                                          ICollectionLifecycleRuntime& runtime);
    [[nodiscard]] static int CleanupGameObjects(
        CollectionState& state, ICollectionLifecycleRuntime& runtime);
    static void CleanupElements(CollectionState& state,
                                ICollectionLifecycleRuntime& runtime);
    static void DestroyNested(CollectionState& state,
                              ICollectionLifecycleRuntime& runtime);
    static void Destroy(CollectionState& state,
                        ICollectionLifecycleRuntime& runtime);
};

}  // namespace pal4
