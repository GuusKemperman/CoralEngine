#ifdef EDITOR
#pragma once
#include "Meta/MetaReflect.h"

namespace CE
{
	class EditorSystem
	{
	public:
		EditorSystem(std::string_view name);

		EditorSystem(EditorSystem&&) = delete;
		EditorSystem(const EditorSystem&) = delete;

		EditorSystem& operator=(EditorSystem&&) = delete;
		EditorSystem& operator=(const EditorSystem&) = delete;

		virtual ~EditorSystem() = default;

		const std::string& GetName() const { return mName; }

		//********************************//
		//		Virtual functions		  //
		//********************************//

		/*
		Called every frame
		*/
		virtual void Tick([[maybe_unused]] const float deltaTime) {};

		/*
		Saves the state of the system. Does not NEED to be implemented, but can be useful
		if you have some settings that you want to carry over to the next time the system
		is started.

		This function is called right before the destructor.

		It is safe to write both binary or readable information to the stream.
		*/
		virtual void SaveState([[maybe_unused]] std::ostream& toStream) const {};

		/*		
		LoadState is called right after construction, but only if the state has ever been 
		saved before; if SaveState has never ran before, LoadState will not be called.
		*/
		virtual void LoadState([[maybe_unused]] std::istream& fromStream) {};

	protected:
		/*
		A generic helper function, akin to ImGui::Begin.

		The reason this is here is to prevent each window from looking very different,
		and we want most windows to have the same name as the editor system as well as a 
		functioning close button.
		
		Every call to Begin must have a call to End, regardless of if this returns true.
		*/
		virtual bool Begin(ImGuiWindowFlags flags = {});

		virtual void End() { ImGui::End(); }

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(EditorSystem);

		std::string mName{};
	};
}
#endif // EDITOR