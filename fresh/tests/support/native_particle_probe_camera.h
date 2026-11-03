#pragma once
#include <rwcore.h>
#include <stdexcept>

namespace pal4::test {
struct NativeParticleProbeCamera {
    RwCamera* camera=RwCameraCreate();
    RwFrame* frame=RwFrameCreate();
    RwRaster* raster=RwRasterCreate(64,64,32,rwRASTERTYPECAMERATEXTURE|rwRASTERFORMAT8888);
    RwRaster* depth=RwRasterCreate(64,64,0,rwRASTERTYPEZBUFFER);
    bool updating{};
    ~NativeParticleProbeCamera(){
        if(updating) RwCameraEndUpdate(camera);
        if(camera){RwCameraSetRaster(camera,nullptr);RwCameraSetZRaster(camera,nullptr);RwCameraSetFrame(camera,nullptr);RwCameraDestroy(camera);}
        if(frame) RwFrameDestroy(frame);
        if(raster) RwRasterDestroy(raster);
        if(depth) RwRasterDestroy(depth);
    }
    void Begin(float y=0.0f) {
        BeginAt({0.0f,y,-80.0f});
    }
    void BeginAt(const RwV3d& translation,float near_clip=1.0f,float far_clip=1000.0f) {
        const RwV2d view{0.5f,0.5f};
        if(!camera || !frame || !raster || !depth ||
            !RwCameraSetFrame(camera,frame) || !RwFrameTranslate(frame,&translation,rwCOMBINEREPLACE))
            throw std::runtime_error("particle probe camera allocation");
        RwCameraSetRaster(camera,raster);RwCameraSetZRaster(camera,depth);
        RwCameraSetViewWindow(camera,&view);RwCameraSetNearClipPlane(camera,near_clip);RwCameraSetFarClipPlane(camera,far_clip);
        RwRGBA black{0,0,0,255};
        if(!RwCameraClear(camera,&black,rwCAMERACLEARIMAGE|rwCAMERACLEARZ))
            throw std::runtime_error("particle probe camera clear");
        if(!RwCameraBeginUpdate(camera))
            throw std::runtime_error("particle probe camera begin");
        updating=true;
    }
    unsigned EndPixels() {
        if(!updating) throw std::runtime_error("particle probe camera is not updating");
        // The bundled SDK adds a logo in EndUpdate (see ordered_75b6c0.md).
        // Read the game's draw output before that unrelated library overlay.
        auto* pixels=RwRasterLock(raster,0u,rwRASTERLOCKREAD);
        if(!pixels) throw std::runtime_error("particle probe checked raster read lock");
        unsigned colored{};
        for(unsigned y=0;y<64u;++y) for(unsigned x=0;x<64u;++x) {
            const auto* pixel=pixels+y*raster->stride+x*4u;
            colored+=(pixel[0] || pixel[1] || pixel[2])?1u:0u;
        }
        if(!RwRasterUnlock(raster)) throw std::runtime_error("particle probe read unlock");
        if(!RwCameraEndUpdate(camera)) throw std::runtime_error("particle probe camera end");
        updating=false;
        return colored;
    }
};
}
