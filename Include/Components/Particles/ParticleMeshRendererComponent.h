#pragma once
#include "Meta/MetaReflect.h"

namespace CE
{
	class StaticMesh;
	class Material;

	class ParticleMeshRendererComponent
	{
	public:
		std::shared_ptr<const StaticMesh> mParticleMesh{};
		std::shared_ptr<const Material> mParticleMaterial{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ParticleMeshRendererComponent);
	};
}
