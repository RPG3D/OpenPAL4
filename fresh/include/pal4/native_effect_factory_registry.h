#pragma once
#include <cstdint>
#include <memory>
#include <string_view>

namespace pal4 {
enum class EfEffectSystemStateTree : std::uint8_t;
class NativeBaseObjectMemory;
class INativeEffectFactoryLookup {
public:
    virtual ~INativeEffectFactoryLookup() = default;
    virtual std::uint32_t FindObjectFactory(std::string_view name) = 0;
    virtual std::uint32_t FindTaskFactory(std::string_view name) = 0;
};

// Owns the singleton's primary (+8) and secondary (+24) creator trees.
// Borrows zeroed storage and memory, which must outlive this owner. Creator
// values are dispatch tokens for rebuilt implementations, never callable EXE VAs.
class NativeEffectFactoryRegistry final : public INativeEffectFactoryLookup {
public:
    NativeEffectFactoryRegistry(NativeBaseObjectMemory&,std::uint32_t singleton);
    ~NativeEffectFactoryRegistry();
    NativeEffectFactoryRegistry(const NativeEffectFactoryRegistry&) = delete;
    NativeEffectFactoryRegistry& operator=(const NativeEffectFactoryRegistry&) = delete;
    void RegisterAll();
    void RegisterObjects();
    void RegisterTasks();
    // Shared CleanupState deletes payload nodes, then resets head/count itself.
    // These operations retain this owner's dynamic nil and tree heads.
    void DeleteStateSubtree(EfEffectSystemStateTree, std::uint32_t root);
    std::uint32_t StateNil(EfEffectSystemStateTree) const;
    void Clear();
    std::uint32_t FindObjectFactory(std::string_view name) override;
    std::uint32_t FindTaskFactory(std::string_view name) override;
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}  // namespace pal4
