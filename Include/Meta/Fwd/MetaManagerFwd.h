#pragma once
#include "Core/EngineSubsystem.h"

#include "Meta/Fwd/MetaTypeIdFwd.h"
#include "Meta/Fwd/MetaReflectFwd.h"
#include "Utilities/IterableRange.h"

namespace CE
{
	class MetaType;

	class MetaManager final :
		public EngineSubsystem<MetaManager>
	{
		friend EngineSubsystem;

		// We only expose the value to the user; not the TypeId key.
		using EachTypeT = IterableRange<EncapsulingForwardIterator<AlwaysSecondEncapsulator, std::unordered_map<Name::HashType, std::reference_wrapper<MetaType>>::iterator>>;

		void PostConstruct() override;

	public:
		MetaType& AddType(MetaType&& type);

		/*
		Get a metatype by name. Name must include namespaces, e.g., "CE::TransformComponent".

		Note:
			If no class exists with this name, this will return nullptr.
			Otherwise returns the metatype.
		*/
		MetaType* TryGetType(Name typeName);

		/*
		Get a metatype by typeId. The hash function used CE::MakeTypeId<>.

		Returns:
			If no class exists with this typehash, this will return nullptr.
			Otherwise returns the type.
		*/
		MetaType* TryGetType(TypeId typeId);

		template<typename T>
		MetaType* TryGetType() { return TryGetType(MakeTypeId<T>()); }

		/*
		Get a metaType. T must be a type for which a Reflect function exists.

		The type will be reflected if it has not been reflected already.
		*/
		template<typename T>
		MetaType& GetType();

		/*
		Returns an iterable range.

		Example:
			for (MetaType& type : MetaManager::Get().EachType())
		*/
		EachTypeT EachType();

		/*
		Removes a type based on it's typeId. The type will be immediately destroyed, any references will become dangling.

		Will return true on success. This function will return false if there is no type with this typeid.
		*/
		bool RemoveType(TypeId typeId);

		/*
		Removes a type based on its name. The type will be immediately destroyed, any references will become dangling.

		Will return true on success. This function will return false if there is no type with this name.
		*/
		bool RemoveType(Name typeName);
	};
}
