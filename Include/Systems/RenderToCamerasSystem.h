#pragma once
#ifdef PLATFORM_WINDOWS
#include "Platform/PC/Rendering/RendererPC.h"
#endif
//#include "System.h"
//
//namespace Engine
//{
//	class RenderToCamerasSystem final :
//		public System
//	{
//	public:
//		void Render(const World& world) override;
//
//		SystemStaticTraits GetStaticTraits() const override
//		{
//			SystemStaticTraits traits{};
//			traits.mPriority = static_cast<int>(TickPriorities::PostRender) + (TickPriorityStepSize >> 1);
//			return traits;
//		}
//
//	private:
//		friend ReflectAccess;
//		static MetaType Reflect();
//		REFLECT_AT_START_UP(RenderToCamerasSystem);
//	};
//}