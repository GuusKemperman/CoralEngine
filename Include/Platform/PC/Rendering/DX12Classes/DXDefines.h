#pragma once

// DirectX 12 specific headers.
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

// D3D12 extension library.
#pragma warning(push)
#pragma warning(disable: 4324)
#include "dx12/d3dx12.h"
#pragma warning(pop) 

//WINDOWS STUFF
#pragma warning(push)
#pragma warning(disable: 4005)
#ifndef  NOMINMAX
#define NOMINMAX
#endif
#include <wrl.h>
#undef APIENTRY
#include <Windows.h>
#pragma warning(pop) 
using namespace Microsoft::WRL;

#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

#define FRAME_BUFFER_COUNT 2
#define MAX_MESHES 65000

//DESCRIPTOR HEAPS
#define RT_HEAP						0
#define DEPTH_HEAP					1
#define RESOURCE_HEAP				2
#define SAMPLER_HEAP				3
#define NUM_DESC_HEAPS SAMPLER_HEAP+1

//DEBUG RENDERER
#define MAX_LINES 32760