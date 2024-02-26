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

#define FRAME_BUFFER_COUNT 2
#define MAX_LIGHTS 20
#define MAX_MESHES 65000

//DESCRIPTOR HEAPS
#define RT_HEAP						0
#define DEPTH_HEAP					1
#define RESOURCE_HEAP				2
#define SAMPLER_HEAP				3
#define NUM_DESC_HEAPS SAMPLER_HEAP+1

//RESOURCES
#define RENDER_TARGETS_RSC				0
#define MSAA_RENDER_TARGETS_RSC			2
#define MODEL_MATRIX_RSC				4
#define DEPTH_STENCIL_RSC				5
#define NUM_RESOURCES DEPTH_STENCIL_RSC+1

//CONSTANT BUFFERS
#define CAM_MATRIX_CB			0
#define MATERIAL_CB				1
#define LIGHT_CB				2
#define MESH_INDEX_CB			3
#define NUM_CBS MESH_INDEX_CB+	1

//PIPELINES
#define PBR_PIPELINE				0
#define SKY_PIPELINE				1
#define NUM_PIPELINES SKY_PIPELINE +1

//RESOURCE HEAP SLOTS
#define  MODEL_MAT_SB_SLOT			0
#define  TEXTURE_START				1
