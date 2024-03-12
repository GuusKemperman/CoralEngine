#pragma once

#ifdef PLATFORM_WINDOWS
#include "Platform/PC/Rendering/RendererPC.h"
#elif PLATFORM_***REMOVED***
#include "Platform/***REMOVED***/Rendering/Renderer***REMOVED***.h"
#endif

REFLECT_AT_START_UP(Renderer, Engine::Renderer);
