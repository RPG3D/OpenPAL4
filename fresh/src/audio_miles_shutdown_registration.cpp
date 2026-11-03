#include "pal4/audio_manager.h"

namespace pal4 {

int AudioSystem::RegisterMilesShutdown(
    CrtExitCallback callback, ICrtAtexitApi& atexit_api) {
    (void)atexit_api.Register(callback);
    return 0;
}

}  // namespace pal4
