#pragma once

#ifdef PLATFORM_WINDOWS
#include "Platform/PC/Rendering/SkinnedMeshPC.h"
#elif PLATFORM_***REMOVED***
#include "Platform/***REMOVED***/Rendering/SkinnedMesh***REMOVED***.h"
#endif

REFLECT_AT_START_UP(SkinnedMesh, Engine::SkinnedMesh);