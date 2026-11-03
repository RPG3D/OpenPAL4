#pragma once
#include <rwcore.h>
#include <cstdint>
#include <memory>
#include <stdexcept>
namespace pal4::test {
inline std::uint64_t GameOverGpuEnergy(RwCamera* camera){
    auto* raster=camera?RwCameraGetRaster(camera):nullptr;
    if(!raster)throw std::runtime_error("GameOver GPU probe missing raster");
    std::unique_ptr<RwImage,decltype(&RwImageDestroy)> image(RwImageCreate(RwRasterGetWidth(raster),RwRasterGetHeight(raster),32),&RwImageDestroy);
    if(!image || !RwImageAllocatePixels(image.get()) || !RwImageSetFromRaster(image.get(),raster))throw std::runtime_error("GameOver GPU readback failed");
    std::uint64_t energy{};
    for(int y=0;y<RwImageGetHeight(image.get());++y){
        const auto* row=reinterpret_cast<const RwRGBA*>(RwImageGetPixels(image.get())+y*RwImageGetStride(image.get()));
        for(int x=0;x<RwImageGetWidth(image.get());++x)energy+=row[x].red+row[x].green+row[x].blue;
    }return energy;
}
}
