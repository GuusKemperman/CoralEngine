#pragma once
#include "Assets/Core/AssetHandle.h"
#include "Meta/MetaReflect.h"

namespace CE
{
	class StaticMesh;
	class Material;

	class ParticleMeshRendererComponent
	{
	public:
		AssetHandle<StaticMesh> mParticleMesh{};
		AssetHandle<Material> mParticleMaterial{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ParticleMeshRendererComponent);
	};
}
