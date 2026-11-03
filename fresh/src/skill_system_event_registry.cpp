#include "pal4/skill_system_event_registry.h"

#include "pal4/skill_event_sorter.h"
#include "pal4/script_skill_vector8.h"

#include <new>
#include <vector>

namespace pal4 {
namespace {

constexpr std::array<SkillEventRegistration, 39> kRegistrations{{
    {9000u, 0x57fd80u}, {9001u, 0x57fdf0u},
    {9100u, 0x57fea0u}, {9101u, 0x7e1840u},
    {9102u, 0x7e1840u}, {9103u, 0x7e1840u},
    {9104u, 0x7e1840u}, {9105u, 0x5800b0u},
    {9106u, 0x5800f0u}, {9107u, 0x5801f0u},
    {9108u, 0x5802d0u}, {9109u, 0x580390u},
    {9110u, 0x5803b0u}, {9111u, 0x5803d0u},
    {9112u, 0x580510u}, {9113u, 0x580530u},
    {9114u, 0x580550u}, {9115u, 0x580590u},
    {9116u, 0x5806d0u}, {9117u, 0x5808e0u},
    {9118u, 0x580a70u}, {9119u, 0x580ba0u},
    {9120u, 0x7e1840u}, {9121u, 0x7e1840u},
    {9122u, 0x580ca0u}, {9123u, 0x580d40u},
    {9124u, 0x580d70u}, {9125u, 0x580dc0u},
    {9126u, 0x580f20u}, {9127u, 0x5810b0u},
    {9128u, 0x581140u}, {9129u, 0x581180u},
    {9130u, 0x5811c0u}, {9131u, 0x581210u},
    {9132u, 0x581250u}, {9133u, 0x581280u},
    {9134u, 0x581380u}, {9135u, 0x5813f0u},
    {9136u, 0x581430u},
}};

static_assert(kRegistrations.size() == SkillSystemEventRegistry::kEventCount);

class VectorRuntime final : public IScriptSkillVector8Runtime {
public:
    explicit VectorRuntime(ISkillSystemLifecycleRuntime& runtime)
        : runtime_(runtime) {}
    std::uint32_t ReadWord(std::uint32_t address) override {
        return runtime_.ReadWord(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        runtime_.WriteWord(address, value);
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        return runtime_.Allocate(size);
    }
    void Free(std::uint32_t address) override {
        static_cast<void>(runtime_.Free(address));
    }
private:
    ISkillSystemLifecycleRuntime& runtime_;
};

}  // namespace

const std::array<SkillEventRegistration,
                 SkillSystemEventRegistry::kEventCount>&
SkillSystemEventRegistry::Entries() {
    return kRegistrations;
}

void SkillSystemEventRegistry::Initialize(
    std::uint32_t object, ISkillSystemLifecycleRuntime& runtime) {
    runtime.SetLogFile(kLogFileAddress, 0u);
    runtime.WriteWord(object, 2u);
    runtime.InitializeEventObjects();
    runtime.SeedRandomNumberGenerator(runtime.TimeGetTime());

    const auto header = object + kVectorOffset;
    runtime.WriteWord(header + 8u, runtime.ReadWord(header + 4u));
    VectorRuntime vector_runtime(runtime);
    for (const auto& entry : kRegistrations) {
        const auto end = runtime.ReadWord(header + 8u);
        if (ScriptSkillVector8::Insert(header, end, 1u,
                                       {entry.id, entry.callback},
                                       vector_runtime) == 0u)
            throw std::bad_alloc();
    }

    std::vector<SkillEventRegistration> entries;
    entries.reserve(kRegistrations.size());
    const auto begin = runtime.ReadWord(header + 4u);
    for (std::uint32_t i = 0u; i < kEventCount; ++i) {
        const auto address = begin + i * 8u;
        entries.push_back({runtime.ReadWord(address),
                           runtime.ReadWord(address + 4u)});
    }
    SkillEventSorter::Sort(entries);
    for (std::uint32_t i = 0u; i < kEventCount; ++i) {
        const auto address = begin + i * 8u;
        if (runtime.ReadWord(address) != entries[i].id)
            runtime.WriteWord(address, entries[i].id);
        if (runtime.ReadWord(address + 4u) != entries[i].callback)
            runtime.WriteWord(address + 4u, entries[i].callback);
    }
}

}  // namespace pal4
