#pragma once
#include "Systems/System.h"

namespace CE
{
	class StaticMeshRenderingSystem final :
		public System
	{
	public:
		void Render(const World& world, RenderCommandQueue& commandQueue) const override;

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(StaticMeshRenderingSystem);
	};
}
