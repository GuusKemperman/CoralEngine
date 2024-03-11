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
#define MAX_LIGHTS 20
#define MAX_MESHES 65000
#define MAX_SKINNED_MESHES 128

//DESCRIPTOR HEAPS
#define RT_HEAP						0
#define DEPTH_HEAP					1
#define RESOURCE_HEAP				2
#define SAMPLER_HEAP				3
#define NUM_DESC_HEAPS SAMPLER_HEAP+1

//RESOURCES
#define RENDER_TARGETS_RSC				0
#define MSAA_RENDER_TARGETS_RSC			2
#define DEPTH_STENCIL_RSC				4
#define NUM_RESOURCES DEPTH_STENCIL_RSC+1
#define RT_COUNT 4

//HEAP SLOTS
#define IMGUI_SLOT 0
#define TEX_START 4

//CONSTANT BUFFERS
#define CAM_MATRIX_CB					0
#define MATERIAL_CB						1
#define LIGHT_CB						2
#define MODEL_MATRIX_CB					3
#define FINAL_BONE_MATRIX_CB			4
#define NUM_CBS FINAL_BONE_MATRIX_CB+	1

//ROOT SIGNATURES
#define MAIN_ROOT_SIGNATURE							0
#define SKINNED_ROOT_SIGNATURE						1
#define NUM_ROOT_SIGNATURES SKINNED_ROOT_SIGNATURE+	1

//PIPELINES
#define PBR_PIPELINE						0
#define SKY_PIPELINE						1
#define PBR_SKINNED_PIPELINE				2
#define NUM_PIPELINES PBR_SKINNED_PIPELINE +1

//RESOURCE HEAP SLOTS
#define  MODEL_MAT_SB_SLOT			0
#define  TEXTURE_START				1

//DEBUG RENDERER
#define MAX_LINES 32760