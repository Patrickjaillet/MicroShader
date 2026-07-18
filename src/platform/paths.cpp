#include "paths.h"

#include <SDL3/SDL.h>

std::string asset_path(const std::string& relative_path)
{
    const char* base_path = SDL_GetBasePath();
    std::string result = base_path != nullptr ? base_path : "./";
    result += "assets/";
    result += relative_path;
    return result;
}
