#pragma once
#include "Systems/System.h"

namespace CE
{
	class UpdateCameraMatricesSystem final :
		public System
	{
	public:
		void Update(World& world, float dt) override;

		void Render(const World& world, RenderCommandQueue& commandQueue) const override;

		SystemStaticTraits GetStaticTraits() const override
		{
			SystemStaticTraits traits{};
			traits.mPriority = static_cast<int>(TickPriorities::PostTick);
			traits.mShouldTickBeforeBeginPlay = true;
			traits.mShouldTickWhilstPaused = true;
			return traits;
		}

	private:
		static void UpdateMatrices(World& world);

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(UpdateCameraMatricesSystem);
	};
}