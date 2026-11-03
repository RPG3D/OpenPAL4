#include "pal4/native_behavior_helper_storage.h"
#include "pal4/native_behavior_helper_physics.h"
#include "pal4/native_behavior_helper_configuration.h"
#include "pal4/native_behavior_grid_association_runtime.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_npc_manager_owner.h"
#include "pal4/grid_node_object.h"
#include "pal4/spatial_grid_wrapper.h"
#include "pal4/native_legacy_crt_math.h"
#include "pal4/grid_manager.h"
#include <float.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <array>
#include <bit>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace {
void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
template<class T> T& At(std::uint32_t address) {
    return *reinterpret_cast<T*>(static_cast<std::uintptr_t>(address));
}
void VerifyCapturedConstraintPower() {
    struct Precision {
        unsigned saved=_controlfp(0,0);
        Precision(){(void)_controlfp(_PC_24,_MCW_PC);}
        ~Precision(){(void)_controlfp(saved,_MCW_PC|_MCW_RC);}
    } precision;
    class Runtime final : public pal4::IGridManagerDirectionRuntime {
    public:
        bool legacy=true;
        float ReadParameter(const pal4::GridManagerState&,std::uint32_t slot) override {
            Check(slot==120u || slot==100u,"captured direction scalar slot");
            return slot==120u?50.f:std::bit_cast<float>(1092307132u);
        }
        pal4::GridVector3 ReadNormal(const pal4::GridManagerState&) override {
            return {std::bit_cast<float>(1060225766u),0.f,std::bit_cast<float>(1060649025u)};
        }
        pal4::GridVector3 ReadZeroVector() override {return {};}
        double Power(double base,double exponent) override {
            Check(base==static_cast<double>(std::bit_cast<float>(0x3f787460u)) && exponent==20.,
                "captured power arguments retain original float division store");
            return legacy?pal4::NativeLegacyCrtMath::Pow(base,exponent):std::pow(base,exponent);
        }
    } runtime;
    const pal4::GridVector3 requested{std::bit_cast<float>(1087378139u),0.f,std::bit_cast<float>(3240483951u)};
    pal4::GridManagerState state{};pal4::GridVector3 result{};
    (void)pal4::GridManager::ConstrainVelocity(state,result,requested,runtime);
    const auto original=std::bit_cast<std::array<std::uint32_t,3>>(result);
    Check(original==std::array<std::uint32_t,3>{1090345308u,0u,3239397866u},
        "public legacy power reproduces original helper8 constrained vector");
    runtime.legacy=false;
    (void)pal4::GridManager::ConstrainVelocity(state,result,requested,runtime);
    Check(std::bit_cast<std::array<std::uint32_t,3>>(result)==
        std::array<std::uint32_t,3>{1090345305u,0u,3239397867u},
        "modern power reproduces former native helper8 fork");
}
// A helper ends exactly at a PAGE_NOACCESS boundary. Oversized host-state
// reads as well as writes fault, including the short original BC/C0 images.
class GuardedImage final {
public:
    explicit GuardedImage(std::uint32_t bytes) : size(bytes) {
        SYSTEM_INFO info{};
        GetSystemInfo(&info);
        region = VirtualAlloc(nullptr, info.dwPageSize * 2u, MEM_RESERVE, PAGE_NOACCESS);
        if (!region) throw std::bad_alloc();
        if (!VirtualAlloc(region, info.dwPageSize, MEM_COMMIT, PAGE_READWRITE)) {
            VirtualFree(region, 0u, MEM_RELEASE);
            throw std::bad_alloc();
        }
        address = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(region)) +
            info.dwPageSize - bytes;
        std::memset(reinterpret_cast<void*>(address - 8u), 0xa5, bytes + 8u);
    }
    ~GuardedImage() { VirtualFree(region, 0u, MEM_RELEASE); }
    GuardedImage(const GuardedImage&) = delete;
    GuardedImage& operator=(const GuardedImage&) = delete;
    void CheckPrefix() const {
        for (unsigned i = 0; i < 8; ++i)
            Check(At<std::uint8_t>(address - 8u + i) == 0xa5u, "helper prefix untouched");
    }
    void* region{};
    std::uint32_t size{}, address{};
};
}

int main() {
    try {
        VerifyCapturedConstraintPower();
        pal4::NativeBaseObjectMemory memory;
        pal4::NativeNpcManagerOwner manager;
        (void)manager.Construct();
        Check(manager.InitializeSceneGrid() != 0u, "actual NPC grid initialized");
        auto& wrapper = At<pal4::SpatialGridWrapperState>(manager.GridWrapperAddress());
        pal4::NativeBehaviorGridAssociationRuntime associations(memory);
        std::uint32_t next_id = 90u, draws = 0u;
        bool bad_random = false;
        bool reject_cleanup = false;
        const auto random = [&] {
            if (bad_random) return 32768u;
            const std::array<std::uint32_t,3> samples{16384u, 24575u, 16384u};
            return samples[draws++ % 3u];
        };
        pal4::NativeBehaviorHelperStorage storage(memory, next_id,
            [&](std::uint32_t association, bool free_self) {
                if (reject_cleanup) throw std::logic_error("nested cleanup unavailable");
                associations.DestroyAssociation(association, free_self);
            }, random);
        pal4::NativeBehaviorHelperConfiguration configuration(memory, associations, random);
        const std::array<std::uint32_t,5> vtables{
            0x842898u,0x842898u,0x842a30u,0x842ae8u,0x842978u};
        for (std::int32_t kind = 0; kind != 5; ++kind) {
            draws = 0u;
            GuardedImage allocation(pal4::NativeBehaviorHelperStorage::ImageSize(kind));
            const auto address = allocation.address;
            auto& image = At<pal4::BehaviorPathEndpointState>(address);
            const auto before_draws = draws;
            storage.Construct(kind, address);
            Check(image.words[0] == vtables[kind] && image.words[22] == 90u + kind,
                  "all five shared lifecycle variants preserve ID and actual vtable");
            Check(draws == before_draws + (kind == 3 ? 3u : 0u),
                  "only random helper constructor consumes shared rejection sampler");
            Check(At<std::uint8_t>(address + 0xbau) == 0xa5u &&
                      At<std::uint8_t>(address + 0xbbu) == 0xa5u,
                  "base tail padding is preserved even in short BC helper");
            const auto random_direction = kind == 3
                ? std::array<std::uint32_t,3>{image.words[50],image.words[51],image.words[52]}
                : std::array<std::uint32_t,3>{};
            if (kind == 3) {
                Check(image.words[48] == 0x40000000u && image.words[49] == 0u &&
                          std::fabs(std::bit_cast<float>(image.words[50]) - 1.0f) < 0.001f &&
                          image.words[51] == 0u,
                      "class3 preserves original normalized XZ random heading and default range");
            }
            const auto before_config = draws;
            const auto path = kind == 4 ? 0x12345678u : 0u;
            (void)configuration.Configure(address, manager.GridWrapperAddress(), path);
            Check(draws == before_config + (kind < 2 ? 4u : 0u),
                  "base reset does not consume endpoint activation random draws");
            Check(pal4::SpatialGridWrapper::CountNodes(wrapper, associations) == 1u,
                  "configured helper is in actual spatial grid");
            const auto& node = At<pal4::GridNodeObjectState>(image.words[42]);
            Check(node.node.payload == address && node.grid == wrapper.grid,
                  "association borrows actual helper address rather than padded host state");
            Check(image.words[25] == 0u && image.words[24] == 0x3f000000u,
                  "original movement reset defaults applied");
            if (kind == 4) Check(image.words[47] == path, "class4 retains original property argument");
            if (kind == 3) Check(random_direction == std::array<std::uint32_t,3>{
                      image.words[50],image.words[51],image.words[52]} && image.words[48] == 0x40000000u,
                  "base configuration preserves random helper derived state");
            using Vectors = pal4::NativeBehaviorHelperConfiguration;
            Vectors::SetVector(image, 0x1cu, {3.0f,4.0f,5.0f});
            Check(Vectors::ReadVector(image, 0x18u) == pal4::BehaviorVector3{3.0f,4.0f,5.0f},
                  "short helper vector reads and writes stay within shared base");
            pal4::NativeBehaviorHelperStorage::SetScalar(image, 0x68u, 7.5f);
            Check(std::bit_cast<float>(image.words[25]) == 7.5f, "short helper scalar setter is bounded");
            (void)configuration.Configure(address, manager.GridWrapperAddress(), path);
            Check(pal4::SpatialGridWrapper::CountNodes(wrapper, associations) == 1u,
                  "reconfiguration replaces association without duplicate spatial nodes");
            if (kind<2 || kind==3) {
                const std::vector<std::uint8_t> tail(
                    reinterpret_cast<const std::uint8_t*>(address+0xbau),
                    reinterpret_cast<const std::uint8_t*>(address+allocation.size));
                Vectors::SetVector(image,0x14u,{0.f,0.f,1.f});
                Vectors::SetVector(image,0x1cu,{0.f,0.f,0.f});
                pal4::NativeBehaviorHelperStorage::SetScalar(image,0x54u,2.f);
                pal4::NativeBehaviorHelperStorage::SetScalar(image,0x68u,0.f);
                pal4::NativeBehaviorHelperStorage::SetScalar(image,0x74u,4.f);
                pal4::NativeBehaviorHelperStorage::SetScalar(image,0x7cu,1.f);
                pal4::NativeBehaviorHelperPhysics::Integrate(address,{0.f,0.f,100.f},0.f);
                Check(Vectors::ReadVector(image,0x18u)==pal4::BehaviorVector3{},
                      "native zero-duration integration does not move helper");
                float previous=0.f;
                for(unsigned frame=0;frame<120;++frame) {
                    pal4::NativeBehaviorHelperPhysics::Integrate(address,{0.f,0.f,100.f},1.f/60.f);
                    const auto point=Vectors::ReadVector(image,0x18u);
                    const auto speed=std::bit_cast<float>(image.words[25]);
                    Check(point[2]>=previous && point[0]==0.f && point[1]==0.f && speed<=1.00001f,
                          "native force/mass integration advances monotonically within speed cap");
                    previous=point[2];
                }
                Check(previous>1.f && previous<=2.f,
                      "two seconds of real shared physics produces bounded continuous movement");
                Check(std::vector<std::uint8_t>(reinterpret_cast<const std::uint8_t*>(address+0xbau),
                          reinterpret_cast<const std::uint8_t*>(address+allocation.size))==tail,
                      "native physics preserves all endpoint/random derived fields and tail padding");
                bool rejected_frame=false;
                try { pal4::NativeBehaviorHelperPhysics::Integrate(address,{},-1.f); }
                catch(const std::invalid_argument&) { rejected_frame=true; }
                Check(rejected_frame && Vectors::ReadVector(image,0x18u)[2]==previous,
                      "invalid native frame rejects before altering movement state");
            }
            image.words[43] = memory.Allocate(20u);
            reject_cleanup = true;
            bool cleanup_failed = false;
            try { storage.Destruct(kind, address); }
            catch (const std::logic_error&) { cleanup_failed = true; }
            Check(cleanup_failed && image.words[0] == 0x84b600u &&
                      image.words[42] != 0u && image.words[43] != 0u,
                  "failed cleanup persists base progress and retains both owned resources");
            reject_cleanup = false;
            storage.Destruct(kind, address);
            storage.Destruct(kind, address);
            allocation.CheckPrefix();
            Check(memory.OutstandingAllocations() == 0u &&
                      pal4::SpatialGridWrapper::CountNodes(wrapper, associations) == 0u,
                  "all lifecycle variants discard associations and buffers exactly once");
        }
        GuardedImage failed(0xe0u);
        bad_random = true;
        bool rejected = false;
        try { storage.Construct(3, failed.address); }
        catch (const std::out_of_range&) { rejected = true; }
        Check(rejected && At<std::uint32_t>(failed.address) == 0x84b6e8u &&
                  memory.OutstandingAllocations() == 0u,
              "random construction failure persists shared base cleanup for safe discard");
        storage.Destruct(3, failed.address);
        failed.CheckPrefix();
        manager.Destroy();
        Check(manager.OutstandingAllocations() == 0u && memory.OutstandingAllocations() == 0u,
              "actual NPC grid and helper owners fully retire");
        std::puts("PASS: five native helper lifecycles and bounded actual grid configuration");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL: %s\n", error.what());
        return 1;
    }
}
