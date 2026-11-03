#include "renderware_raster_resource_cases.h"
#include "pal4/rw_raster_copy_native.h"
#include <rwcore.h>
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <vector>

namespace pal4::test {
namespace {
void Require(bool ok, const char* text) { if (!ok) throw std::runtime_error(text); }
int created{}, destroyed{}, plugin_offset{};
void* Construct(void* object, RwInt32 offset, RwInt32 size) {
    Require(size == 4 && offset == plugin_offset, "raster plugin layout");
    *reinterpret_cast<int*>(static_cast<char*>(object) + offset) = 0x4567; ++created; return object;
}
void* Destruct(void* object, RwInt32 offset, RwInt32) {
    Require(*reinterpret_cast<int*>(static_cast<char*>(object) + offset) == 0x4567, "raster plugin state");
    ++destroyed; return object;
}
struct DropRaster { void operator()(RwRaster* p) const { if (p) RwRasterDestroy(p); } };
using Raster = std::unique_ptr<RwRaster, DropRaster>;
Raster Make(bool mips = false) {
    Raster r(RwRasterCreate(16, 8, 32, rwRASTERTYPETEXTURE | rwRASTERFORMAT8888 |
        (mips ? rwRASTERFORMATMIPMAP : 0)));
    Require(r != nullptr, "raster creation"); return r;
}
void Context() {
    auto* baseline = RwRasterGetCurrentContext();
    const auto c = created, d = destroyed;
    {
        auto a = Make(), b = Make();
        Require(created == c + 2, "raster constructors");
        Require(RwRasterPushContext(a.get()) && RwRasterGetCurrentContext() == a.get(), "first raster context");
        Require(RwRasterPushContext(b.get()) && RwRasterGetCurrentContext() == b.get(), "nested raster context");
        Require(RwRasterPopContext() == b.get() && RwRasterGetCurrentContext() == a.get(), "nested context pop");
        Require(RwRasterPopContext() == a.get() && RwRasterGetCurrentContext() == baseline, "outer context pop");
    }
    Require(destroyed == d + 2, "raster destructors");
}
void Locks() {
    auto raster = Make(true);
    Require(RwRasterGetNumLevels(raster.get()) == 5, "explicit mip count");
    for (int level = 0; level < 5; ++level) {
        auto* data = RwRasterLock(raster.get(), static_cast<RwUInt8>(level), rwRASTERLOCKWRITE | rwRASTERLOCKNOFETCH);
        const auto w = std::max(1, 16 >> level), h = std::max(1, 8 >> level);
        Require(data && raster->width == w && raster->height == h && raster->stride >= 4 * w,
            "locked mip dimensions/stride");
        for (int y = 0; y < h; ++y) std::memset(data + y * raster->stride, 31 + 17 * level, 4 * w);
        Require(RwRasterUnlock(raster.get()) == raster.get() && raster->width == 16 && raster->height == 8,
            "unlock restores base dimensions");
    }
    for (int level = 0; level < 5; ++level) {
        auto* data = RwRasterLock(raster.get(), static_cast<RwUInt8>(level), rwRASTERLOCKREAD);
        Require(data != nullptr, "mip read lock");
        for (int y = 0; y < raster->height; ++y) for (int x = 0; x < raster->width * 4; ++x)
            Require(data[y * raster->stride + x] == 31 + 17 * level, "mip write/read mismatch");
        RwRasterUnlock(raster.get());
    }
    auto plain = Make(); Require(RwRasterGetNumLevels(plain.get()) == 1, "nonmip level count");
}
struct StandardGuard {
    int index; RwStandardFunc old;
    StandardGuard(int slot, RwStandardFunc replacement) : index(slot), old(RWSRCGLOBAL(stdFunc[slot])) {
        RWSRCGLOBAL(stdFunc[slot]) = replacement;
    }
    ~StandardGuard() { RWSRCGLOBAL(stdFunc[index]) = old; }
};
RwRaster* expected{}; int expected_flags{}, calls{}; bool succeed{};
std::array<RwUInt8, 16> buffer{};
RwBool Probe(void* out, void* in, RwInt32 flags) {
    Require(in == expected && flags == expected_flags, "standard callback argument packing");
    ++calls; if (out) *static_cast<void**>(out) = buffer.data(); return succeed;
}
RwBool Levels(void* out, void* in, RwInt32 flags) {
    Require(in == expected && flags == 0, "mip count dispatch");
    ++calls; *static_cast<int*>(out) = 7; return succeed;
}
RwBool FailCreate(void*, void*, RwInt32) { return FALSE; }
RwResEntry* presented_entry{};
void* expected_window{};
RwBool ShowProbe(void* raster, void* window, RwInt32 flags) {
    Require(raster == expected && window == expected_window && flags == expected_flags, "show dispatch arguments");
    Require(RWRESOURCESGLOBAL(res.freeEntries)->link.next == &presented_entry->link,
        "ShowRaster must age resources before device standard, even on failure");
    return succeed;
}
void Dispatch() {
    RwRaster raster{}; expected = &raster; calls = 0; succeed = true;
    {
        StandardGuard guard(rwSTANDARDRASTERLOCK, Probe); expected_flags = (0xfe << 8) + 5;
        Require(RwRasterLock(&raster, 0xfe, 5) == buffer.data() && calls == 1, "lock packs byte level and mode");
        succeed = false; Require(!RwRasterLock(&raster, 0xfe, 5), "failed lock output ignored");
    }
    {
        StandardGuard guard(rwSTANDARDRASTERUNLOCK, Probe); expected_flags = 0;
        Require(RwRasterUnlock(&raster) == &raster, "unlock must ignore standard failure");
    }
    {
        StandardGuard guard(rwSTANDARDRASTERLOCKPALETTE, Probe); expected_flags = 3; succeed = true;
        Require(RwRasterLockPalette(&raster, 3) == buffer.data(), "palette lock dispatch");
        succeed = false; Require(!RwRasterLockPalette(&raster, 3), "palette lock failure");
    }
    {
        StandardGuard guard(rwSTANDARDRASTERUNLOCKPALETTE, Probe); expected_flags = 0; raster.privateFlags = 0xff;
        Require(RwRasterUnlockPalette(&raster) == &raster && raster.privateFlags == 0xe7,
            "palette unlock masks flags despite failure");
    }
    {
        StandardGuard guard(rwSTANDARDRASTERGETMIPLEVELS, Levels); const auto before = calls;
        raster.cFormat = 0; Require(RwRasterGetNumLevels(&raster) == 1 && calls == before, "nonmip avoids standard");
        raster.cFormat = 0x80; succeed = false;
        Require(RwRasterGetNumLevels(&raster) == -1, "mip standard failure result");
        succeed = true; Require(RwRasterGetNumLevels(&raster) == 7, "mip standard output");
    }
    {
        StandardGuard guard(rwSTANDARDRASTERCREATE, FailCreate); const auto before = created;
        Require(!RwRasterCreate(16, 8, 32, rwRASTERTYPETEXTURE | rwRASTERFORMAT8888) && created == before,
            "failed device create must free object and skip plugins");
    }
    {
        int owner{}; RwResEntry* entry{};
        Require(RwResourcesAllocateResEntry(&owner, &entry, 64, nullptr) != nullptr, "show aging resource");
        presented_entry = entry; expected_window = &owner; expected_flags = 0; succeed = false;
        StandardGuard guard(rwSTANDARDRASTERSHOWRASTER, ShowProbe);
        Require(!RwRasterShowRaster(&raster, expected_window, 0) && entry == presented_entry,
            "failed show result or premature resource destruction");
        Require(RwResourcesFreeResEntry(entry) && !entry, "show aging resource cleanup");
        presented_entry = nullptr;
    }
}
void GameCopy() {
    auto raster = Make(); auto* data = RwRasterLock(raster.get(), 0, rwRASTERLOCKWRITE | rwRASTERLOCKNOFETCH);
    Require(data != nullptr, "copy initial lock");
    for (int y = 0; y < 8; ++y) std::memset(data + y * raster->stride, 0x19, 16 * 4);
    RwRasterUnlock(raster.get());
    auto* texture = RwTextureCreate(raster.get()); Require(texture != nullptr, "copy texture"); raster.release();
    struct Texture { RwTexture* p; ~Texture() { RwTextureDestroy(p); } } owner{texture};
    NativeRwRasterCopyRuntime runtime; const auto address = reinterpret_cast<std::uint32_t>(texture);
    std::array<std::uint8_t, 3 * 2 * 4> pixels{};
    for (size_t i = 0; i < pixels.size(); ++i) pixels[i] = static_cast<std::uint8_t>(81 + i);
    Require(RwRasterCopy::LockAndCopyData(address, pixels.data(), 4, 3, 3, 2, runtime) == address,
        "game rectangular copy return");
    Require(!RwRasterCopy::LockAndCopyData(address, pixels.data(), 15, 3, 3, 2, runtime), "game copy bounds rejection");
    auto* r = RwTextureGetRaster(texture); data = RwRasterLock(r, 0, rwRASTERLOCKREAD);
    Require(data != nullptr, "copy readback");
    for (int y = 0; y < 8; ++y) for (int x = 0; x < 16; ++x) for (int c = 0; c < 4; ++c) {
        const auto wanted = x >= 4 && x < 7 && y >= 3 && y < 5 ? pixels[((y - 3) * 3 + x - 4) * 4 + c] : 0x19;
        Require(data[y * r->stride + x * 4 + c] == wanted, "copy pixel/untouched-region mismatch");
    }
    RwRasterUnlock(r);
}
struct Resource { int id; RwResEntry* entry{}; };
std::vector<int> notifications;
void Notify(RwResEntry* entry) {
    auto* owner = static_cast<Resource*>(entry->owner);
    Require(owner && owner->entry == entry && entry->ownerRef == &owner->entry,
        "notify must precede clearing owner reference");
    Require(*reinterpret_cast<int*>(entry + 1) == owner->id, "resource payload alive during notify");
    notifications.push_back(owner->id);
}
void Allocate(Resource& owner, int size) {
    auto* entry = RwResourcesAllocateResEntry(&owner, &owner.entry, size, Notify);
    Require(entry && owner.entry == entry && entry->size == size, "resource allocation/owner link");
    *reinterpret_cast<int*>(entry + 1) = owner.id;
}
void Lifetime() {
    Require(RwResourcesGetArenaSize() == 4 * 1024 * 1024, "game resource arena size");
    notifications.clear(); Resource a{1}, b{2}; Allocate(a, 128); Allocate(b, 96);
    Require(RwResourcesFreeResEntry(a.entry) && !a.entry && notifications == std::vector<int>{1}, "explicit resource free");
    _rwResourcesPurge(); _rwResourcesPurge();
    Require(b.entry && notifications == std::vector<int>{1}, "Purge must age, not release entries");
    Require(RwResourcesEmptyArena() && !b.entry && notifications == std::vector<int>({1, 2}), "empty arena callback/owner clear");
    Require(RwResourcesEmptyArena() && notifications.size() == 2, "empty arena repeated destruction");
    // FreeResEntry also accepts an unlinked allocation from the engine memory API.
    Resource standalone{3}; standalone.entry = static_cast<RwResEntry*>(RwMalloc(sizeof(RwResEntry) + 16, 0));
    Require(standalone.entry != nullptr, "unlinked resource allocation");
    *standalone.entry = {}; standalone.entry->size = 16; standalone.entry->owner = &standalone;
    standalone.entry->ownerRef = &standalone.entry; standalone.entry->destroyNotify = Notify;
    *reinterpret_cast<int*>(standalone.entry + 1) = 3;
    Require(RwResourcesFreeResEntry(standalone.entry) && !standalone.entry &&
        notifications == std::vector<int>({1, 2, 3}), "unlinked resource callback and engine free path");
}
void Eviction(bool fragmented) {
    notifications.clear(); constexpr int megabyte = 1024 * 1024;
    const auto block = fragmented ? megabyte : 1200 * 1024;
    Resource a{1}, b{2}, c{3}, d{4}; Allocate(a, block); Allocate(b, block);
    _rwResourcesPurge(); Require(notifications.empty(), "age must not free");
    Allocate(c, block); RwResourcesUseResEntry(a.entry);
    Allocate(d, fragmented ? megabyte + megabyte / 2 : megabyte);
    Require(!b.entry && a.entry && d.entry && (fragmented ? !c.entry : c.entry != nullptr) &&
        notifications == (fragmented ? std::vector<int>{2, 3} : std::vector<int>{2}),
        "unused-first eviction or contiguous-hole requirement");
    Resource huge{5}; huge.entry = reinterpret_cast<RwResEntry*>(1);
    Require(!RwResourcesAllocateResEntry(&huge, &huge.entry, 4 * megabyte, Notify) && !huge.entry,
        "oversized allocation must fail and clear output");
    Require(!a.entry && !c.entry && !d.entry && notifications == std::vector<int>({2, 3, 1, 4}),
        "oversized allocation drains used entries oldest first");
    Require(RwResourcesEmptyArena(), "post-failure empty arena");
    Resource fresh{6}; Allocate(fresh, 64); Require(RwResourcesFreeResEntry(fresh.entry) && !fresh.entry,
        "allocation after destructive pressure failure");
    std::printf("resource pressure: fragmented=%d first eviction=%s; oversized request drains survivors then fails; retry works\n",
        fragmented, fragmented ? "B,C" : "B");
}
}
void RegisterRasterPlugin() {
    plugin_offset = RwRasterRegisterPlugin(4, 0x123440, Construct, Destruct, nullptr);
    Require(plugin_offset >= static_cast<int>(sizeof(RwRaster)), "raster plugin registration");
}
void RunRasterResourceCase(std::string_view name) {
    if (name == "raster_context") Context();
    else if (name == "raster_locks") Locks();
    else if (name == "raster_dispatch") Dispatch();
    else if (name == "raster_game_copy") GameCopy();
    else if (name == "resources_lifetime") Lifetime();
    else if (name == "resources_eviction") Eviction(false);
    else if (name == "resources_fragmentation") Eviction(true);
    else throw std::runtime_error("unknown raster/resource case");
}
}
