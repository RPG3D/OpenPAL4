#pragma once

#include <rwcore.h>

namespace pal4 {

// RenderWare image callback backed by the reconstructed PAL4 TGA path.
RwImage* ReadNativeGameTga(const RwChar* filename);

}  // namespace pal4
