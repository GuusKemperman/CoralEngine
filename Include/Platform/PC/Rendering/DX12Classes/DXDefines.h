#pragma once

// DirectX 12 specific headers.
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4324)
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wclass-conversion"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#pragma clang diagnostic ignored "-Wuninitialized"
#endif

// D3D12 extension library.
#include "dx12/d3dx12.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif

// WINDOWS STUFF
#pragma warning(push)
#pragma warning(disable : 4005)
#ifndef NOMINMAX
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
#define MAX_TEXTS 100
#define MAX_QUADS 100
#define MAX_CHAR_PER_TEXT 300
#define MAX_PARTICLES 65000
#define MAX_SKINNED_MESHES 5012
#define MAX_LIGHTS_PER_CLUSTER 1024
#define MAX_TEXT_QUADS 65000

// DESCRIPTOR HEAPS
#define RT_HEAP 0
#define DEPTH_HEAP 1
#define RESOURCE_HEAP 2
#define SAMPLER_HEAP 3
#define NUM_DESC_HEAPS SAMPLER_HEAP + 1

// DEBUG RENDERER
#define MAX_LINES 32760
#define MAX_LINE_VERTICES MAX_LINES * 2

// MSAA
#define MSAA_COUNT 4
#define MSAA_QUALITY 0
