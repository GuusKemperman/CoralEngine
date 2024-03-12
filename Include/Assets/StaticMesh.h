#pragma once

#ifdef PLATFORM_WINDOWS
#include "Platform/PC/Rendering/MeshPC.h"
#elif PLATFORM_***REMOVED***
#include "Platform/***REMOVED***/Rendering/StaticMesh***REMOVED***.h"
#endif

REFLECT_AT_START_UP(StaticMesh, Engine::StaticMesh);
