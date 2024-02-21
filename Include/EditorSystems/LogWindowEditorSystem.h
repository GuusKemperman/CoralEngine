#ifdef EDITOR
#include "EditorSystems/EditorSystem.h"

namespace Engine
{
	class LogWindow final :
		public EditorSystem
	{
	public:
		LogWindow();
		~LogWindow() override;

		void Tick(float deltaTime) override;

		void SaveState(std::ostream& toStream) const override;
		void LoadState(std::istream& fromStream) override;

	private:
		void DisplayMenuBar();
		void DisplayWindowContents();

		bool mAutoScroll = true;
		ImGuiWindowFlags mFlags = ImGuiWindowFlags_MenuBar;

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(LogWindow);
	};
}
#endif // EDITOR