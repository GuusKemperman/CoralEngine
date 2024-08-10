#pragma once
#include "Systems/System.h"

namespace CE
{
	class Registry;

	class ParticleDebugVisualizationSystem final :
		public System
	{

	public:
		void Render(const World& world, RenderCommandQueue& commandQueue) const override;

		SystemStaticTraits GetStaticTraits() const override
		{
			SystemStaticTraits traits{};
			traits.mPriority = static_cast<int>(TickPriorities::Render);
			return traits;
		}

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ParticleDebugVisualizationSystem);
	};
}