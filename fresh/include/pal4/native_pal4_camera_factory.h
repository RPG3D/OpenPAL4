#pragma once
struct RwCamera;
namespace pal4 {
// Uses SDK allocation/plugins and the IDA75AFA0 game callback variant.
[[nodiscard]] RwCamera* CreateNativePal4Camera();
}
