//#pragma once
//#include "Systems/System.h"
//
//
//namespace Engine
//{
//	class Registry;
//
//	class ParticleMeshRenderingSystem final :
//		public System
//	{
//	public:
//		void Render(const World& world) override;
//
//		SystemStaticTraits GetStaticTraits() const override
//		{
//			SystemStaticTraits traits{};
//			traits.mPriority = static_cast<int>(TickPriorities::Render);
//			return traits;
//		}
//
//	private:
//		static void SimpleRender(const Registry& reg);
//		static void ColorRender(const Registry& reg);
//		static void ColorOverTimeRender(const Registry& reg);
//
//		friend ReflectAccess;
//		static MetaType Reflect();
//		REFLECT_AT_START_UP(ParticleMeshRenderingSystem);
//	};
//}