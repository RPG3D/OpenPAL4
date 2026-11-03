#include <windows.h>
#include "support/native_test_display.h"
#include <rwcore.h>
#include <rpworld.h>
#include <d3d9.h>
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <memory>
#include <set>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <vector>

// Probe the installed SDK standard callbacks, then exercise their public
// dictionary caller. No native texture codec is implemented by the host.
extern "C" RwInt32 _RwD3D9RasterExtOffset; // test-only failure observation
static_assert(sizeof(RwRaster) == 52 && sizeof(RwTexture) == 88);
static_assert(offsetof(RwTexture, filterAddressing) == 80 && offsetof(RwRaster, cFormat) == 35);
static_assert(std::is_same_v<RwStandardFunc, RwBool(__cdecl *)(void *, void *, RwInt32)>);
namespace {
void Check(bool ok, const char *text) {
    if (!ok)
        throw std::runtime_error(text);
}
std::set<void *> rasters;
void *Born(void *p, RwInt32, RwInt32) {
    rasters.insert(p);
    return p;
}
struct DestroyObservation {
    bool enabled{}, cube{};
    RwUInt32 format{};
    D3DFORMAT resource_format = D3DFMT_UNKNOWN;
} destroyed;
void *Died(void *p, RwInt32, RwInt32) {
    if (destroyed.enabled) {
        auto *ext = static_cast<unsigned char *>(p) + _RwD3D9RasterExtOffset;
        std::memcpy(&destroyed.format, ext + 24, 4);
        IDirect3DBaseTexture9 *base{};
        std::memcpy(&base, ext, 4);
        destroyed.cube = base && base->GetType() == D3DRTYPE_CUBETEXTURE;
        if (destroyed.cube) {
            D3DSURFACE_DESC desc{};
            static_cast<IDirect3DCubeTexture9 *>(base)->GetLevelDesc(0, &desc);
            destroyed.resource_format = desc.Format;
        }
    }
    rasters.erase(p);
    return p;
}
struct Session {
    HWND window{};
    Session() {
        Check(RwEngineInit(nullptr, 0, 0), "engine init");
        Check(RpWorldPluginAttach(), "world attach");
        Check(RwRasterRegisterPlugin(0, 0x123479, Born, Died, nullptr) >= 0, "raster lifetime observer");
        const auto display = pal4::test::TestDisplay();
        window = CreateWindowA("STATIC", "PAL4 native texture", WS_OVERLAPPEDWINDOW,
                               display.bounds.left, display.bounds.top, 64, 64,
                               nullptr, nullptr, GetModuleHandleA(nullptr), nullptr);
        Check(window, "window");
        RwEngineOpenParams p{window};
        Check(RwEngineOpen(&p), "engine open");
        Check(RwEngineStart(), "engine start");
    }
    ~Session() {
        RwEngineStop();
        RwEngineClose();
        RwEngineTerm();
        DestroyWindow(window);
    }
};
struct DropTexture {
    void operator()(RwTexture *p) const {
        if (p)
            RwTextureDestroy(p);
    }
};
struct DropDict {
    void operator()(RwTexDictionary *p) const {
        if (p)
            RwTexDictionaryDestroy(p);
    }
};
struct DropStream {
    void operator()(RwStream *p) const {
        if (p)
            RwStreamClose(p, nullptr);
    }
};
using Texture = std::unique_ptr<RwTexture, DropTexture>;
using Dictionary = std::unique_ptr<RwTexDictionary, DropDict>;
using Stream = std::unique_ptr<RwStream, DropStream>;
using Bytes = std::vector<RwUInt8>;
RwUInt32 Word(const Bytes &b, size_t off) {
    Check(off + 4 <= b.size(), "field read bounds");
    RwUInt32 w;
    std::memcpy(&w, b.data() + off, 4);
    return w;
}
void Word(Bytes &b, size_t off, RwUInt32 w) {
    Check(off + 4 <= b.size(), "field write bounds");
    std::memcpy(b.data() + off, &w, 4);
}
RwBool Standard(int index, void *out, void *in) { return RWSRCGLOBAL(stdFunc)[index](out, in, 0); }
RwUInt32 Size(RwTexture *t) {
    RwUInt32 size = 0;
    Check(Standard(rwSTANDARDNATIVETEXTUREGETSIZE, &size, t), "native size callback");
    return size;
}
Texture Make(bool mips = false, bool automatic = false, D3DFORMAT compressed = D3DFMT_UNKNOWN,
             int width = 8) {
    int flags = rwRASTERTYPETEXTURE | (mips ? rwRASTERFORMATMIPMAP : 0) |
                (automatic ? rwRASTERFORMATAUTOMIPMAP : 0);
    auto *r = compressed == D3DFMT_UNKNOWN ? RwRasterCreate(width, width, 32, flags | rwRASTERFORMAT8888)
                                           : RwD3D9RasterCreate(width, width, compressed, flags);
    Check(r, "raster creation");
    Texture t(RwTextureCreate(r));
    Check(t != nullptr, "texture creation");
    RwTextureSetName(t.get(), "native_stone");
    RwTextureSetMaskName(t.get(), "native_mask");
    (void)RwTextureSetFilterMode(t.get(), rwFILTERLINEARMIPLINEAR);
    (void)RwTextureSetAddressingU(t.get(), rwTEXTUREADDRESSCLAMP);
    (void)RwTextureSetAddressingV(t.get(), rwTEXTUREADDRESSMIRROR);
    int count = automatic ? 1 : RwRasterGetNumLevels(r);
    for (int level = 0; level < count; ++level) {
        auto *p = RwRasterLock(r, level, rwRASTERLOCKWRITE | rwRASTERLOCKNOFETCH);
        Check(p, "fill level lock");
        const int rows = compressed == D3DFMT_UNKNOWN ? r->height : std::max(1, r->height / 4);
        std::memset(p, 0x31 + level, r->stride * rows);
        RwRasterUnlock(r);
    }
    return t;
}
Bytes Write(RwTexture *t) {
    Stream s(RwStreamOpen(rwSTREAMMEMORY, rwSTREAMWRITE, nullptr));
    Check(s != nullptr, "write open");
    Check(Standard(rwSTANDARDNATIVETEXTUREWRITE, s.get(), t), "native write");
    RwMemory m{};
    Check(RwStreamClose(s.release(), &m), "write close");
    Bytes bytes(m.start, m.start + m.length);
    RwFree(m.start);
    return bytes;
}
Texture Read(Bytes &b, bool ok = true) {
    RwMemory memory{b.data(), static_cast<RwUInt32>(b.size())};
    Stream s(RwStreamOpen(rwSTREAMMEMORY, rwSTREAMREAD, &memory));
    Check(s != nullptr, "read open");
    auto *sentinel = reinterpret_cast<RwTexture *>(0x1234);
    auto *result = sentinel;
    auto success = Standard(rwSTANDARDNATIVETEXTUREREAD, s.get(), &result);
    if (bool(success) != ok) {
        RwError error{};
        RwErrorGet(&error);
        std::fprintf(stderr, "native read at %u/%zu error %d plugin %d format %x flags %x\n",
                     s->Type.memory.position, b.size(), error.errorCode, error.pluginID,
                     b.size() > 88 ? Word(b, 84) : 0, b.size() > 99 ? b[99] : 0);
    }
    Check(bool(success) == ok, "native read expected result");
    if (!ok) {
        Check(result == sentinel, "failed reader leaves output untouched");
        return {};
    }
    Check(s->Type.memory.position == b.size(), "native reader consumes complete payload");
    return Texture(result);
}
void RoundTrip(Bytes &b) {
    auto result = Read(b);
    auto again = Write(result.get());
    Check(again == b, "all header/pitch/mip/face bytes roundtrip");
    Check(Size(result.get()) == b.size(), "size agrees with actual bytes");
}
void Layout() {
    auto t = Make();
    auto b = Write(t.get());
    Check(Size(t.get()) == b.size() && b.size() == 360, "8x8 RGBA native size includes STRUCT header");
    Check(Word(b, 0) == rwID_STRUCT && Word(b, 4) == b.size() - 12 && (Word(b, 8) & 0xffff) == 0xa,
          "SDK structure header/build");
    Check(Word(b, 12) == 9 && Word(b, 16) == (t->filterAddressing & 0xffff), "platform9/filter/address");
    Check(!std::strcmp(reinterpret_cast<char *>(b.data() + 20), "native_stone") &&
              !std::strcmp(reinterpret_cast<char *>(b.data() + 52), "native_mask"),
          "fixed32 name/mask");
    Check(Word(b, 84) == rwRASTERFORMAT8888 && Word(b, 88) == D3DFMT_A8R8G8B8, "raster and D3D formats");
    Check(b[92] == 8 && b[94] == 8 && b[96] == 32 && b[97] == 1 && b[98] == rwRASTERTYPETEXTURE &&
              b[99] == 1 && Word(b, 100) == 256,
          "16-byte raster header and first payload size");
    RoundTrip(b);
    Texture empty(RwTextureCreate(nullptr));
    Check(Size(empty.get()) == 84, "null raster reports84 although writer requires raster");
}
void Mips() {
    auto t = Make(true);
    auto b = Write(t.get());
    Check(b[97] == 4 && b[99] == 1, "manual four mips");
    size_t cursor = 100;
    for (int level = 0; level < 4; ++level) {
        int w = std::max(1, 8 >> level);
        auto n = Word(b, cursor);
        Check(n == w * w * 4, "uncompressed pitch times height");
        Check(b[cursor + 4] == 0x31 + level, "ascending mip order");
        cursor += 4 + n;
    }
    Check(cursor == b.size(), "mip lengths exhaust native payload");
    RoundTrip(b);
}
void Auto() {
    auto t = Make(true, true);
    auto b = Write(t.get());
    Check((b[99] & 4) != 0 && b.size() == 360, "hardware automipmap writes only base level");
    auto r = Read(b);
    Check((RwRasterGetFormat(r->raster) & rwRASTERFORMATAUTOMIPMAP) != 0,
          "auto flag restored after read");
    Check(Write(r.get()) == b, "auto base payload roundtrip");
}
void Compressed() {
    for (auto format : {D3DFMT_DXT1, D3DFMT_DXT3, D3DFMT_DXT5}) {
        auto t = Make(true, false, format);
        auto b = Write(t.get());
        Check((b[99] & 8) != 0 && Word(b, 88) == format && b[97] == 4,
              "compressed native format and levels");
        size_t cursor = 100;
        for (int level = 0; level < 4; ++level) {
            const int blocks = std::max(1, (8 >> level) / 4),
                      n = blocks * blocks * (format == D3DFMT_DXT1 ? 8 : 16);
            Check(Word(b, cursor) == n, "compressed rows use max1(height/4), including tiny mips");
            cursor += 4 + n;
        }
        Check(cursor == b.size(), "compressed size");
        RoundTrip(b);
    }
}
void Cube() {
    for (auto format : {D3DFMT_UNKNOWN, D3DFMT_DXT1, D3DFMT_DXT3, D3DFMT_DXT5}) {
        auto t = Make(true, false, format);
        auto b = Write(t.get());
        Bytes face(b.begin() + 100, b.end());
        b.resize(100);
        b[99] |= 2;
        for (int f = 0; f < 6; ++f) {
            auto copy = face;
            copy[4] = static_cast<RwUInt8>(f + 1);
            b.insert(b.end(), copy.begin(), copy.end());
        }
        Word(b, 4, static_cast<RwUInt32>(b.size() - 12));
        if (format == D3DFMT_UNKNOWN) {
            const auto before = rasters;
            destroyed = {};
            destroyed.enabled = true;
            Read(b, false);
            destroyed.enabled = false;
            Check(rasters == before && destroyed.cube && destroyed.resource_format == D3DFMT_A8R8G8B8 &&
                      destroyed.format == 0,
                  "uncompressed cube allocates real resource but rejects zero extension format, then "
                  "destroys raster");
            continue;
        }
        auto texture = Read(b);
        Check(Write(texture.get()) == b && Size(texture.get()) == b.size(),
              "cube six faces/mips roundtrip");
        Check(RwD3D9SetTexture(texture.get(), 0), "bind cube");
        auto *device = static_cast<IDirect3DDevice9 *>(RwD3D9GetCurrentD3DDevice());
        IDirect3DBaseTexture9 *base{};
        Check(SUCCEEDED(device->GetTexture(0, &base)) && base && base->GetType() == D3DRTYPE_CUBETEXTURE,
              "real D3D cube resource");
        auto *cube = static_cast<IDirect3DCubeTexture9 *>(base);
        for (int f = 0; f < 6; ++f) {
            D3DLOCKED_RECT lock{};
            Check(SUCCEEDED(cube->LockRect(static_cast<D3DCUBEMAP_FACES>(f), 0, &lock, nullptr,
                                           D3DLOCK_READONLY)),
                  "cube face lock");
            Check(*static_cast<RwUInt8 *>(lock.pBits) == f + 1, "face order +X -X +Y -Y +Z -Z");
            cube->UnlockRect(static_cast<D3DCUBEMAP_FACES>(f), 0);
        }
        base->Release();
        RwD3D9SetTexture(nullptr, 0);
    }
}
void Headers() {
    auto t = Make();
    auto original = Write(t.get());
    const auto before = rasters;
    for (auto version : {0x35000u, 0x37002u, 0x34000u, 0x37003u}) {
        auto b = original;
        const auto packed = ((version - 0x30000) & 0x3ff00) << 14 | (version & 0x3f) << 16 | 0x65;
        Word(b, 8, packed);
        auto r = Read(b, version >= 0x35000 && version <= 0x37002);
    }
    Check(rasters == before, "header tests release resources");
    auto b = original;
    Word(b, 12, 8);
    Read(b, false);
    Check(rasters == before, "wrong platform rejects before allocation");
    // STRUCT length is not used to bound the fixed fields or mip data.
    b = original;
    Word(b, 4, 1);
    auto result = Read(b);
    Check(result != nullptr, "native reader ignores declared STRUCT length");
}
void Truncated() {
    auto t = Make(true);
    auto original = Write(t.get());
    const auto before = rasters;
    for (size_t length : {size_t(0), size_t(8), size_t(50), size_t(90), size_t(100), size_t(102),
                          size_t(105), original.size() - 1}) {
        auto b = original;
        b.resize(length);
        Read(b, false);
        Check(rasters == before, "truncated native stream destroys created raster and unlocks");
    }
}
struct Sink {
    std::vector<RwUInt32> sizes;
    size_t fail = 0;
};
RwBool SinkWrite(void *p, const void *, RwUInt32 n) {
    auto &s = *static_cast<Sink *>(p);
    s.sizes.push_back(n);
    return s.sizes.size() != s.fail;
}
void WriterFailure() {
    auto t = Make(false, false, D3DFMT_UNKNOWN, 32);
    for (size_t fail : {size_t(0), size_t(4), size_t(5)}) {
        Sink sink{{}, fail};
        RwStreamCustom callbacks{nullptr, nullptr, SinkWrite, nullptr, &sink};
        Stream stream(RwStreamOpen(rwSTREAMCUSTOM, rwSTREAMWRITE, &callbacks));
        Check(stream != nullptr, "custom sink");
        Check(bool(Standard(rwSTANDARDNATIVETEXTUREWRITE, stream.get(), t.get())) == (fail == 0),
              "writer forwards size/data failure");
        if (!fail) {
            Check(sink.sizes == std::vector<RwUInt32>({12, 72, 16, 4, 1024, 1024, 1024, 1024}),
                  "native writer emits at most1024-byte data chunks");
            Check(t->raster->cpPixels == nullptr, "successful writer unlocks");
        } else {
            Check(t->raster->cpPixels != nullptr, "size/data failure leaves raster locked");
            Check(RwRasterUnlock(t->raster), "diagnostic cleanup of retained lock");
        }
    }
}
void DictionaryCase() {
    Dictionary original(RwTexDictionaryCreate());
    auto t = Make(true);
    auto expected = Write(t.get());
    Check(RwTexDictionaryAddTexture(original.get(), t.get()) == t.get(), "dictionary add");
    t.release();
    Stream s(RwStreamOpen(rwSTREAMMEMORY, rwSTREAMWRITE, nullptr));
    Check(RwTexDictionaryStreamWrite(original.get(), s.get()) == original.get(),
          "public dictionary write");
    RwMemory memory{};
    Check(RwStreamClose(s.release(), &memory), "dictionary write close");
    Check(memory.length == RwTexDictionaryStreamGetSize(original.get()) + 12, "public dictionary size");
    Stream read(RwStreamOpen(rwSTREAMMEMORY, rwSTREAMREAD, &memory));
    Check(RwStreamFindChunk(read.get(), rwID_TEXDICTIONARY, nullptr, nullptr), "find dictionary");
    Dictionary result(RwTexDictionaryStreamRead(read.get()));
    Check(result != nullptr, "public dictionary read");
    auto *texture = RwTexDictionaryFindNamedTexture(result.get(), "native_stone");
    Check(texture && texture->refCount == 1 && Write(texture) == expected,
          "public dictionary invokes native codec and retains mip pixels");
    read.reset();
    RwFree(memory.start);
}
} // namespace
int main(int argc, char **argv) {
    try {
        Check(argc == 2, "one case required");
        {
            Session session;
            const auto before = rasters;
            std::string_view name = argv[1];
            if (name == "layout")
                Layout();
            else if (name == "mips")
                Mips();
            else if (name == "auto")
                Auto();
            else if (name == "compressed")
                Compressed();
            else if (name == "cube")
                Cube();
            else if (name == "headers")
                Headers();
            else if (name == "truncated")
                Truncated();
            else if (name == "writer_failure")
                WriterFailure();
            else if (name == "dictionary")
                DictionaryCase();
            else
                throw std::runtime_error("unknown native texture case");
            Check(rasters == before, "no raster lifetime leaks");
        }
        Check(rasters.empty(), "engine teardown raster ownership");
        std::puts("PASS");
        return 0;
    } catch (const std::exception &e) {
        std::fprintf(stderr, "FAIL: %s\n", e.what());
        return 1;
    }
}
