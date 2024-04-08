#pragma once
#include "Rendering/ISubRenderer.h"
#include "Platform/PC/Rendering/DX12Classes/DXDefines.h"

namespace CE
{
	class ShadowMapRenderer final : 
		public ISubRenderer
	{
	public:
		ShadowMapRenderer();
		~ShadowMapRenderer();
		void Render(const World& world) override;

	private:
		ComPtr<ID3D12PipelineState>  mShadowMapPipeline;
		ComPtr<ID3D12PipelineState>  mShadowMapSkinnedPipeline;
	};
}

