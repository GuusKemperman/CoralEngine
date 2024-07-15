#pragma once
#include "Assets/Asset.h"

#include "Scripting/ScriptFunc.h"
#include "Scripting/ScriptField.h"
#include "Scripting/ScriptErrors.h"

namespace CE
{
	class BinaryGSONObject;
	class MetaType;

	class Script final :
		public Asset
	{
	public:
		Script(std::string_view name);
		Script(AssetLoadInfo& loadInfo);

		Script(Script&&) noexcept = default;
		Script(const Script&) = delete;

		Script& operator=(Script&&) = delete;
		Script& operator=(const Script&) = delete;

		// May invalidate pointers holding onto any function from this script,
		// if the internal vector resizes
		ScriptFunc& AddFunc(std::string_view name);

		// May invalidate pointers holding onto any function from this script,
		// if the internal vector resizes
		ScriptFunc& AddEvent(const EventBase& event);

		// May invalidate pointers holding onto any function from this script
		void RemoveFunc(Name name);

		ScriptFunc* TryGetFunc(Name name);
		const ScriptFunc* TryGetFunc(Name name) const;

		Span<ScriptFunc> GetFunctions() { return mFunctions; }
		Span<const ScriptFunc> GetFunctions() const { return mFunctions; }

		// May invalidate pointers holding onto any datamembers from this script,
		// if the internal vector resizes
		ScriptField& AddField(std::string_view name);

		// May invalidate pointers holding onto any members from this script
		void RemoveField(Name name);

		ScriptField* TryGetField(Name name);
		const ScriptField* TryGetField(Name name) const;

		Span<ScriptField> GetFields() { return mFields; }
		Span<const ScriptField> GetFields() const { return mFields; }

		/*
		Will make a type containing fields defined in this script and add it to the MetaManager.

		The type will contain no functions yet.

		The creation of scripts happens in several steps. The next step is the PostDeclarationRefresh.
		*/
		MetaType* DeclareMetaType();


		/*Some nodes need to lookup data. To speed up execution,
		each node only does this lookup once, when this function is called.

		Some types may also not exist at the time of loading. The 'this'
		parameter for example, for member functions. So what we do is,
		we first declare all the metaTypes, without any of the functions.
		Then we declare the functions, now with the guarantee that the
		'this' parameter, or any other type created from scripts, exists.
		*/
		void PostDeclarationRefresh();

		/*
		Will add all the functions to the type.

		The functions do nothing when called; first all the script classes are declared, after which Script::DefineMetaType
		is called. See Script::DefineMetaType comment.
		*/
		void DeclareMetaFunctions(MetaType& type);

		/*
		The type created during DeclareMetaType is passed as an argument. The functions inside the type will then be 
		compiled and become usable (if there are no compilation errors).

		The reason that this is in two steps (DeclareMetaType and DefineMetaType) rather than one function, is because script
		Player might reference something in script Enemy; if script player compiles before script enemy, it will not be
		able to find a type called Enemy and fail to compile.
		*/
		void DefineMetaType(MetaType& type, bool OnlyDefineBigFiveAndDestructor);

		// This assumption has been made in a few places, indicated by static_assert(sIsTypeIdTheSameAsNameHash),
		// so if this ever changes, you'll know exactly where you need to start fixing things.
		static constexpr bool sIsTypeIdTheSameAsNameHash = true;

		// Each component script secretly gets assigned a field of type entt::entity with this name.
		// Upon constructing the component, the correct entity id is assigned.
		static constexpr Name sNameOfOwnerField = "Owner"_Name;

		void CollectErrors(ScriptErrorInserter inserter) const;

	private:
#ifdef REMOVE_FROM_SCRIPTS_ENABLED
		// Refreshing often leads to pins having to be replaced,
		// which we cant do if we cant delete the old ones. Make
		// sure your assets are up to date when shipping.
		void Refresh();
#endif // REMOVE_FROM_SCRIPTS_ENABLED

		template<typename T>
		static typename std::vector<T>::iterator GetByName(std::vector<T>& vector, Name name);

		void AddDefaultConstructor(MetaType& toType, bool define) const;
		void AddMoveConstructor(MetaType& toType, bool define) const;
		void AddCopyConstructor(MetaType& toType, bool define) const;
		void AddMoveAssign(MetaType& toType, bool define) const;
		void AddCopyAssign(MetaType& toType, bool define) const;
		void AddDestructor(MetaType& toType, bool define) const;

		void OnSave(AssetSaveInfo& saveInfo) const override;

		std::vector<ScriptFunc> mFunctions{};
		std::vector<ScriptField> mFields{};

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(Script);
	};
}

