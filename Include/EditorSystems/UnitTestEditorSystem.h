#ifdef EDITOR
#pragma once
#include "EditorSystems/EditorSystem.h"

namespace CE
{
	class UnitTestEditorSystem final :
		public EditorSystem
	{
	public:
		UnitTestEditorSystem();

		void Tick(float deltaTime) override;

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(UnitTestEditorSystem);
	};
}
#endif // EDITOR