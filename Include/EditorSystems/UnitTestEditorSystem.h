#ifdef EDITOR
#include "EditorSystems/EditorSystem.h"

namespace Engine
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