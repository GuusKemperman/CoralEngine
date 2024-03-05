#pragma once

#ifdef PLATFORM_WINDOWS
#include "Platform/PC/Rendering/TexturePC.h"
#elif PLATFORM_***REMOVED***
#include "Platform/***REMOVED***/Rendering/Texture***REMOVED***.h"
#endif

REFLECT_AT_START_UP(Texture, Engine::Texture);
