#pragma once
#include "Systems/System.h"

namespace CE
{
	class UpdateTopDownCamSystem final : 
		public System
	{
	public:
		void Update(World& world, float dt) override;
		void Render(const World& world) override;

		SystemStaticTraits GetStaticTraits() const override
		{
			SystemStaticTraits traits {};
			traits.mShouldTickWhilstPaused = true;
			return traits;
		}

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(UpdateTopDownCamSystem);
	};
}