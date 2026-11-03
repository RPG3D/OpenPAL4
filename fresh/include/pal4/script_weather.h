#pragma once
#include <cstdint>
namespace pal4 {
// Narrow script boundary for original5D3FB0/5BCF90. Both VM declarations
// return void; incidental native EAX/AL values are deliberately not exposed.
class IScriptWeatherRuntime {
public:
    virtual ~IScriptWeatherRuntime() = default;
    virtual void OpenWeather(std::int32_t type) = 0;
    virtual void CloseWeather() = 0;
};
}
