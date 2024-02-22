#pragma once
#include "Core/EngineSubsystem.h"

#include "Meta/MetaTypeId.h"
#include "Meta/MetaReflect.h"
#include "Utilities/IterableRange.h"

namespace Engine
{
	class MetaType;

	class MetaManager final :
		public EngineSubsystem<MetaManager>
	{
		friend EngineSubsystem;

		// We only expose the value to the user; not the TypeId key.
		using EachTypeT = IterableRange<EncapsulingForwardIterator<
			AlwaysSecondEncapsulator, std::unordered_map<Name::HashType, std::reference_wrapper<MetaType>>::iterator>>;

		void PostConstruct() override;

	public:
		MetaType& AddType(MetaType&& type);

		/*
		Get a metatype by name. Name must include namespaces, e.g., "Engine::TransformComponent".

		Note:
			If no class exists with this name, this will return nullptr.
			Otherwise returns the metatype.
		*/
		MetaType* TryGetType(Name typeName);

		/*
		Get a metatype by typeId. The hash function used Engine::MakeTypeId<>.

		Returns:
			If no class exists with this typehash, this will return nullptr.
			Otherwise returns the type.
		*/
		MetaType* TryGetType(TypeId typeId);

		template <typename T>
		MetaType* TryGetType() { return TryGetType(MakeTypeId<T>()); }

		/*
		Get a metaType. T must be a type for which a Reflect function exists.

		The type will be reflected if it has not been reflected already.
		*/
		template <typename T>
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

	template <typename T>
	MetaType& MetaManager::GetType()
	{
		static_assert(sIsReflectable<T>,
			R"(Type does not have a reflect function, so the type can never be gotten from the MetaManager. 
If it does have a Reflect function, make sure it is included from wherever this error originated.
If you are trying to reflect an std::vector<std::shared_ptr<const Material>>, you need to include Material.h, ReflectVector.h and ReflectSmartPtr.h.)"
		);

		static constexpr TypeId typeId = MakeTypeId<T>();

		if (MetaType* const existingType = TryGetType(typeId);
			existingType != nullptr)
		{
			LIKELY;
			return *existingType;
		}

		MetaType& newType = AddType(Internal::CallReflect<T>());
		return newType;
	}
}
